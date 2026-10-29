/* AI PlushPet: AMB82-MINI microphone/camera body + remote Gemma personality.
 * See README_AI_PlushPet.md for setup, protocol, examples and hardware limits.
 */
#include <SPI.h>
#include "AmebaILI9341.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include "StreamIO.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "MP4Recording.h"
#include "AmebaFatFS.h"
#include "VideoStream.h"
#include "Base64.h"
#include "GenAI.h"
#include "PlushPetConfig.h"
#include "PlushPetHttp.h"
#include "PlushPetListen.h"
#include "PlushPetFaces.h"
#include "PlushPetPcm.h"
extern "C" {
#include "mmf2_module.h"
#include "module_audio.h"
}

// Audio::end() only stops DMA and never releases the codec, so a second
// Audio::begin() fails silently and tears down the mic thread and speaker
// buffers. File::playMp3() also inits/deinits the same codec on its own.
// Call begin() once, then release/reset the codec around MP3 playback.
class PetAudio : public Audio {
public:
    void releaseCodec()
    {
        mm_module_ctrl(_p_mmf_context, CMD_AUDIO_SET_TRX, 0);
        mm_module_ctrl(_p_mmf_context, CMD_AUDIO_FORCE_DEINIT, 0);
    }
    void resetCodec() { mm_module_ctrl(_p_mmf_context, CMD_AUDIO_SET_RESET, 0); } // re-init + restart DMA
};

#define TFT_RESET 5
#define TFT_DC 4
#define TFT_CS SPI_SS
#define CAMERA_CHANNEL 0
AmebaILI9341 tft(TFT_CS, TFT_DC, TFT_RESET);
AmebaFatFS fs;
AudioSetting configA(PET_AUDIO_PRESET);
PetAudio audio;
AAC aac;
MP4Recording mp4;
PetPcmRecorder pcm;
StreamIO monitor(1, 1), audioToAAC(1, 1), aacToMp4(1, 1), audioToPcm(1, 1);
VideoSetting configV(640, 480, 5, VIDEO_JPEG, 1);
WiFiClient aiClient;
GenAI tts;
int controlSocket = -1; // HTTP control listener, see PlushPetListen.h

// Keep the loop responsive during listening and recording. Network inference and
// the SDK MP3 player are synchronous, with bounded HTTP timeouts.
enum PetState { LISTENING, RECORDING, THINKING, RESPONDING, COOLDOWN };
PetState state = COOLDOWN;
uint32_t stateSince = 0, lastMicPoll = 0, lastWifiAttempt = 0;
bool sdReady = false, monitorRunning = false, recordingSeen = false;
bool cameraReady = false, cameraInitialized = false, autoListen = true, serverStarted = false;
bool audioStarted = false, codecReleased = false;
int micLevel = 0, micPeak = 0, loudSamples = 0;
String clipPath, lastSpeech, lastError, pendingCommand, pendingValue;
String historyUser[3], historyAssistant[3];
unsigned int historyCount = 0;
const PetFace *currentFace = nullptr;
void drawPackedFace(const PetFace &face);
const uint8_t REPLY_ROWS = 4, REPLY_FONT_SIZE = 2;
const int16_t REPLY_LINE_HEIGHT = 20, REPLY_PADDING = 4;
String replyLines[REPLY_ROWS], displayReply;
size_t replyOffset = 0;
uint8_t replyLineCount = 0;
uint32_t lastReplyScroll = 0;

void clearReplyText()
{
    const bool wasVisible = replyLineCount != 0;
    displayReply = ""; replyOffset = 0; replyLineCount = 0;
    for (uint8_t i = 0; i < REPLY_ROWS; ++i) replyLines[i] = "";
    // Restore the portrait even if the next emotion selects the same face.
    if (wasVisible && currentFace) drawPackedFace(*currentFace);
}

void drawReplyLines()
{
    if (!replyLineCount) return;
    const int16_t height = REPLY_ROWS * REPLY_LINE_HEIGHT + 2 * REPLY_PADDING;
    const int16_t top = tft.getHeight() - height;
    tft.fillRectangle(0, top, tft.getWidth(), height, ILI9341_BLACK);
    tft.setFontSize(REPLY_FONT_SIZE);
    tft.setForeground(ILI9341_WHITE); tft.setBackground(ILI9341_BLACK);
    for (uint8_t i = 0; i < replyLineCount; ++i) {
        // A short reply starts on the bottom line; additional lines push it up.
        tft.setCursor(REPLY_PADDING, top + REPLY_PADDING +
                      (REPLY_ROWS - replyLineCount + i) * REPLY_LINE_HEIGHT);
        tft.print(replyLines[i]);
    }
}

bool appendReplyLine()
{
    if (replyOffset >= displayReply.length()) return false;
    const size_t columns = (tft.getWidth() - 2 * REPLY_PADDING) / (6 * REPLY_FONT_SIZE);
    String line;
    while (replyOffset < displayReply.length() && line.length() < columns) {
        uint8_t c = (uint8_t)displayReply[replyOffset++];
        if (c == '\n') break;
        if (c == '\r') continue;
        if (c == '\t') c = ' ';
        // The SDK has a byte font, not a Unicode font. Replace each UTF-8
        // character once rather than printing its bytes as unrelated glyphs.
        if (c >= 0x80) {
            while (replyOffset < displayReply.length() &&
                   ((uint8_t)displayReply[replyOffset] & 0xc0) == 0x80) ++replyOffset;
            c = '?';
        }
        line += (char)(c < 0x20 || c == 0x7f ? ' ' : c);
    }
    // A newline immediately after a full row terminates that row, rather
    // than introducing an extra blank row on the next update.
    if (line.length() == columns) {
        if (replyOffset < displayReply.length() && displayReply[replyOffset] == '\r') ++replyOffset;
        if (replyOffset < displayReply.length() && displayReply[replyOffset] == '\n') ++replyOffset;
    }
    if (replyLineCount == REPLY_ROWS) {
        for (uint8_t i = 1; i < REPLY_ROWS; ++i) replyLines[i - 1] = replyLines[i];
        --replyLineCount;
    }
    replyLines[replyLineCount++] = line;
    return true;
}

void showReplyText(const String &text)
{
    displayReply = text; replyOffset = 0; replyLineCount = 0;
    for (uint8_t i = 0; i < REPLY_ROWS; ++i) replyLines[i] = "";
    while (replyLineCount < REPLY_ROWS && appendReplyLine()) {}
    if (!replyLineCount && currentFace) drawPackedFace(*currentFace);
    drawReplyLines();
    lastReplyScroll = millis();
}

void updateReplyText()
{
    if (replyOffset < displayReply.length() && millis() - lastReplyScroll >= 1500UL) {
        if (appendReplyLine()) drawReplyLines();
        lastReplyScroll = millis();
    }
}

const char *persona =
    "You are Giraffe, a warm, playful plush pet. Speak briefly and kindly in the user's language. "
    "Respond to spoken audio as conversation, using an attached image as context if present. "
    "Return ONLY a JSON object: {\"speech\":\"your reply\",\"emotion\":\"happy\","
    "\"action\":\"none\",\"sound\":\"none\"}. "
    "emotion: neutral,happy,excited,curious,shy,sleepy. "
    "action: none,wave,wink,cheer. sound: none,hello,laugh,wow,curious,thinking,happy,sleepy,yes,no. ";

// Face pixels are sent in bulk with spi_master_write_stream(), which is much
// faster than one SPI.transfer() per byte (153600 bytes per portrait).
//
// That API is asynchronous and TX-only, so it has to be driven carefully --
// see flushFaceBuffer(). If it ever fails to complete we fall back to the
// blocking SPI.transfer() path, so a failure degrades into a slower face
// rather than a blank screen.
//
// Must match the SPIClass the tft was constructed with. AmebaILI9341 defaults
// to &SPI, and setup() calls SPI.setDefaultFrequency().
#define FACE_STREAM_BUFFER 1920  // 960 px = 4 rows of 240; matches the driver chunk
static uint8_t faceBuffer[FACE_STREAM_BUFFER];
static size_t faceBuffered = 0;
static bool faceStreamBroken = false;  // latch: use the slow path from now on

// Wait for TX_BUSY to clear, but never spin forever: an unbounded wait is
// indistinguishable from a hang, and shows up as a blank screen.
static bool faceWaitTxIdle()
{
    uint32_t deadline = millis() + 250UL;  // a full frame is ~61ms at 20MHz
    while (SPI.pSpiMaster->state & SPI_STATE_TX_BUSY) {
        if ((int32_t)(millis() - deadline) >= 0) return false;
    }
    return true;
}

// Transmit the staged bytes through spi_master_write_stream().
//
// Three failure modes are handled here, all confirmed in the SDK source:
//  - It returns HAL_BUSY (not HAL_OK) if a previous transfer is still in
//    flight. Ignoring the return code silently drops the frame.
//  - It is TX-only, so nothing drains the RX FIFO: the write ISR's drain is
//    gated on "rx_length > 0" (rtl8735b_ssi.c:516), which is always false for
//    a pure write, while every byte shifted out shifts one in.
//  - Completion relies on spi_tx_done_callback() clearing TX_BUSY, which is
//    only wired up by spi_format(); if it never runs, the wait above times out.
static bool flushFaceBuffer()
{
    if (!faceBuffered) return true;

    bool ok = false;
    if (!faceStreamBroken) {
        ok = faceWaitTxIdle();
        if (ok) {
            spi_flush_rx_fifo(SPI.pSpiMaster);
            ok = spi_master_write_stream(SPI.pSpiMaster, (char *)faceBuffer,
                                        faceBuffered) == HAL_OK &&
                 faceWaitTxIdle();
            spi_flush_rx_fifo(SPI.pSpiMaster);
        }
        if (!ok) faceStreamBroken = true;
    }

    if (faceStreamBroken) {
        // Blocking write+read per byte: completes on return and drains the RX
        // FIFO as it goes, so it cannot stall or overflow. faceBuffered is a
        // byte count, and the buffer already holds big-endian pixel bytes.
        for (size_t i = 0; i < faceBuffered; ++i) SPI.transfer(faceBuffer[i]);
    }

    faceBuffered = 0;
    return ok;
}

// Repeat/literal packets preserve the portraits exactly while fitting flash.
void drawPackedFace(const PetFace &face)
{
    // Packed RGB565 is already high-byte first; do not byte-swap it.
    tft.setAddress(0, 0, 239, 319);
    digitalWrite(TFT_DC, HIGH);
    size_t offset = 0, pixels = 0;
    while (offset < face.size && pixels < 240UL * 320UL) {
        uint8_t control = face.data[offset++];
        size_t count = (control & 0x7f) + 1;
        if (count > 240UL * 320UL - pixels) break;
        if (control & 0x80) {
            if (offset + 2 > face.size) break;
            uint8_t high = face.data[offset++], low = face.data[offset++];
            for (size_t i = 0; i < count; ++i) {
                faceBuffer[faceBuffered++] = high;
                faceBuffer[faceBuffered++] = low;
                if (faceBuffered == sizeof(faceBuffer)) flushFaceBuffer();
            }
        } else {
            if (offset + count * 2 > face.size) break;
            for (size_t i = 0; i < count * 2; ++i) {
                faceBuffer[faceBuffered++] = face.data[offset++];
                if (faceBuffered == sizeof(faceBuffer)) flushFaceBuffer();
            }
        }
        pixels += count;
    }
    flushFaceBuffer();  // partial final buffer; also reached via break above
}

void showFace(const String &emotion, const String &action = "none")
{
    const PetFace *face = &neutral_smile_face;
    if (emotion == "happy") face = &happy_smile_face;
    if (emotion == "excited") face = &cheering_face;
    if (emotion == "curious") face = &edge_gallery_face;
    if (emotion == "shy" || emotion == "sleepy") face = &shy_smile_face;
    if (action == "wave") face = &waving_face;
    if (action == "wink") face = &waving_wink_face;
    if (action == "cheer") face = &cheering_face;
    if (face != currentFace) {
        drawPackedFace(*face); currentFace = face;
        drawReplyLines();
    }
}

void setState(PetState next) { state = next; stateSince = millis(); }
void reportError(const String &message) { lastError = message; Serial.println("[Error] " + message); }

// WAV clips live in RAM; MP4 clips need the SD card.
bool recorderReady() { return PET_AUDIO_WAV ? pcm.wav() != nullptr : sdReady; }

void stopListening()
{
    if (monitorRunning) { monitor.end(); monitorRunning = false; }
}

// Give the mic/speaker pipeline the codec, starting it on first use.
void claimCodec()
{
    if (!audioStarted) {
        audio.begin(); audioStarted = true;
        audio.muteSpk(1); // Kept across resets; silent loopback supplies micLevel samples.
    } else if (codecReleased) audio.resetCodec();
    codecReleased = false;
}

// File::playMp3() initializes the codec itself; it must start from a released one.
void releaseCodec()
{
    if (audioStarted && !codecReleased) { audio.releaseCodec(); codecReleased = true; }
}

// Say why sound cannot start a recording, or that the pet is ready to hear it.
void printListenStatus()
{
    String blocked;
    if (!PET_ENABLE_AUDIO) blocked += " audio disabled;";
    else if (!monitorRunning) blocked += " mic monitor not running;";
    if (!autoListen) blocked += " listen is off;";
    if (!recorderReady()) blocked += PET_AUDIO_WAV ? " WAV buffer unavailable;" : " no SD card;";
    if (WiFi.status() != WL_CONNECTED) blocked += " WiFi disconnected;";
    if (blocked.length()) Serial.println("[Listen] Auto-record blocked:" + blocked);
    else Serial.println("[Listen] Listening; sound above mic level " + String(PET_TRIGGER_LEVEL) + " starts recording");
}

void startListening()
{
    loudSamples = 0;
    if (PET_ENABLE_AUDIO && !monitorRunning) {
        claimCodec();
        if (monitor.begin() == 0) monitorRunning = true;
        else reportError("Microphone monitor link failed");
    }
    showFace("neutral");
    setState(LISTENING);
    printListenStatus();
}

bool playFile(const String &relativePath)
{
    if (!sdReady) return false;
    String path = String(fs.getRootPath()) + relativePath;
    if (!fs.exists(path)) return false;
    File file = fs.open(path, MP3);
    if (!file) return false;
    if (!file.size() || file.size() > PET_MAX_MEDIA_BYTES) { file.close(); return false; }
    // File::playMp3 owns the audio codec. Stop mic links before handing it over.
    stopListening(); releaseCodec();
    file.setMp3DigitalVol(PET_MP3_VOLUME);
    file.playMp3();
    file.close();
    return true;
}

bool allowedSound(const String &id)
{
    const char *ids[] = {"hello", "laugh", "wow", "curious", "thinking", "happy", "sleepy", "yes", "no"};
    for (const char *s : ids) if (id == s) return true;
    return false;
}

bool playSound(const String &id)
{
    return allowedSound(id) && playFile("audio/" + id + ".mp3");
}

// Overlay one line at the bottom of the face; it stays until the face changes.
void showStatusLine(const String &text)
{
    const int16_t size = 2, height = 8 * size + 8, y = tft.getHeight() - height;
    int16_t x = (tft.getWidth() - (int16_t)text.length() * 6 * size) / 2;
    tft.fillRectangle(0, y, tft.getWidth(), height, ILI9341_BLACK);
    tft.setFontSize(size); tft.setForeground(ILI9341_WHITE); tft.setBackground(ILI9341_BLACK);
    tft.setCursor(x < 0 ? 0 : x, y + 4);
    tft.print(text);
}

void maintainWifi()
{
    if (!strlen(PET_WIFI_SSID)) return;
    if (WiFi.status() != WL_CONNECTED) {
        petCloseSocket(controlSocket); controlSocket = -1; serverStarted = false;
        if (!lastWifiAttempt || millis() - lastWifiAttempt >= 15000UL) {
            lastWifiAttempt = millis();
            WiFi.begin(PET_WIFI_SSID, PET_WIFI_PASSWORD); // Blocks until joined or failed.
        }
    }
    if (WiFi.status() == WL_CONNECTED && !serverStarted) {
        controlSocket = petListen(PET_CONTROL_PORT); serverStarted = true;
        if (controlSocket < 0) reportError("HTTP control server failed to start");
        IPAddress ip = WiFi.localIP();
        Serial.print("[WiFi] PlushPet address: "); Serial.println(ip);
        showStatusLine("IP " + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]));
    }
}

bool beginPost(WiFiClient &client, const char *host, uint16_t port,
               const char *path, const char *key, size_t length)
{
    client.stop();
    if (WiFi.status() != WL_CONNECTED || !strlen(host) || !client.connect(host, port)) {
        reportError("AI connection unavailable"); return false;
    }
    String headers = "POST " + String(path) + " HTTP/1.0\r\nHost: " + String(host) + ":" + String(port) +
        "\r\nContent-Type: application/json; charset=utf-8\r\nConnection: close\r\nContent-Length: " +
        String((unsigned long)length) + "\r\n";
    if (strlen(key)) headers += "Authorization: Bearer " + String(key) + "\r\n";
    headers += "\r\n";
    return petWriteAll(client, headers.c_str(), headers.length());
}

// Stream 768-byte blocks (multiples of 3) so only the last block needs padding.
// Never read beyond the final memory/file byte, unlike the original image example.
bool sendBase64(const uint8_t *data, File *file, size_t length)
{
    char input[768], output[1025];
    size_t offset = 0;
    while (offset < length) {
        size_t count = length - offset;
        if (count > sizeof(input)) count = sizeof(input);
        if (file) {
            if (file->read(input, count) != (int)count) return false;
        } else memcpy(input, data + offset, count);
        int encoded = base64_encode(output, input, count);
        if (encoded != (int)(4 * ((count + 2) / 3)) || !petWriteAll(aiClient, output, encoded)) return false;
        offset += count;
    }
    return true;
}

void remember(const String &user, const String &assistant)
{
    if (historyCount == 3) {
        for (int i = 0; i < 2; ++i) { historyUser[i] = historyUser[i + 1]; historyAssistant[i] = historyAssistant[i + 1]; }
        historyCount = 2;
    }
    historyUser[historyCount] = user.substring(0, 1024);
    historyAssistant[historyCount++] = assistant.substring(0, 2048);
}

// Google TTS uses short requests. Split at UTF-8 boundaries without discarding
// any bytes; prefer spaces when available to avoid cutting English words.
String nextTtsChunk(const String &text, size_t &offset)
{
    size_t end = offset + PET_TTS_CHUNK_BYTES;
    if (end >= text.length()) end = text.length();
    else {
        while (end > offset && ((uint8_t)text[end] & 0xc0) == 0x80) --end;
        for (size_t i = end; i > offset + PET_TTS_CHUNK_BYTES / 2; --i) {
            if (text[i - 1] == ' ') { end = i; break; }
        }
    }
    String chunk = text.substring(offset, end);
    offset = end;
    return chunk;
}

bool sdPlayMP3(const String &filename)
{
    // googletts() calls fs.end() internally, so restore the shared SD host.
    // Do not unmount after playback: the next MP4 recording needs it mounted.
    sdReady = fs.begin();
    if (!sdReady) { reportError("Cannot remount SD for TTS playback"); return false; }
    return playFile(filename);
}

bool speak(const String &text)
{
    if (!PET_ENABLE_TTS || !text.length()) return false;
    if (!sdReady || WiFi.status() != WL_CONNECTED) {
        reportError("Google TTS requires WiFi and an SD card"); return false;
    }
    stopListening();
    const String mp3Filename = PET_TTS_FILENAME;
    String path = String(fs.getRootPath()) + mp3Filename;
    String tempPath = String(fs.getRootPath()) + "temp.MP3";
    size_t offset = 0;
    while (offset < text.length()) {
        String chunk = nextTtsChunk(text, offset);
        if (!chunk.length()) { reportError("Invalid TTS text boundary"); return false; }
        // Both opens in GenAI use OPEN_ALWAYS. Remove stale files first so a
        // failed/shorter download cannot replay or append a previous reply.
        if ((fs.exists(path) && !fs.remove(path)) ||
            (fs.exists(tempPath) && !fs.remove(tempPath))) {
            reportError("Cannot clear previous TTS download"); return false;
        }
        tts.googletts(mp3Filename, chunk, PET_TTS_LANGUAGE);
        delay(500);
        bool ok = sdPlayMP3(mp3Filename);
        if (sdReady) {
            if (fs.exists(path)) fs.remove(path);
            if (fs.exists(tempPath)) fs.remove(tempPath);
        }
        if (!ok) { reportError("Google TTS download/playback failed"); return false; }
    }
    return true;
}

void applyReply(const String &content)
{
    setState(RESPONDING);
    String json = content; json.trim();
    if (json.startsWith("```")) { int nl = json.indexOf('\n'); int end = json.lastIndexOf("```"); if (nl >= 0 && end > nl) json = json.substring(nl + 1, end); }
    JsonDocument reply;
    DeserializationError error = deserializeJson(reply, json);
    String emotion = "happy", action = "none", sound = "none";
    lastSpeech = content.substring(0, 2048);
    if (!error && reply["speech"].is<const char *>()) {
        lastSpeech = reply["speech"].as<String>().substring(0, 2048);
        emotion = reply["emotion"] | "happy";
        action = reply["action"] | "none";
        sound = reply["sound"] | "none";
    }
    showFace(emotion, action);
    Serial.println("[Giraffe] " + lastSpeech);
    showReplyText(lastSpeech);
    if (allowedSound(sound)) playSound(sound);
    else if (emotion == "excited") playSound("wow");
    else if (emotion == "happy") playSound("happy");
    speak(lastSpeech);
}

bool sendAI(const String &prompt, bool withAudio, bool withImage)
{
    Serial.println("[AI] Request to " + String(PET_AI_HOST) + ":" + String(PET_AI_PORT) +
        String(PET_AI_PATH) + " model=" + String(PET_AI_MODEL));
    stopListening(); setState(THINKING); showFace("curious");
    playSound("thinking");
    bool success = false;
    File clip;
    uint32_t imageAddress = 0, imageLength = 0;
    size_t audioLength = 0;
    if (WiFi.status() != WL_CONNECTED) { reportError("WiFi is disconnected"); goto finish; }
    if (withAudio && PET_AUDIO_WAV) {
        audioLength = pcm.finish();
        if (!pcm.pcmBytes()) { reportError("Audio clip empty"); goto finish; }
    } else if (withAudio) {
        if (!sdReady || !fs.exists(clipPath)) { reportError("Audio clip missing"); goto finish; }
        clip = fs.open(clipPath);
        audioLength = clip ? clip.size() : 0;
        if (!audioLength || audioLength > PET_MAX_MEDIA_BYTES) { reportError("Audio clip empty or too large"); goto finish; }
    }
    if (withImage) {
        if (!PET_ENABLE_CAMERA) { reportError("Camera disabled"); goto finish; }
        if (!cameraReady) {
            if (!cameraInitialized) {
                Camera.configVideoChannel(CAMERA_CHANNEL, configV);
                Camera.videoInit(); cameraInitialized = true;
            }
            Camera.channelBegin(CAMERA_CHANNEL);
            cameraReady = true;
            delay(1000);
        }
        Camera.getImage(CAMERA_CHANNEL, &imageAddress, &imageLength);
        if (!imageAddress || !imageLength || imageLength > PET_MAX_MEDIA_BYTES) { reportError("JPEG capture failed or too large"); goto finish; }
    }
    {
        JsonDocument request;
        request["model"] = PET_AI_MODEL;
        // The AMB82 core defines false as integer 0; force a JSON boolean.
        request["stream"] = static_cast<bool>(false);
        // The server accepts exactly one user message, so earlier turns travel
        // as a transcript inside it rather than as separate chat messages.
        String context = persona;
        if (historyCount) {
            context += "Conversation so far:\n";
            for (unsigned int i = 0; i < historyCount; ++i)
                context += "User: " + historyUser[i] + "\nGiraffe: " + historyAssistant[i] + "\n";
            context += "Now the user says: ";
        }
        JsonArray messages = request["messages"].to<JsonArray>();
        JsonObject user = messages.add<JsonObject>(); user["role"] = "user";
        JsonArray parts = user["content"].to<JsonArray>();
        JsonObject text = parts.add<JsonObject>(); text["type"] = "text"; text["text"] = context + prompt;
        if (withAudio) {
            JsonObject p = parts.add<JsonObject>(); p["type"] = "input_audio";
            p["input_audio"]["data"] = "__PET_AUDIO__"; p["input_audio"]["format"] = PET_AUDIO_WAV ? "wav" : "mp4";
        }
        if (withImage) {
            JsonObject p = parts.add<JsonObject>(); p["type"] = "image_url";
            p["image_url"]["url"] = "data:image/jpeg;base64,__PET_IMAGE__";
        }
        String envelope; serializeJson(request, envelope);
        if (request.overflowed()) { reportError("Request allocation failed"); goto finish; }
        // Find the actual media fields, rather than a matching token in user text.
        int audioPos = withAudio ? envelope.indexOf("\"data\":\"__PET_AUDIO__\"") + 8 : -1;
        int imagePos = withImage ? envelope.indexOf("\"url\":\"data:image/jpeg;base64,__PET_IMAGE__\"") + 30 : -1;
        if ((withAudio && (audioPos < 8 || envelope.substring(audioPos, audioPos + 13) != "__PET_AUDIO__")) ||
            (withImage && (imagePos < 30 || envelope.substring(imagePos, imagePos + 13) != "__PET_IMAGE__"))) {
            reportError("Media envelope invalid"); goto finish;
        }
        size_t length = envelope.length();
        if (withAudio) length += 4 * ((audioLength + 2) / 3) - 13;
        if (withImage) length += 4 * ((imageLength + 2) / 3) - 13;
        bool ok = beginPost(aiClient, PET_AI_HOST, PET_AI_PORT, PET_AI_PATH, PET_AI_KEY, length);
        size_t offset = 0;
        if (ok && withAudio) {
            ok = petWriteAll(aiClient, envelope.c_str(), audioPos) &&
                (PET_AUDIO_WAV ? sendBase64(pcm.wav(), nullptr, audioLength) : sendBase64(nullptr, &clip, audioLength));
            offset = audioPos + 13;
        }
        if (ok && withImage) {
            ok = petWriteAll(aiClient, envelope.c_str() + offset, imagePos - offset) &&
                sendBase64((const uint8_t *)(uintptr_t)imageAddress, nullptr, imageLength);
            offset = imagePos + 13;
        }
        if (ok) ok = petWriteAll(aiClient, envelope.c_str() + offset, envelope.length() - offset);
        clip.close();
        if (!ok) { reportError("AI upload failed"); goto finish; }
        PetHttpReader reader(aiClient, PET_RESPONSE_TIMEOUT_MS);
        String first, body;
        ok = reader.headers(first) && reader.text(body, PET_MAX_REPLY_BYTES);
        aiClient.stop();
        if (!ok) { reportError("AI response incomplete, too large or timed out"); goto finish; }
        if (reader.status != 200) { reportError("AI HTTP " + String(reader.status) + ": " + body.substring(0, 256)); goto finish; }
        JsonDocument response;
        DeserializationError error = deserializeJson(response, body);
        if (error || !response["choices"][0]["message"]["content"].is<const char *>()) { reportError("Invalid AI reply"); goto finish; }
        String content = response["choices"][0]["message"]["content"].as<String>();
        if (!content.length()) { reportError("Empty AI reply"); goto finish; }
        success = true; lastError = "";
        remember(withAudio ? "[User spoke to Giraffe]" : prompt, content);
        applyReply(content);
    }
finish:
    clip.close(); aiClient.stop();
    if (cameraReady) { Camera.channelEnd(CAMERA_CHANNEL); cameraReady = false; }
    if (success && withAudio && !PET_AUDIO_WAV && PET_DELETE_UPLOADED_CLIPS) fs.remove(clipPath);
    setState(COOLDOWN);
    return success;
}

bool Ollama_sendText(const String &text) { return sendAI(text, false, false); }
bool Ollama_sendImage(const String &prompt) { return sendAI(prompt, false, true); }
bool Ollama_sendAudio() { return sendAI("Listen and respond to the user.", true, PET_IMAGE_WITH_AUDIO); }

void startRecording()
{
    if (!PET_ENABLE_AUDIO || !recorderReady()) {
        reportError(PET_AUDIO_WAV ? "Recording requires audio enabled and a WAV buffer" :
                                    "Recording requires audio enabled and a mounted SD card");
        return;
    }
    stopListening();
    if (PET_AUDIO_WAV) {
        claimCodec(); pcm.arm();
        if (audioToPcm.begin() != 0) { reportError("Audio-to-PCM link failed"); setState(COOLDOWN); return; }
        setState(RECORDING); showFace("curious");
        Serial.println("[Recording] WAV " + String(PET_RECORD_SECONDS) + " s in RAM");
        return;
    }
    const char *name = "PetClip";
    clipPath = String(fs.getRootPath()) + name + ".mp4";
    // Replace the previous recording, including any failed/unsent clip.
    if (fs.exists(clipPath) && !fs.remove(clipPath)) {
        reportError("Cannot replace previous audio recording"); setState(COOLDOWN); return;
    }
    mp4.setRecordingFileName(name);
    claimCodec(); aac.begin();
    if (audioToAAC.begin() != 0) {
        aac.end(); reportError("Audio-to-AAC link failed"); setState(COOLDOWN); return;
    }
    if (aacToMp4.begin() != 0) {
        audioToAAC.end(); aac.end(); reportError("AAC-to-MP4 link failed"); setState(COOLDOWN); return;
    }
    recordingSeen = false;
    mp4.begin(); setState(RECORDING); showFace("curious");
    Serial.println("[Recording] " + clipPath);
}

void finishRecording(bool ok)
{
    if (PET_AUDIO_WAV) {
        audioToPcm.end();
        Serial.println("[Recording] " + String((unsigned long)pcm.pcmBytes()) + " PCM bytes");
        if (ok) Ollama_sendAudio();
        else { pcm.finish(); reportError("Recording captured no audio"); setState(COOLDOWN); }
        return;
    }
    mp4.end(); aacToMp4.end(); audioToAAC.end(); aac.end();
    if (ok && fs.exists(clipPath)) Ollama_sendAudio();
    else { reportError("Recording failed to start or timed out; clip retained"); setState(COOLDOWN); }
}

// LISTENING is also the idle state for commands and touch; with "listen off"
// sound cannot trigger a recording, so report it as idle instead.
const char *stateName()
{
    const char *states[] = {"listening", "recording", "thinking", "responding", "cooldown"};
    return state == LISTENING && !autoListen ? "idle" : states[state];
}

void printStatus()
{
    Serial.print("State="); Serial.print(stateName());
    Serial.print(" autoListen="); Serial.print(autoListen);
    Serial.print(" micMonitor="); Serial.print(monitorRunning);
    Serial.print(" SD="); Serial.print(sdReady);
    Serial.print(" WiFi="); Serial.println(WiFi.status() == WL_CONNECTED);
    // Auto-record needs all of the above and a peak above the trigger level.
    Serial.print("mic="); Serial.print(micLevel);
    Serial.print(" peakSinceLastStatus="); Serial.print(micPeak);
    Serial.print(" trigger>"); Serial.print(PET_TRIGGER_LEVEL);
    Serial.print(" x"); Serial.println(PET_TRIGGER_SAMPLES);
    micPeak = 0;
    Serial.println("Last reply: " + lastSpeech);
    Serial.println("Last error: " + lastError);
}

bool queueCommand(const String &command, const String &value)
{
    if (command != "text" && command != "speak" && command != "image" && command != "record" && command != "play" &&
        command != "emotion" && command != "listen" && command != "clear") return false;
    if (state != LISTENING && state != COOLDOWN) return false;
    if (pendingCommand.length() || value.length() > 1024) return false;
    if (command == "play" && !allowedSound(value)) return false;
    if (command == "listen" && value != "on" && value != "off") return false;
    if ((command == "text" || command == "speak") && !value.length()) return false;
    pendingCommand = command; pendingValue = value; return true;
}

void serviceSerial()
{
    static String line; static bool overflow = false;
    for (int budget = 0; budget < 128 && Serial.available(); ++budget) {
        char c = Serial.read();
        if (c == '\r' || c == '\n') {
            if (overflow) Serial.println("Command too long");
            else {
                line.trim(); int space = line.indexOf(' ');
                String command = space < 0 ? line : line.substring(0, space);
                String value = space < 0 ? "" : line.substring(space + 1);
                // Ignore blank lines, including the LF following a CR in CRLF.
                if (!line.length()) continue;
                if (command == "status") printStatus();
                else if (!queueCommand(command, value)) Serial.println("Unknown/invalid command or pet busy");
                else Serial.println("[Command] Accepted: " + command);
            }
            line = ""; overflow = false;
        } else if (!overflow) {
            if (line.length() >= 1100 || !line.concat(c)) overflow = true;
        }
    }
}

void sendControlReply(WiFiClient &client, int code, const String &body)
{
    String header = "HTTP/1.0 " + String(code) + " Response\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " + String(body.length()) + "\r\n\r\n";
    petWriteAll(client, header.c_str(), header.length()); petWriteAll(client, body.c_str(), body.length());
}

int queryHexDigit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool decodeCommandQuery(const String &query, String &command, String &value)
{
    // One command per request. Bare text/emotion values are also supported.
    if (!query.length() || query.indexOf('&') >= 0) return false;
    int equal = query.indexOf('=');
    command = equal < 0 ? query : query.substring(0, equal);
    value = "";
    if (!command.length()) return false;
    for (size_t i = equal < 0 ? 0 : equal + 1; i < query.length(); ++i) {
        char c = query[i];
        if (c == '+') c = ' ';
        else if (c == '%') {
            if (i + 2 >= query.length()) return false;
            int high = queryHexDigit(query[i + 1]), low = queryHexDigit(query[i + 2]);
            if (high < 0 || low < 0) return false;
            c = (char)((high << 4) | low); i += 2;
        }
        if (!c || value.length() >= 1024 || !value.concat(c)) return false;
    }
    if (equal < 0) {
        if (value == "record" || value == "clear" || value == "image") {
            command = value; value = "";
        } else if (value == "neutral" || value == "happy" || value == "excited" ||
                   value == "curious" || value == "shy" || value == "sleepy") command = "emotion";
        else command = "text";
    }
    return true;
}

void serviceControl()
{
    int sock = petAccept(controlSocket);
    if (sock < 0) return;
    WiFiClient client((uint8_t)sock);
    PetHttpReader reader(client, 2000UL);
    String first, body;
    if (!reader.headers(first)) { client.stop(); return; }
    Serial.println("[HTTP] " + first);
    if (first.startsWith("GET /status ")) {
        JsonDocument status;
        status["state"] = stateName(); status["mic_level"] = micLevel;
        status["sd_ready"] = sdReady; status["auto_listen"] = autoListen;
        status["speech"] = lastSpeech; status["error"] = lastError;
        status["tts_enabled"] = (bool)PET_ENABLE_TTS;
        status["tts_language"] = PET_TTS_LANGUAGE;
        serializeJson(status, body); sendControlReply(client, 200, body);
    } else if (first.startsWith("GET /command?") || first.startsWith("POST /command?")) {
        int start = first.indexOf('?') + 1, end = first.indexOf(' ', start);
        String command, value;
        if (end < 0 || !decodeCommandQuery(first.substring(start, end), command, value))
            sendControlReply(client, 400, "{\"error\":\"use /command?message or /command?emotion=happy; one command per request\"}");
        else if (queueCommand(command, value))
            sendControlReply(client, 202, "{\"accepted\":true}");
        else sendControlReply(client, 409, "{\"error\":\"busy or invalid command/value\"}");
    } else if (!first.startsWith("POST /command ")) sendControlReply(client, 404, "{\"error\":\"unknown endpoint\",\"endpoints\":[\"GET /status\",\"GET /command?text=message\",\"POST /command\"]}");
    else if (reader.chunked || reader.remaining < 0 || reader.remaining > 2048 || !reader.text(body, 2048))
        sendControlReply(client, 400, "{\"error\":\"bounded Content-Length required\"}");
    else {
        JsonDocument command;
        DeserializationError error = deserializeJson(command, body);
        if (error || !command["command"].is<const char *>()) sendControlReply(client, 400, "{\"error\":\"invalid JSON command\"}");
        else if (queueCommand(command["command"].as<String>(), command["value"] | ""))
            sendControlReply(client, 202, "{\"accepted\":true}");
        else sendControlReply(client, 409, "{\"error\":\"busy or invalid command/value\"}");
    }
    client.stop();
}

void executeCommand()
{
    if (!pendingCommand.length()) return;
    String command = pendingCommand, value = pendingValue;
    pendingCommand = ""; pendingValue = "";
    clearReplyText();
    if (command == "text") Ollama_sendText(value);
    else if (command == "speak") {
        setState(RESPONDING); showFace("happy");
        lastSpeech = value; Serial.println("[Giraffe] " + lastSpeech);
        showReplyText(lastSpeech);
        speak(value); setState(COOLDOWN);
    }
    else if (command == "image") Ollama_sendImage(value.length() ? value : "What do you see?");
    else if (command == "record") startRecording();
    else if (command == "play") { playSound(value); setState(COOLDOWN); }
    else if (command == "emotion") showFace(value);
    else if (command == "listen") {
        autoListen = value == "on";
        if (state == LISTENING) printListenStatus(); // Otherwise startListening() reports it.
    }
    else if (command == "clear") {
        for (int i = 0; i < 3; ++i) { historyUser[i] = ""; historyAssistant[i] = ""; }
        historyCount = 0; lastSpeech = ""; lastError = "";
    }
}

void setup()
{
    Serial.begin(115200);
    SPI.setDefaultFrequency(20000000);
    tft.begin(); tft.clr(); showFace("happy", "wave");
    sdReady = fs.begin(); // Keep SD mounted; fs.end() breaks subsequent recordings.
    if (!sdReady) reportError("SD card unavailable; text/image requests still work");
    if (PET_ENABLE_AUDIO) {
        audio.configAudio(configA); aac.configAudio(configA);
        // Stored in configA; applied by audio.begin() and kept across codec resets.
        audio.setAMicBoost(PET_MIC_BOOST_DB == 40 ? MIC_40DB : PET_MIC_BOOST_DB == 30 ? MIC_30DB :
                           PET_MIC_BOOST_DB == 20 ? MIC_20DB : MIC_0DB);
        if (PET_AUDIO_WAV) {
            if (pcm.begin(configA._sampleRate, configA._audioParams.channel, PET_RECORD_SECONDS)) {
                audioToPcm.registerInput(audio); audioToPcm.registerOutput(pcm);
            } else reportError("WAV recording buffer allocation failed");
        }
        mp4.configAudio(configA, CODEC_AAC); mp4.setRecordingDataType(STORAGE_AUDIO);
        mp4.setRecordingDuration(PET_RECORD_SECONDS); mp4.setRecordingFileCount(1);
        monitor.registerInput(audio); monitor.registerOutput(audio);
        audioToAAC.registerInput(audio); audioToAAC.registerOutput(aac);
        aacToMp4.registerInput(aac); aacToMp4.registerOutput(mp4);
    }
    if (PET_TOUCH_PIN >= 0) pinMode(PET_TOUCH_PIN, INPUT_PULLUP);
    playSound("hello");
    if (strlen(PET_WIFI_SSID)) {
        showStatusLine("WiFi connecting...");
        maintainWifi();
        if (WiFi.status() != WL_CONNECTED) showStatusLine("WiFi not connected");
        delay(PET_IP_DISPLAY_MS); // Keep the address readable before the first face change.
    }
    setState(COOLDOWN);
    Serial.println("AI PlushPet ready (Serial line ending: Newline or Carriage return): status | text <message> | speak <text> | image <prompt> | record | play <id> | emotion <name> | listen on/off | clear");
}

void loop()
{
    maintainWifi(); serviceSerial(); serviceControl(); executeCommand();
    updateReplyText();
    if (state == RECORDING && PET_AUDIO_WAV) {
        uint32_t elapsed = millis() - stateSince;
        if (pcm.full()) finishRecording(true);
        else if (elapsed > (PET_RECORD_SECONDS + 2UL) * 1000UL) finishRecording(pcm.pcmBytes() > 0);
    } else if (state == RECORDING) {
        bool running = mp4.getRecordingState() != 0;
        if (running) recordingSeen = true;
        uint32_t elapsed = millis() - stateSince;
        if (recordingSeen && !running) finishRecording(true);
        else if ((!recordingSeen && elapsed > 2000UL) || elapsed > (PET_RECORD_SECONDS + 10UL) * 1000UL) finishRecording(false);
    } else if (state == COOLDOWN && millis() - stateSince >= PET_COOLDOWN_MS) startListening();
    else if (state == LISTENING) {
        static bool touchWasDown = false;
        bool touchDown = PET_TOUCH_PIN >= 0 && digitalRead(PET_TOUCH_PIN) == LOW;
        if (touchDown && !touchWasDown) { showFace("happy"); playSound("happy"); setState(COOLDOWN); }
        touchWasDown = touchDown;
        if (!autoListen) loudSamples = 0; // "listen off": no mic polling, triggers or [Mic] lines.
        else if (state == LISTENING && PET_ENABLE_AUDIO && monitorRunning && millis() - lastMicPoll >= 100UL) {
            lastMicPoll = millis(); micLevel = audio.micLevel(); if (micLevel > micPeak) micPeak = micLevel;
            static int reportPeak = 0; static uint32_t lastMicReport = 0;
            if (micLevel > reportPeak) reportPeak = micLevel;
            if (PET_MIC_DEBUG && millis() - lastMicReport >= 1000UL) {
                Serial.println("[Mic] peak=" + String(reportPeak) + " trigger>" + String(PET_TRIGGER_LEVEL));
                reportPeak = 0; lastMicReport = millis();
            }
            if (recorderReady() && WiFi.status() == WL_CONNECTED && micLevel > PET_TRIGGER_LEVEL) ++loudSamples;
            else loudSamples = 0;
            if (loudSamples >= PET_TRIGGER_SAMPLES) startRecording();
        }
    }
    delay(5);
}
