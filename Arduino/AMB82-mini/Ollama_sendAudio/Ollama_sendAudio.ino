/*
AMB82-mini: sound-triggered audio recording and Ollama transcription.
Insert an SD card. Monitor at 115200 baud; tune TRIGGER_VOL for your microphone.
Records five seconds AFTER the trigger (no pre-roll), then resumes listening.
*/
#include <ArduinoJson.h> 
#include <WiFi.h>
#include "StreamIO.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "MP4Recording.h"
#include "AmebaFatFS.h"
#include "Base64.h"

#define FILENAME "TestRecordingAudioOnly"
#define RECORDING_SECONDS 5
#define RESPONSE_TIMEOUT_MS 120000UL

#define CHANNEL 0
#define TRIGGER_VOL 7000

// Define States
enum AppState {
    STATE_LISTENING,
    STATE_RECORDING,
    STATE_SENDING
};

AppState currentState = STATE_LISTENING;
WiFiClient client;
AmebaFatFS fs;
JsonDocument doc;

#define PRESET 0


// Default audio preset configurations:
// 0 :  8kHz Mono Analog Mic
// 1 : 16kHz Mono Analog Mic
// 2 :  8kHz Mono Digital PDM Mic
// 3 : 16kHz Mono Digital PDM Mic
VideoSetting configV(CHANNEL);
AudioSetting configA(PRESET);
Audio audio;
AAC aac;
MP4Recording mp4;
StreamIO audioStreamer(1, 1);    // 1 Input Audio -> 1 Output Audio
StreamIO audioToAAC(1, 1);
StreamIO avMixStreamer(2, 1);    // 2 Input Video + Audio -> 1 Output MP4

// WiFi settings
char ssid[] = "HITRON-DF90-2.4G";    // your network SSID (name)
char pass[] = "0972211921";        // your network password
int status = WL_IDLE_STATUS;     // Indicator of Wifi status

//const char *myDomain = "192.168.0.13"; // Ollama Server IP address
//String model = "gemma4:e2b";
//String ollama_key = "5c77421befed47b1a7da2dafc01a590b";
const char *myDomain = "192.168.0.16"; // Ollama Server IP address
String model = "gemma4:e4b";
String ollama_key = "ollama";
String prompt = "Transcribe the audio";


void setup()
{
    Serial.begin(115200);

    // Attempt to connect to WiFi network
    while (status != WL_CONNECTED) {
        Serial.print("\r\nAttempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }

    // Configure camera video channel with video format information
    Camera.configVideoChannel(CHANNEL, configV);
    Camera.videoInit();

    // Configure audio peripheral for audio data format
    audio.configAudio(configA);

    // Configure AAC audio encoder
    aac.configAudio(configA);

    // Configure MP4 recording settings
    mp4.configVideo(configV);
    mp4.configAudio(configA, CODEC_AAC);
    mp4.setRecordingDuration(5);
    mp4.setRecordingFileCount(1);
    mp4.setRecordingFileName(FILENAME);    

    // Configure StreamIO object to stream data from audio mic input to audio speaker output
    audioStreamer.registerInput(audio);
    audioStreamer.registerOutput(audio);

    audioToAAC.registerInput(audio);
    audioToAAC.registerOutput(aac);

    // Configure StreamIO object to stream data from video channel and AAC encoder to MP4 recording
    avMixStreamer.registerInput1(Camera.getStream(CHANNEL));
    avMixStreamer.registerInput2(aac);
    avMixStreamer.registerOutput(mp4);

    audio.begin();
    audio.muteSpk(1); // Keep the listening loopback from causing acoustic feedback.
    Serial.println("Audio begin");
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
            send_Ollama();
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
    currentState = STATE_LISTENING; // Failure paths always return to listening.

    aac.begin();
    audioToAAC.begin();    
    Camera.channelBegin(CHANNEL);

    if (avMixStreamer.begin() != 0) {
        Serial.println("StreamIO avMixStreamer link start failed");
    }

    mp4.begin();

    uint32_t started = millis();
    // Allow the recorder task to start before checking for completion.
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
    avMixStreamer.end();

    Camera.channelEnd(CHANNEL);

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

void send_Ollama()
{
    Serial.println("[State] Sending audio to Ollama...");
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
    String filepath = String(fs.getRootPath()) + FILENAME +".mp4";
    if (!fs.exists(filepath)) {
        Serial.println("[Error] Recording file not found: " + filepath);
        fs.end();
        return;
    }
    File file = fs.open(filepath);
    unsigned int fileSize = file ? file.size() : 0;
    if (!fileSize || fileSize > 1024UL * 1024UL) {
        Serial.println("[Error] Recording is empty, unreadable, or exceeds 1 MB.");
        file.close();
        fs.end();
        return;
    }
    char *fileinput = (char *)malloc(fileSize);
    if (!fileinput) {
        Serial.println("[Error] Not enough memory for recording.");
        file.close();
        fs.end();
        return;
    }
    int bytesRead = file.read(fileinput, fileSize);
    file.close();
    fs.end();
    if (bytesRead != (int)fileSize) {
        Serial.println("[Error] Incomplete SD card read.");
        free(fileinput);
        return;
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

    // Serialize settings to escape any quotes in a customized prompt/model name.
    doc.clear();
    doc["model"] = model;
    doc["stream"] = false;
    doc["messages"][0]["role"] = "user";
    doc["messages"][0]["content"][0]["type"] = "text";
    doc["messages"][0]["content"][0]["text"] = prompt;
    String prefix;
    serializeJson(doc, prefix);
    doc.clear();
    // Replace the closing content-array/message/request delimiters with audio.
    prefix.remove(prefix.length() - 4);
    prefix += ",{\"type\":\"input_audio\",\"input_audio\":{\"data\":\"";
    const char *suffix = "\",\"format\":\"mp4\"}}]}]}";

    client.stop();
    Serial.println("Connect to " + String(myDomain));
    if (!client.connect(myDomain, 11434)) {
        Serial.println("[Error] Ollama connection failed.");
        free(encodedData);
        return;
    }
    // HTTP/1.0 plus close requests an unchunked response, simplifying decoding.
    client.println("POST /v1/chat/completions HTTP/1.0");
    client.println("Host: " + String(myDomain) + ":11434");
    client.println("Authorization: Bearer " + ollama_key);
    client.println("Content-Type: application/json; charset=utf-8");
    client.println("Content-Length: " + String(prefix.length() + encodedLen + strlen(suffix)));
    client.println("Connection: close");
    client.println();
    bool sent = writeAll(prefix.c_str(), prefix.length()) &&
                writeAll(encodedData, encodedLen) && writeAll(suffix, strlen(suffix));
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
            Serial.println("[Error] Ollama response timed out.");
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
                Serial.println("[Error] Ollama response exceeds available buffer.");
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
        Serial.print("[Error] Ollama HTTP status: ");
        Serial.println(httpStatus);
        Serial.println(feedback);
        return;
    }
    DeserializationError error = deserializeJson(doc, feedback);
    if (error) {
        Serial.print("[Error] Invalid Ollama JSON: ");
        Serial.println(error.c_str());
    } else if (doc["choices"][0]["message"]["content"].is<const char *>()) {
        Serial.println("[Transcription]");
        Serial.println(doc["choices"][0]["message"]["content"].as<const char *>());
    } else {
        Serial.println("[Error] Ollama returned no transcription.");
        Serial.println(feedback);
    }
    doc.clear();
}
