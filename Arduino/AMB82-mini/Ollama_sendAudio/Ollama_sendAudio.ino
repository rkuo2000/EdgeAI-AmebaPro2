/*
* AMB82-mini : sound-triggered AUDIO recording -> Google AI Edge Gallery on-device Gemma
*             via the "Text / image / audio API" (Ollama-compatible server on the phone).

* This is the FIXED version of Ollama_sendAudio.ino.
*
* What was wrong with the customer build (see the failing log):
*   The sketch recorded STORAGE_ALL (audio + video) and drove the recording through the
*   camera/VOE pipeline, which never initialized on the board:
*       "It don't do the sensor initial process"
*       "VOE command 0x206 fail ret 0x0"     (0x206 = VOE_OPEN_CMD)
*       "[Error] Recording did not start or timed out."
*   The EdgeAI phone app is NOT involved in that failure - nothing was ever sent to it.
*
* Fixes applied:
*   1. mp4.setRecordingDataType(STORAGE_AUDIO)   <- audio-only MP4, no camera, no VOE.
*   2. No Camera.* / VideoSetting calls at all; aac -> mp4 is a 1-in-1-out StreamIO.
*   3. Robust HTTP/1.0 upload with timeouts, API-key check, JSON response parsing kept.
*
* How to run:
*   1. Insert an SD card into the AMB82-mini (the MP4 is saved to it).
*   2. Phone: Edge Gallery -> Model Management -> download Gemma-4-E4B-it (needs 12 GB RAM)
*      or Gemma-4-E2B-it (8 GB RAM).
*   3. Phone: AI Chat -> send one message so the model fully initializes.
*   4. Phone: tap "Text / image / audio API" -> Allow local-network permission.
*      KEEP THAT DIALOG OPEN ("Ready. Keep this screen open."). It lists:
*          http://<phone-ip>:11434/v1/chat/completions  and an API key.
*   5. Copy <phone-ip> into myDomain and the API key into ollama_key below, then flash.
*/

#include <ArduinoJson.h>
#include <WiFi.h>
#include "StreamIO.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "MP4Recording.h"
#include "AmebaFatFS.h"
#include "Base64.h"

#define FILENAME_BASE      "Clip"                      // per-trigger unique base name
#define RECORDING_SECONDS 5
#define RESPONSE_TIMEOUT_MS 120000UL

#define TRIGGER_VOL 7000                                // tune for your mic level

// Default audio preset configurations:
// 0 :  8 kHz Mono Analog Mic
// 1 : 16 kHz Mono Analog Mic
// 2 :  8 kHz Mono Digital PDM Mic
// 3 : 16 kHz Mono Digital PDM Mic
#define PRESET 0

enum AppState {
    STATE_LISTENING,
    STATE_RECORDING,
    STATE_SENDING
};
AppState currentState = STATE_LISTENING;

AudioSetting configA(PRESET);
Audio audio;
AAC aac;
MP4Recording mp4;
StreamIO audioStreamer(1, 1);   // 1 Input Audio -> 1 Output Audio   (mic-level monitor)
StreamIO audioToAAC(1, 1);     // 1 Input Audio -> 1 Output AAC    (recording path)
StreamIO aacToMp4(1, 1);       // 1 Input AAC  -> 1 Output MP4     (muxer, audio only)

WiFiClient client;
AmebaFatFS fs;
JsonDocument doc;
int clipCounter = 0;                       // bumped each recording -> unique filename
char currentFilePath[64];                  // e.g. "/Clip1.mp4" (filled at each trigger)

// WiFi settings
char ssid[] = "HITRON-DF90-2.4G";     // your network SSID (name)
char pass[] = "0972211921";           // your network password
int status = WL_IDLE_STATUS;          // Indicator of Wifi status

// Google AI Edge Gallery API (from the "Text / image / audio API" dialog)
const char *myDomain = "192.168.0.199";              // <-- phone IP shown in the dialog
String model = "gemma4:e2b";                        // "gemma4:e2b" if you loaded E2B
String ollama_key = "333a3555f457411091ceab6225700431";     // <-- API key from the dialog
String prompt = "Transcribe the audio";


void setup()
{
    Serial.begin(115200);

    // Attempt to connect to WiFi network
    while (status != WL_CONNECTED) {
        Serial.print("\r\nAttempting to connect to SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        // wait 10 seconds for connection:
        delay(10000);
    }

    // Audio peripheral (analog mic, into an AAC encoder, into an AUDIO-ONLY MP4).
    audio.configAudio(configA);
    audio.begin();
    audio.muteSpk(1);                     // Keep the listening loopback quiet (no feedback).

    aac.configAudio(configA);

    mp4.configAudio(configA, CODEC_AAC);
    mp4.setRecordingDuration(RECORDING_SECONDS);
    mp4.setRecordingFileCount(1);
    mp4.setRecordingFileName(FILENAME_BASE);      // actual file becomes Clip<counter> at trigger
    mp4.setRecordingDataType(STORAGE_AUDIO);      // *** audio-only: no camera / no VOE ***

    audioStreamer.registerInput(audio);
    audioStreamer.registerOutput(audio);

    audioToAAC.registerInput(audio);
    audioToAAC.registerOutput(aac);

    aacToMp4.registerInput(aac);
    aacToMp4.registerOutput(mp4);

    Serial.println("Audio begin (STORAGE_AUDIO, trigger recording)");
}

void loop()
{
    switch (currentState) {
        case STATE_LISTENING:
            doListening();
            break;
        case STATE_RECORDING:
            doRecording();
            break;
        case STATE_SENDING:
            send_to_gallery();
            currentState = STATE_LISTENING;
            Serial.println("[State] Returning to Listen Mode");
            delay(1000);
            break;
    }
}

void doListening()
{
    Serial.println("[State] Listening...");

    if (audioStreamer.begin() != 0) {
        Serial.println("Error starting audioStreamer");
    }

    while (currentState == STATE_LISTENING) {
        delay(100);    // To avoid flooding the Serial monitor

        int currentVol = audio.micLevel();

        static int count = 0;
        if (count++ > 10) {
            Serial.print("Mic Level: ");
            Serial.println(currentVol);
            count = 0;
        }

        if (currentVol > TRIGGER_VOL) {
            Serial.print("Mic Level: ");
            Serial.println(currentVol);
            Serial.println(">>> TRIGGERED <<<");
            currentState = STATE_RECORDING;
        }
    }
    audioStreamer.end();
    Serial.println("[State] Listening Stopped.");
}

void doRecording(void)
{
    Serial.println("[State] Recording Setup");
    currentState = STATE_LISTENING;  // Failure paths always return to listening.

    // Use a UNIQUE filename per trigger. The MP4 muxer cannot overwrite an existing
    // file, so re-running with the same name fails: "open file ... fail. Can't open
    // the file  STORAGE_ERROR". Counting up avoids the collision entirely.
    clipCounter++;
    snprintf(currentFilePath, sizeof(currentFilePath), "%s%d", FILENAME_BASE, clipCounter);
    mp4.setRecordingFileName(currentFilePath);

    aac.begin();
    audioToAAC.begin();
    if (aacToMp4.begin() != 0) {
        Serial.println("StreamIO aacToMp4 link start failed");
    }

    mp4.begin();

    // Allow the muxer to start before checking for completion.
    uint32_t started = millis();
    while (!mp4.getRecordingState() && millis() - started < 2000UL) {
        delay(10);
    }
    bool recordingStarted = mp4.getRecordingState() != 0;
    while (mp4.getRecordingState() &&
           millis() - started < (RECORDING_SECONDS + 10UL) * 1000UL) {
        delay(100);
    }
    bool complete = recordingStarted && !mp4.getRecordingState();

    mp4.end();
    aacToMp4.end();
    audioToAAC.end();
    aac.end();

    if (complete) {
        Serial.println("[State] Recording Done");
        currentState = STATE_SENDING;
    } else {
        Serial.println("[Error] Recording did not start or timed out.");
        delay(1000);
    }
}

bool writeAll(const char *data, size_t length)
{
    uint32_t lastProgress = millis();
    while (length > 0) {
        if (!client.connected() || millis() - lastProgress > 15000UL) return false;
        size_t block = length > 1024 ? 1024 : length;
        size_t written = client.write((const uint8_t *)data, block);
        if (written) {
            data += written;
            length -= written;
            lastProgress = millis();
        } else {
            delay(1);
        }
    }
    return true;
}

void send_to_gallery()
{
    Serial.println("[State] Sending audio to Edge AI Gallery...");
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, pass);
        uint32_t started = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - started < 15000UL) delay(100);
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[Error] WiFi disconnected.");
            return;
        }
    }
    if (!fs.begin()) {
        Serial.println("[Error] Cannot mount SD card.");
        return;
    }
    String filepath = String(fs.getRootPath()) + currentFilePath + ".mp4";
    if (!fs.exists(filepath)) {
        Serial.println("[Error] Recording file not found: " + filepath);
        return;    // no fs.end() - keep the SD mounted for the next recording cycle
    }
    File file = fs.open(filepath);
    unsigned int fileSize = file ? file.size() : 0;
    if (!fileSize || fileSize > 1024UL * 1024UL) {
        Serial.println("[Error] Recording is empty, unreadable, or exceeds 1 MB.");
        file.close();
        return;    // no fs.end()
    }
    char *fileinput = (char *)malloc(fileSize);
    if (!fileinput) {
        Serial.println("[Error] Not enough memory for recording.");
        file.close();
        return;    // no fs.end()
    }
    int bytesRead = file.read(fileinput, fileSize);
    file.close();
    // NOTE: NEVER call fs.end() on the read path - it de-inits the SD GPIO host, and the
    // NEXT recording's muxer will then fail with "open file ... fail. Can't open the file".
    if (bytesRead != (int)fileSize) {
        Serial.println("[Error] Incomplete SD card read.");
        free(fileinput);
        return;    // no fs.end()
    }

    int encodedLen = base64_enc_len(fileSize);
    char *encodedData = (char *)malloc(encodedLen + 1); // Include terminating NUL.
    if (!encodedData) {
        Serial.println("[Error] Not enough memory for Base64 audio.");
        free(fileinput);
        return;
    }
    base64_encode(encodedData, fileinput, fileSize);
    free(fileinput);
    if (encodedLen < 50) {
        Serial.println("[Error] Audio file is nearly empty; did recording produce samples?");
        free(encodedData);
        return;
    }

    // Build the request body as ONE ArduinoJson document so the JSON is ALWAYS valid
    // (no byte-trimming of a serialized string, no field-order or newline dependence).
    // Schema matches the EdgeAI Gallery server's /v1/chat/completions parser:
    //   {"model":..., "messages":[{"role":"user","content":[
    //        {"type":"text","text":...},
    //        {"type":"input_audio","input_audio":{"data":"<base64>","format":"mp4"}}  ]}]}
    doc.clear();
    doc["model"] = model;
    doc["stream"] = false;                  // server rejects streaming ("Streaming is not supported")
    JsonArray messages = doc["messages"].to<JsonArray>();
    JsonObject userMsg = messages.add<JsonObject>();
    userMsg["role"] = "user";
    JsonArray content = userMsg["content"].to<JsonArray>();
    JsonObject textPart = content.add<JsonObject>();
    textPart["type"] = "text";
    textPart["text"] = prompt;
    JsonObject audioPart = content.add<JsonObject>();
    audioPart["type"] = "input_audio";
    JsonObject audioData = audioPart["input_audio"].to<JsonObject>();
    audioData["data"] = encodedData;          // base64 string (null-terminated)
    audioData["format"] = "mp4";
    String requestBody;
    serializeJson(doc, requestBody);          // complete, valid JSON, "stream":false implied
    doc.clear();

    // Debug: print only the structural start of the request (not the full base64).
    Serial.print("[DEBUG] JSON head: ");
    Serial.println(requestBody.substring(0, requestBody.indexOf("data") + 8));

    // Send the whole request in one writeAll using its exact length.
    String bodyForSend = requestBody;

    client.stop();
    Serial.println("Connect to " + String(myDomain));
    if (!client.connect(myDomain, 11434)) {
        Serial.println("[Error] Gallery connection failed.");
        free(encodedData);
        return;
    }
    // HTTP/1.0 plus close requests an unchunked response, simplifying decoding.
    client.println("POST /v1/chat/completions HTTP/1.0");
    client.println("Host: " + String(myDomain) + ":11434");
    client.println("Authorization: Bearer " + ollama_key);
    client.println("Content-Type: application/json; charset=utf-8");
    client.println("Content-Length: " + String(bodyForSend.length()));
    client.println("Connection: close");
    client.println();
    bool sent = writeAll(bodyForSend.c_str(), bodyForSend.length());
    free(encodedData);
    if (!sent) {
        Serial.println("[Error] Audio upload failed.");
        client.stop();
        return;
    }
    Serial.println("[INFO] Audio sent; waiting for transcription...");

    String line, feedback;
    bool inBody = false;
    bool statusRead = false;
    int httpStatus = 0;
    uint32_t started = millis();
    bool failed = false;
    while (client.connected() || client.available()) {
        if (millis() - started >= RESPONSE_TIMEOUT_MS) {
            Serial.println("[Error] Gallery response timed out.");
            failed = true;
            break;
        }
        if (!client.available()) {
            delay(10);
            continue;
        }
        char c = (char)client.read();
        if (inBody) {
            if (feedback.length() >= 32768 || !feedback.concat(c)) {
                Serial.println("[Error] Gallery response exceeds available buffer.");
                failed = true;
                break;
            }
        } else if (c == '\n') {
            if (!statusRead) {
                httpStatus = line.substring(line.indexOf(' ') + 1).toInt();
                statusRead = true;
            } else if (line.length() == 0) {
                inBody = true;
            }
            line = "";
        } else if (c != '\r') {
            if (line.length() >= 1024) {
                Serial.println("[Error] HTTP header too long.");
                failed = true;
                break;
            }
            line += c;
        }
    }
    client.stop();
    if (failed) return;
    if (httpStatus != 200) {
        Serial.print("[Error] Gallery HTTP status: ");
        Serial.println(httpStatus);
        Serial.println(feedback);
        return;
    }
    DeserializationError error = deserializeJson(doc, feedback);
    if (error) {
        Serial.print("[Error] Invalid Gallery JSON: ");
        Serial.println(error.c_str());
    } else if (doc["choices"][0]["message"]["content"].is<const char *>()) {
        Serial.println("[Transcription]");
        Serial.println(doc["choices"][0]["message"]["content"].as<const char *>());
    } else {
        Serial.println("[Error] Gallery returned no transcription.");
        Serial.println(feedback);
    }
    doc.clear();

    // The clip was uploaded. Optionally delete it so the SD doesn't fill up across many
    // triggers. The disk is STILL MOUNTED here (we never called fs.end() after reading),
    // and it MUST stay mounted for the next recording cycle.
    String oldFile = String(fs.getRootPath()) + currentFilePath + ".mp4";
    if (fs.exists(oldFile)) {
        if (fs.remove(oldFile)) {
            Serial.println("[INFO] Deleted uploaded clip: " + oldFile);
        } else {
            Serial.println("[WARN] Could not delete clip: " + oldFile);
        }
    }
    // Do NOT call fs.end() here - it de-inits the SD GPIO host and breaks the next
    // recording ("open file ... fail") until the whole board is power-cycled.
}
