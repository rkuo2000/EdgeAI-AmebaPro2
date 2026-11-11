/*
   RoboCar_Gemma4 : HUB8735-ultra RoboCar guided by Gemma-4 (VLA) on an Ollama-compatible server

   Hardware
     - HUB8735-ultra : camera (scene images), mic (voice commands), audio out (MP3 / TTS)
     - DRV8833       : two DC motors (left / right wheel)
     - MPU6050       : heading angle (Yaw) via DMP
     - PAM8403       : audio amplifier + 8 ohm speaker
     - SD card       : required (voice clips + TTS mp3 are stored on it)

   Server
     Ollama (PC) or Google AI Edge Gallery "Text / image / audio API" (Android phone)
     running Gemma-4-E2B-it, endpoint  http://<server-ip>:11434/v1/chat/completions

   Features (see README.md)
     1. Init DRV8833 + MPU6050 (+ camera, audio, SD, WiFi), then flash LED twice = ready.
     2. Status is spoken by TTS and displayed on the built-in HTTP server.
     3. "status" = IMU heading (Yaw); "scene" = camera image + scene recognition by Gemma-4.
     4. Commands by voice, serial port or HTTP page.
     5. Every movement is a 1-second step (or a 45 deg IMU turn) followed by a stop,
        so the car can look at the scene again to position itself.
     6. Prompts tell Gemma-4 the exact one-line JSON format the RoboCar understands.
     7. "go to kitchen" : navigation guided by periodic image scene recognition + HOME_MAP routes.
     8. "find the dog"  : look around until the object is seen, stop, and say "I found the dog".

   Commands (serial / web text box / voice)
     F, B, L, R, S          forward / backward / turn left / turn right / stop (1 step)
     V or listen            record a 3-second voice command now
     status                 speak the IMU heading angle
     scene, look            capture an image, recognize place + scene with Gemma-4, speak it
     imu reset              re-initialize + calibrate the MPU6050 (keep the car still), read the heading
     go to <place>          navigate, e.g. "go to kitchen"
     find <object>          search, e.g. "find the dog"
     anything else          sent to Gemma-4, which converts it into a RoboCar command

   Web : http://<robocar-ip>/   (buttons, text command, live status, camera snapshot)
         /F /B /L /R /S /V   /cmd?q=find+the+dog   /status.json   /snap.jpg

   Board package : ideasHatch AmebaPro2 (HUB8735-ultra). Hands-free voice triggering needs
   Audio::micLevel(), which only some AmebaPro2 cores provide; it is detected at compile time.
   Without it, start a voice command with "V" (serial) or the "Voice" web button.
*/

#include <WiFi.h>
#include <ArduinoJson.h>
#include "Wire.h"
#include "I2Cdev.h"
#include <MPU6050_IMU_libraries/MPU6050_6Axis_MotionApps612.h>
#include "VideoStream.h"
#include "StreamIO.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "MP4Recording.h"
#include "AmebaFatFS.h"
#include "Base64.h"
#include "RoboCar_Gemma4.h"  // Audio::micLevel() probe + web page HTML
#include "RoboCarHttp.h"     // HTTP message reader (never uses WiFiClient::available())
#include "RoboCarListen.h"   // non-blocking lwIP listener for the web server
#include "RoboCarMp3.h"      // bounded MP3 player (File::playMp3() can hang forever)

// ============================== User configuration ==============================
char ssid[] = "HITRON-DF90-2.4G";    // your network SSID (name)
char pass[] = "0972211921";          // your network password

const char *ollamaHost = "123.192.49.97";              // Ollama PC or Edge Gallery phone IP
const uint16_t ollamaPort = 11434;
String model = "gemma4:e2b";
String ollama_key = "ollama";  // API key (Edge Gallery), any text for Ollama

String ttsLang = "en-US";             // Google Translate TTS language
#define TTS_VOLUME 160                // MP3 digital volume (0..175)
#define MP3_STALL_MS 1000             // abort playback if the audio DMA stops for this long

// Describe your home: places, landmarks, and the routes between them.
// Gemma-4 uses this text together with the camera image to choose each move.
const char *HOME_MAP =
    "Places and landmarks:\n"
    "- Living-room: sofa, TV, coffee table, carpet.\n"
    "- Kitchen: fridge, sink, stove, cabinets, kitchen island.\n"
    "- Bedroom: bed, wardrobe\n"
    "- Hallway: long corridor, doors, shoe rack.\n"
    "Routes:\n"
    "- Living-room -> Kitchen: face the TV, turn left, go forward through the doorway, the kitchen is ahead.\n"
    "- Kitchen -> Living-room: turn around, go forward through the doorway, the sofa is ahead.\n"
    "- Kitchen -> Bedroom: leave the kitchen into the hallway, turn right, go forward to the end door.\n"
    "- Bedroom -> Kitchen: leave the bedroom into the hallway, go forward, turn left into the kitchen.\n"
    "- Living-room -> Bedroom: go to the hallway behind the sofa, turn right, go forward to the end door.\n";

// MPU6050 on Wire (I2C0, D3=SDA D4=SCL). 0 = do not touch I2C at all: no heading,
// L/R turns use TURN_FALLBACK_MS. Use 0 to test whether I2C hangs after the camera starts.
#define USE_IMU          1

// Movement tuning
#define MOVE_MS          1000     // forward / backward step length (README: 1 second per move)
#define TURN_STEP_DEG    45       // one L / R step turns this many degrees (IMU closed loop)
#define TURN_TIMEOUT_MS  2000     // turn safety limit
#define TURN_FALLBACK_MS 400      // turn time per step if the IMU is not available
#define SETTLE_MS        400      // pause after a move so the camera image is sharp
#define HEADING_WAIT_MS  200      // max wait for a fresh IMU packet when the heading is needed
#define NAV_MAX_STEPS    40       // give up navigation after this many steps
#define FIND_MAX_STEPS   30       // give up searching after this many looks

// Voice
#define RECORDING_SECONDS 3
#define TRIGGER_VOL       7000    // mic level that starts a recording (tune for your mic)
#define AUDIO_PRESET      0       // 0: 8kHz analog mic, 1: 16kHz analog, 2: 8kHz PDM, 3: 16kHz PDM

// Network timeouts
#define LLM_TIMEOUT_MS   120000UL
#define LLM_MAX_REPLY    32768UL  // bytes of JSON accepted from the server

// Serial: a line without CR/LF is taken after this pause (0 = require Enter).
// Use 0 for terminals that send each keystroke (minicom, screen) if you type slowly.
#define SERIAL_IDLE_SUBMIT_MS 500

// Web server
#define WEB_PORT         80
#define WEB_SLOTS        3        // accepted sockets waiting for their request
#define WEB_IDLE_MS      5000UL   // close a connection that sends nothing (browser preconnect)
#define WEB_REQ_MS       1000UL   // whole request line + headers must arrive within this
#define AUTO_SCENE_MS    0        // >0: refresh place/scene on the web page every N ms while idle

// DRV8833 (nSLEEP tied to Vcc, nFAULT floating)
#define IN1_PIN  7    // IN1
#define IN2_PIN 11    // IN2
#define IN3_PIN  8    // IN3 (needs 10K pull-up)
#define IN4_PIN 12    // IN4
// ================================================================================

#define CHANNEL       0
#define CAM_SETTLE_MS 1000      // camera auto-exposure time after a resume
#define CLIP_BASE     "Clip"
#define TTS_FILE      "tts.mp3"

// ------------------------------ types (before any function) ------------------------------
struct Action {
    String cmd;    // F B L R S GOTO FIND STATUS SCENE LISTEN NONE
    int n;         // number of steps for F/B/L/R
    String arg;    // destination for GOTO, object for FIND
    String say;    // text to speak
};

// ------------------------------ globals ------------------------------
int webListener = -1;            // lwIP listening socket (see RoboCarListen.h)
int webSock[WEB_SLOTS];          // accepted, request not read yet
uint32_t webSince[WEB_SLOTS];
AmebaFatFS fs;
bool sdReady = false;
bool camRunning = false;         // camera channel streaming (paused while speaking)
uint32_t camResumedAt = 0;

VideoSetting camConfig(768, 768, CAM_FPS, VIDEO_JPEG, 1);

AudioSetting configA(AUDIO_PRESET);
RoboAudio audio;                 // Audio + codec release/reset (RoboCar_Gemma4.h)
AAC aac;
MP4Recording mp4;
StreamIO audioStreamer(1, 1);    // audio -> audio (speaker muted), keeps micLevel() updated
StreamIO audioToAAC(1, 1);       // audio -> AAC
StreamIO aacToMp4(1, 1);         // AAC -> MP4 (audio only, no VOE)
bool audioStarted = false;       // audio.begin() done (call it only once)
bool codecReleased = false;      // codec handed to File::playMp3()
bool listening = false;
bool voiceTrigger = false;       // true when Audio::micLevel() exists
int clipCounter = 0;
char clipName[32];
uint32_t quietUntil = 0;         // ignore mic triggers until this time

MPU6050 mpu;
bool dmpReady = false;
uint8_t fifoBuffer[128];
Quaternion q;
VectorFloat gravity;
float ypr[3];
float yawDeg = 0;                // -180..180
String imuState = "not started"; // shown on the web page
int imuResets = 0;               // completed "imu reset" commands (web page waits for this to change)

// RoboCar status (shown on web page, spoken by TTS)
String motionState = "STOP";
String taskState = "IDLE";
String currentPlace = "unknown";
String lastScene = "";
String lastHeard = "";
String lastReply = "";

// Command input
String pendingCmd = "";
String pendingSrc = "";
volatile bool abortTask = false;
String serialLine = "";
uint32_t lastAutoScene = 0;
uint32_t llmDot = 0;             // last "." printed while waiting for Gemma

// ============================== Motors (DRV8833) ==============================
void cmd_F() { digitalWrite(IN1_PIN, HIGH); digitalWrite(IN2_PIN, LOW);  digitalWrite(IN3_PIN, LOW);  digitalWrite(IN4_PIN, HIGH); }
void cmd_B() { digitalWrite(IN1_PIN, LOW);  digitalWrite(IN2_PIN, HIGH); digitalWrite(IN3_PIN, HIGH); digitalWrite(IN4_PIN, LOW);  }
void cmd_R() { digitalWrite(IN1_PIN, HIGH); digitalWrite(IN2_PIN, LOW);  digitalWrite(IN3_PIN, HIGH); digitalWrite(IN4_PIN, LOW);  }
void cmd_L() { digitalWrite(IN1_PIN, LOW);  digitalWrite(IN2_PIN, HIGH); digitalWrite(IN3_PIN, LOW);  digitalWrite(IN4_PIN, HIGH); }
void cmd_S() { digitalWrite(IN1_PIN, HIGH); digitalWrite(IN2_PIN, HIGH); digitalWrite(IN3_PIN, HIGH); digitalWrite(IN4_PIN, HIGH); }

void motorsStop()
{
    cmd_S();
    motionState = "STOP";
}

// ============================== IMU (MPU6050 DMP) ==============================
void init_imu()
{
    Serial.println("Initializing MPU6050...");
    mpu.initialize();
    // Like WebServer_RoboCar_DRV8833_IMU, continue to dmpInitialize() even if this fails:
    // MPU6050 clones report another WHO_AM_I but the DMP still works.
    // Device ID 0x34 = genuine MPU6050; 0x0 or 0x3F usually means no I2C answer (wiring).
    Serial.println(String(mpu.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed") +
                   " (device ID 0x" + String(mpu.getDeviceID(), HEX) + ")");

    uint8_t devStatus = mpu.dmpInitialize();
    mpu.setXGyroOffset(51);
    mpu.setYGyroOffset(8);
    mpu.setZGyroOffset(21);
    mpu.setXAccelOffset(1150);
    mpu.setYAccelOffset(-50);
    mpu.setZAccelOffset(1060);

    if (devStatus == 0) {
        mpu.CalibrateAccel(6);    // keep the car still and level during calibration
        mpu.CalibrateGyro(6);
        Serial.println();
        mpu.PrintActiveOffsets();
        mpu.setDMPEnabled(true);
        mpu.getIntStatus();
        dmpReady = true;
        Serial.println("DMP ready");
    } else {
        Serial.print("DMP initialization failed (code ");
        Serial.print(devStatus);
        Serial.println(devStatus == 1 ? ": initial memory load failed)" :
                       devStatus == 2 ? ": DMP configuration updates failed)" : ")");
    }
}

// Bounded replacement for mpu.dmpGetCurrentFIFOPacket(). The library version has an
// inner "while (getFIFOCount() > length)" loop with no timeout, and Wire.requestFrom()
// on this core ignores I2C errors and returns the stale rxBuffer. One I2C glitch (motor
// noise, bus busy after camera/audio start) can make getFIFOCount() return a stale value
// > 28 forever -> loop() hangs and serial / web commands are never read again.
bool readLatestDmpPacket()
{
    const uint16_t len = mpu.dmpGetFIFOPacketSize();    // 28 for MotionApps612
    uint16_t n = mpu.getFIFOCount();
    if (n < len) return false;                           // no complete packet yet
    if (n > 200 || n % len) {                            // overflow or bad read: start over
        mpu.resetFIFO();
        return false;
    }
    while (n > len) {                                    // at most 7 packets: drop the old ones
        mpu.getFIFOBytes(fifoBuffer, len);
        n -= len;
    }
    mpu.getFIFOBytes(fifoBuffer, len);                   // the newest packet
    return true;
}

// Read the latest DMP packet (if any) and update yawDeg.
// Returns true when a new packet was read. Only called where the heading is needed
// (status, turns, navigation prompt, IMU init); the DMP keeps integrating the yaw
// on its own in between, so loop() never touches I2C.
bool updateIMU()
{
    if (!dmpReady) return false;
    if (readLatestDmpPacket()) {
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        yawDeg = ypr[0] * 180.0 / M_PI;
        return true;
    }
    return false;
}

// On-demand heading: the FIFO has overflowed since the last read (it fills in ~0.4 s),
// so the first updateIMU() resets it; wait for the next fresh packet (DMP ~100 Hz).
bool readHeading(uint32_t timeoutMs)
{
    if (!dmpReady) return false;
    uint32_t t0 = millis();
    while (millis() - t0 < timeoutMs) {
        if (updateIMU()) return true;
        delay(5);
    }
    Serial.println("[IMU] no fresh heading, using the last one");
    return false;
}

// Initialize the MPU6050 + DMP (up to 3 attempts) and read the first heading.
// The car must stand still and level: init_imu() calibrates the accel and gyro.
bool startIMU()
{
#if USE_IMU
    dmpReady = false;
    imuState = "initializing";
    for (int attempt = 1; attempt <= 3 && !dmpReady; attempt++) {
        if (attempt > 1) {
            Serial.println("[IMU] retry " + String(attempt) + " of 3");
            delay(500);
        }
        init_imu();
    }
    if (!dmpReady) {
        imuState = "FAILED";
        Serial.println("[IMU] FAILED: no heading; L/R turns are timed (check MPU6050 wiring SDA=D3, SCL=D4, VCC, GND)");
        return false;
    }
    yawDeg = 0;
    bool got = readHeading(1000);
    imuState = got ? "OK" : "OK (no DMP data yet)";
    Serial.println("[IMU] " + imuState + ", heading " + String(heading()));
    return true;
#else
    imuState = "disabled (USE_IMU 0)";
    Serial.println("IMU disabled (USE_IMU 0): no heading, turns are timed");
    return false;
#endif
}

// "imu reset": re-initialize the MPU6050, then read and show the heading.
void resetIMU()
{
    taskState = "IMU RESET";
    motorsStop();
    Serial.println("[IMU] reset: keep the car still and level during calibration");
    startIMU();
    imuResets++;
}

// Heading 0..359 for display / prompts
int heading()
{
    int h = (int)(yawDeg + 180.0);
    if (h >= 360) h -= 360;
    if (h < 0) h += 360;
    return h;
}

float angleDiff(float a, float b)
{
    float d = a - b;
    while (d > 180) d -= 360;
    while (d < -180) d += 360;
    return d;
}

// ============================== Movement steps ==============================
// Wait while polling inputs; returns false if a new command aborted the task.
bool waitMs(uint32_t ms)
{
    uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        pollInputs();
        if (abortTask) return false;
        delay(10);
    }
    return true;
}

void turnStep(bool right)
{
    motionState = right ? "RIGHT" : "LEFT";
    readHeading(HEADING_WAIT_MS);    // fresh start angle (the IMU is not read between commands)
    float start = yawDeg;
    uint32_t t0 = millis();
    if (right) cmd_R(); else cmd_L();
    if (dmpReady) {
        while (millis() - t0 < TURN_TIMEOUT_MS) {
            updateIMU();
            if (fabs(angleDiff(yawDeg, start)) >= TURN_STEP_DEG - 5) break;
            pollSerial();    // serial only: keep the IMU loop tight (web is polled after the turn)
            if (abortTask) break;
            delay(5);
        }
    } else {
        waitMs(TURN_FALLBACK_MS);
    }
    motorsStop();
}

// One RoboCar step: move for 1 s (or turn 45 deg), then stop and settle.
void moveStep(char m)
{
    switch (m) {
        case 'F': motionState = "FORWARD";  cmd_F(); waitMs(MOVE_MS); break;
        case 'B': motionState = "BACKWARD"; cmd_B(); waitMs(MOVE_MS); break;
        case 'L': turnStep(false); break;
        case 'R': turnStep(true); break;
        default: break;
    }
    motorsStop();
    waitMs(SETTLE_MS);
    // Heading is only read during turns; F/B steps do not touch the IMU.
    if ((m == 'L' || m == 'R') && dmpReady) Serial.println("[Move] " + String(m) + "  heading=" + String(heading()));
    else Serial.println("[Move] " + String(m));
}

// ============================== LED ==============================
void flashLED(int times)
{
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
    }
}

// ============================== Helpers ==============================
String jsonQuote(const String &s)
{
    String out = "\"";
    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if ((uint8_t)c < 0x20) out += ' ';
        else out += c;
    }
    out += "\"";
    return out;
}

String urlEncode(const String &s)
{
    const char *hex = "0123456789ABCDEF";
    String out;
    for (unsigned int i = 0; i < s.length(); i++) {
        uint8_t c = (uint8_t)s[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += (char)c;
        } else {
            out += '%';
            out += hex[c >> 4];
            out += hex[c & 0xF];
        }
    }
    return out;
}

String urlDecode(const String &s)
{
    String out;
    for (unsigned int i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == '+') {
            out += ' ';
        } else if (c == '%' && i + 2 < s.length()) {
            char h[3] = {s[i + 1], s[i + 2], 0};
            out += (char)strtol(h, NULL, 16);
            i += 2;
        } else {
            out += c;
        }
    }
    return out;
}

// Gemma may wrap its JSON in ```json ... ``` or add text: keep the outermost {...}
String extractJson(const String &s)
{
    int a = s.indexOf('{');
    int b = s.lastIndexOf('}');
    if (a < 0 || b <= a) return "";
    return s.substring(a, b + 1);
}

bool jsonBool(JsonVariantConst v)
{
    if (v.is<bool>()) return v.as<bool>();
    if (v.is<const char *>()) return String(v.as<const char *>()).equalsIgnoreCase("true");
    if (v.is<int>()) return v.as<int>() != 0;
    return false;
}

String jsonStr(JsonVariantConst v)
{
    if (v.is<const char *>()) return String(v.as<const char *>());
    return "";
}

char moveChar(const String &m)
{
    if (m.length() == 0) return 'S';
    char c = toupper(m[0]);
    if (c == 'F' || c == 'B' || c == 'L' || c == 'R' || c == 'S') return c;
    return 'S';
}

// Send on the raw lwIP socket. Do NOT use WiFiClient::write() on the ideasHatch core:
// ServerDrv::sendData() is declared bool, so write() always reports 1 byte sent although
// the whole buffer went out. A write loop then resends almost everything again and the
// peer receives garbage (broken web pages, invalid JSON uploads to Ollama).
// (Fixed in the realtek AMB82 core, where sendData() returns ssize_t.)
bool writeAll(int sock, const uint8_t *data, size_t length)
{
    return roboSend(sock, data, length);
}

bool writeAll(int sock, const String &s)
{
    return roboSend(sock, (const uint8_t *)s.c_str(), s.length());
}

bool ensureWiFi()
{
    if (WiFi.status() == WL_CONNECTED) return true;
    Serial.println("[WiFi] reconnecting...");
    WiFi.begin(ssid, pass);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000UL) delay(100);
    return WiFi.status() == WL_CONNECTED;
}

String ipString()
{
    IPAddress ip = WiFi.localIP();
    return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

// ============================== Gemma-4 (OpenAI-compatible chat API) ==============================
// Body = prefix + data + suffix. "data" is a base64 blob (image or audio), or NULL for text only.
// Returns true and the assistant "content" on success.
bool postChat(const String &prefix, const char *data, size_t dataLen, const String &suffix, String &content)
{
    content = "";
    if (!ensureWiFi()) {
        Serial.println("[LLM] WiFi not connected");
        return false;
    }
    IPAddress serverIp;
    if (!WiFi.hostByName(ollamaHost, serverIp)) {
        Serial.println("[LLM] cannot resolve " + String(ollamaHost));
        return false;
    }
    // Raw lwIP socket for sending (see writeAll). 50 ms receive timeout: short socket
    // reads let llmIdle() keep the web page / serial responsive while Gemma thinks.
    int sock = roboConnect((uint32_t)serverIp, ollamaPort, 50, 15000);
    if (sock < 0) {
        Serial.println("[LLM] connection to server failed");
        return false;
    }
    WiFiClient llmClient((uint8_t)sock);    // used for reading only; closes the socket when done
    size_t total = prefix.length() + dataLen + suffix.length();
    // HTTP/1.0 + close -> unchunked response
    String head = "POST /v1/chat/completions HTTP/1.0\r\n"
                  "Host: " + String(ollamaHost) + ":" + String(ollamaPort) + "\r\n"
                  "Authorization: Bearer " + ollama_key + "\r\n"
                  "Content-Type: application/json; charset=utf-8\r\n"
                  "Content-Length: " + String(total) + "\r\n"
                  "Connection: close\r\n\r\n";

    bool sent = writeAll(sock, head) && writeAll(sock, prefix);
    if (sent && data && dataLen) sent = writeAll(sock, (const uint8_t *)data, dataLen);
    if (sent) sent = writeAll(sock, suffix);
    if (!sent) {
        Serial.println("[LLM] upload failed");
        llmClient.stop();
        return false;
    }

    // Read the response with RoboHttpReader: WiFiClient::available() spins forever on a
    // blocking socket while Gemma is still thinking.
    Serial.print("[LLM] waiting");
    llmDot = millis();
    uint32_t t0 = millis();
    RoboHttpReader reader(llmClient, LLM_TIMEOUT_MS, llmIdle);
    String first, body;
    bool complete = reader.headers(first) && reader.text(body, LLM_MAX_REPLY);
    llmClient.stop();
    Serial.println(" " + String((millis() - t0) / 1000.0, 1) + " s");

    if (reader.cancelled) {
        Serial.println("[LLM] aborted by new command");
        return false;
    }
    if (!complete) {
        Serial.println(reader.timedOut ? "[LLM] response timeout" : "[LLM] response incomplete or too large");
        return false;
    }
    int httpStatus = reader.status;
    if (httpStatus != 200) {
        Serial.println("[LLM] HTTP status " + String(httpStatus));
        Serial.println(body);
        return false;
    }
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
        Serial.println("[LLM] invalid JSON: " + String(err.c_str()));
        return false;
    }
    if (!doc["choices"][0]["message"]["content"].is<const char *>()) {
        Serial.println("[LLM] no content: " + body);
        return false;
    }
    content = doc["choices"][0]["message"]["content"].as<const char *>();
    content.trim();
    lastReply = content;
    Serial.println("[Gemma] " + content);
    return true;
}

// Runs while postChat() waits for the reply; returning false cancels the request.
bool llmIdle()
{
    pollInputs();    // keep web page / serial responsive while Gemma thinks
    if (millis() - llmDot > 1000) {
        Serial.print(".");
        llmDot = millis();
    }
    return !abortTask;
}

String chatHead(const String &prompt)
{
    return String("{\"model\":") + jsonQuote(model) +
           ",\"stream\":false,\"messages\":[{\"role\":\"user\",\"content\":[{\"type\":\"text\",\"text\":" +
           jsonQuote(prompt) + "}";
}

bool askText(const String &prompt, String &content)
{
    return postChat(chatHead(prompt), NULL, 0, String("]}]}"), content);
}

// Capture a camera frame and ask Gemma-4 about it.
// ============================== Camera ==============================
// The camera is paused during MP3 playback: with the 768x768 JPEG channel streaming,
// the audio playback is chopped and hangs (ILI9341_Giraffe, which works, never plays
// MP3 while its camera runs).
void cameraPause()
{
    if (!camRunning) return;
    Camera.channelEnd(CHANNEL);
    camRunning = false;
}

void cameraResume()
{
    if (camRunning) return;
    Camera.channelBegin(CHANNEL);
    camRunning = true;
    camResumedAt = millis();
}

// Resume the camera and wait until its exposure has settled; false if a new command aborted.
bool cameraReady()
{
    cameraResume();
    while (millis() - camResumedAt < CAM_SETTLE_MS) {
        pollInputs();
        if (abortTask) return false;
        delay(10);
    }
    return true;
}

bool askImage(const String &prompt, String &content)
{
    if (!cameraReady()) return false;
    uint32_t img_addr = 0, img_len = 0;
    Camera.getImage(CHANNEL, &img_addr, &img_len);
    if (!img_addr || !img_len) {
        Serial.println("[Camera] capture failed");
        return false;
    }
    int b64Len = base64_enc_len(img_len);
    char *b64 = (char *)malloc(b64Len + 1);
    if (!b64) {
        Serial.println("[Camera] not enough memory for base64 image");
        return false;
    }
    base64_encode(b64, (char *)img_addr, img_len);    // encode right away, before the frame changes
    Serial.println("[Camera] image " + String(img_len) + " bytes");

    String prefix = chatHead(prompt) + ",{\"type\":\"image_url\",\"image_url\":{\"url\":\"data:image/jpeg;base64,";
    bool ok = postChat(prefix, b64, strlen(b64), String("\"}}]}]}"), content);
    free(b64);
    return ok;
}

// Send the last recorded clip (MP4 / AAC) with a prompt.
bool askAudio(const String &prompt, String &content)
{
    if (!sdReady) return false;
    String path = String(fs.getRootPath()) + clipName + ".mp4";
    if (!fs.exists(path)) {
        Serial.println("[Voice] clip not found: " + path);
        return false;
    }
    File file = fs.open(path);
    uint32_t size = file ? file.size() : 0;
    if (!size || size > 1024UL * 1024UL) {
        Serial.println("[Voice] clip empty or too big");
        file.close();
        return false;
    }
    char *raw = (char *)malloc(size);
    if (!raw) { file.close(); return false; }
    int got = file.read(raw, size);
    file.close();
    if (got != (int)size) { free(raw); return false; }

    int b64Len = base64_enc_len(size);
    char *b64 = (char *)malloc(b64Len + 1);
    if (!b64) { free(raw); return false; }
    base64_encode(b64, raw, size);
    free(raw);

    String prefix = chatHead(prompt) + ",{\"type\":\"input_audio\",\"input_audio\":{\"data\":\"";
    bool ok = postChat(prefix, b64, strlen(b64), String("\",\"format\":\"mp4\"}}]}]}"), content);
    free(b64);
    fs.remove(path);    // keep the SD card clean; NEVER fs.end() (breaks the next recording)
    return ok;
}

// ============================== Prompts (README item 6: output format for RoboCar) ==============================
String placeList()
{
    return "Living-room, Kitchen, Bedroom, Hallway";
}

String commandFormat()
{
    return String(
        "You control a small two-wheel robot car named RoboCar. Convert the user's request into a RoboCar command.\n"
        "Reply with ONE line of JSON only, no markdown, no explanation:\n"
        "{\"cmd\":\"F|B|L|R|S|GOTO|FIND|STATUS|SCENE|IMURESET|NONE\",\"n\":1,\"arg\":\"\",\"say\":\"\"}\n"
        "cmd meanings: F=move forward, B=move backward, L=turn left, R=turn right, S=stop, "
        "GOTO=navigate to a place (arg = place name, one of: ") + placeList() + "), "
        "FIND=look around for an object or animal (arg = the object, e.g. dog), "
        "STATUS=report the robot's heading angle, SCENE=look with the camera and tell where the robot is and what it sees, "
        "IMURESET=reset / recalibrate the IMU (heading sensor), "
        "NONE=not a robot command (answer in say).\n"
        "n = number of steps for F/B/L/R (1 to 5, one step = 1 second or 45 degrees).\n"
        "say = a short friendly sentence the robot will speak.\n";
}

String navPrompt(const String &dest, int step, const String &history)
{
    return String("You are the navigator of a small two-wheel robot car. The image is its front camera view, about 10 cm above the floor.\n") +
           HOME_MAP +
           "Goal: go to the " + dest + ".\n"
           "Last recognized place: " + currentPlace + ". Heading: " + String(heading()) + " degrees. Step " + String(step) +
           " of " + String(NAV_MAX_STEPS) + ". Previous moves (oldest first): " + (history.length() ? history : String("none")) + ".\n"
           "Recognize the current place from the image, then choose the next single move toward the goal using the routes.\n"
           "Moves: F=forward 1 second (about 30 cm), B=backward 1 second, L=turn left 45 degrees, R=turn right 45 degrees, S=stop because the goal is reached.\n"
           "If an obstacle or wall is right in front, turn L or R instead of F.\n"
           "Reply with ONE line of JSON only, no markdown:\n"
           "{\"place\":\"<current place>\",\"arrived\":false,\"move\":\"F\",\"say\":\"<few words about what you see>\"}";
}

String findPrompt(const String &target)
{
    return String("The image is the front camera view of a small robot car, about 10 cm above the floor.\n"
                  "Is there a ") + target + " visible in the image?\n"
           "Reply with ONE line of JSON only, no markdown:\n"
           "{\"found\":false,\"where\":\"left|center|right|none\",\"clear\":true,\"scene\":\"<few words>\"}\n"
           "found = true only if a " + target + " is clearly visible. where = its position in the image. "
           "clear = true if the floor straight ahead is free to drive forward about 30 cm.";
}

String scenePrompt()
{
    return String("The image is the front camera view of a small robot car in a home.\n") + HOME_MAP +
           "Which place is it, and what does the robot see?\n"
           "Reply with ONE line of JSON only, no markdown:\n"
           "{\"place\":\"<one of: " + placeList() + ", or unknown>\",\"scene\":\"<one short sentence>\"}";
}

// ============================== TTS (Google Translate) + MP3 playback ==============================
String readLineTimed(WiFiSSLClient &c, uint32_t timeout)
{
    String s;
    uint32_t t0 = millis();
    while (millis() - t0 < timeout) {
        if (c.available()) {
            char ch = (char)c.read();
            if (ch == '\n') return s;
            if (ch != '\r' && s.length() < 512) s += ch;
            t0 = millis();
        } else if (!c.connected()) {
            break;
        } else {
            delay(1);
        }
    }
    return s;
}

// Copy up to n body bytes to the file (n < 0 = until the connection closes).
uint32_t copyToFile(WiFiSSLClient &c, File &f, long n)
{
    uint8_t buf[1024];
    uint32_t total = 0;
    uint32_t t0 = millis();
    while ((n < 0 || (long)total < n) && millis() - t0 < 10000UL) {
        int avail = c.available();
        if (avail > 0) {
            int want = avail > (int)sizeof(buf) ? (int)sizeof(buf) : avail;
            if (n >= 0 && want > n - (long)total) want = n - total;
            int got = c.read(buf, want);
            if (got > 0) {
                f.write(buf, got);
                total += got;
                t0 = millis();
            }
        } else if (!c.connected()) {
            break;
        } else {
            delay(1);
        }
    }
    return total;
}

// Download speech for one short text (< 200 chars) to TTS_FILE. Keeps the SD mounted
// (GenAI::googletts() calls fs.end(), which breaks later MP4 recordings).
bool ttsDownload(const String &text)
{
    if (!sdReady || !ensureWiFi()) return false;
    String path = String(fs.getRootPath()) + TTS_FILE;
    if (fs.exists(path)) fs.remove(path);
    File f = fs.open(path);
    if (!f) {
        Serial.println("[TTS] cannot create " + path);
        return false;
    }
    WiFiSSLClient tls;
    bool ok = false;
    if (tls.connect("translate.google.com", 443)) {
        tls.println("GET /translate_tts?ie=UTF-8&client=tw-ob&tl=" + ttsLang + "&ttsspeed=1&q=" + urlEncode(text) + " HTTP/1.0");
        tls.println("Host: translate.google.com");
        tls.println("User-Agent: Mozilla/5.0");
        tls.println("Connection: close");
        tls.println();

        String status = readLineTimed(tls, 10000);
        bool chunked = false;
        long contentLength = -1;
        while (true) {
            String h = readLineTimed(tls, 10000);
            if (h.length() == 0) break;
            String hl = h;
            hl.toLowerCase();
            if (hl.startsWith("transfer-encoding:") && hl.indexOf("chunked") > 0) chunked = true;
            if (hl.startsWith("content-length:")) contentLength = hl.substring(15).toInt();
        }
        uint32_t total = 0;
        if (status.indexOf(" 200") > 0) {
            if (chunked) {
                while (true) {
                    String sz = readLineTimed(tls, 10000);
                    long n = strtol(sz.c_str(), NULL, 16);
                    if (n <= 0) break;
                    total += copyToFile(tls, f, n);
                    readLineTimed(tls, 10000);    // CRLF after chunk
                }
            } else {
                total = copyToFile(tls, f, contentLength);
            }
            ok = total > 0;
            Serial.println("[TTS] " + String(total) + " bytes" + (contentLength >= 0 && (long)total != contentLength ? " of " + String(contentLength) + " (TRUNCATED)" : String("")));
        } else {
            Serial.println("[TTS] " + status);
        }
        tls.stop();
    } else {
        Serial.println("[TTS] connect to translate.google.com failed");
    }
    f.close();
    return ok;
}

// Plays TTS_FILE with roboPlayMp3() instead of File::playMp3(): the core player blocks
// forever in xQueueSend() if the audio DMA stops (chopped speech, then a dead loop()).
void playTTSFile()
{
    String path = String(fs.getRootPath()) + TTS_FILE;
    File f = fs.open(path);
    if (!f) return;
    uint32_t size = f.size();
    uint8_t *mp3 = (size && size <= 512UL * 1024UL) ? (uint8_t *)malloc(size) : NULL;
    int got = mp3 ? f.read(mp3, size) : 0;
    f.close();
    if (!mp3 || got != (int)size) {
        Serial.println("[MP3] cannot load " + path + " (" + String(size) + " bytes)");
        free(mp3);
        return;
    }
    RoboMp3Result r = roboPlayMp3(mp3, size, TTS_VOLUME, MP3_STALL_MS);
    free(mp3);
    if (r.noMemory) Serial.println("[MP3] out of memory");
    else Serial.println("[MP3] " + String(r.frames) + " frames, " + String(r.sampleRate) + " Hz, DMA irqs " +
                        String(r.txIrqs) + (r.stalled ? "  STALLED: audio DMA stopped, playback aborted" : ""));
}

// Give the codec to the mic pipeline: begin() the first time, reset after MP3 playback.
void audioBegin()
{
    if (!audioStarted) {
        audio.begin();
        audio.muteSpk(1);    // no mic->speaker loopback noise (kept across codec resets)
        audioStarted = true;
    } else if (codecReleased) {
        audio.resetCodec();
    }
    codecReleased = false;
}

// Hand the codec to roboPlayMp3() (RoboCarMp3.c), which initializes it itself.
void audioEnd()
{
    stopListening();
    if (audioStarted && !codecReleased) {
        audio.releaseCodec();
        codecReleased = true;
    }
}

// Speak a sentence (split into <=180 character pieces for Google TTS).
void speak(String text)
{
    text.trim();
    if (text.length() == 0) return;
    Serial.println("[Say] " + text);
    if (!sdReady) return;
    audioEnd();    // MP3 playback drives the audio codec directly (RoboCarMp3.c)
    cameraPause();    // no camera streaming during MP3 playback (see cameraPause())
    while (text.length() > 0) {
        String part = text;
        if (part.length() > 180) {
            int cut = part.lastIndexOf(' ', 180);
            if (cut < 40) cut = 180;
            part = text.substring(0, cut);
            text = text.substring(cut + 1);
        } else {
            text = "";
        }
        if (ttsDownload(part)) playTTSFile();
    }
    cameraResume();
    // The codec stays released: recordClip() / startListening() take it back when needed.
    quietUntil = millis() + 1500;
}

// ============================== Voice (mic -> MP4 audio clip) ==============================
void startListening()
{
    if (!voiceTrigger || listening) return;
    audioBegin();
    if (audioStreamer.begin() == 0) listening = true;
}

void stopListening()
{
    if (listening) {
        audioStreamer.end();
        listening = false;
    }
}

bool recordClip()
{
    if (!sdReady) {
        Serial.println("[Voice] SD card is required");
        return false;
    }
    stopListening();
    audioBegin();
    clipCounter++;
    snprintf(clipName, sizeof(clipName), "%s%d", CLIP_BASE, clipCounter);    // unique name: muxer can't overwrite
    mp4.setRecordingFileName(clipName);

    Serial.println("[Voice] recording " + String(RECORDING_SECONDS) + " s ...");
    digitalWrite(LED_BUILTIN, HIGH);
    aac.begin();
    audioToAAC.begin();
    aacToMp4.begin();
    mp4.begin();

    uint32_t t0 = millis();
    while (!mp4.getRecordingState() && millis() - t0 < 2000UL) delay(10);
    bool started = mp4.getRecordingState() != 0;
    while (mp4.getRecordingState() && millis() - t0 < (RECORDING_SECONDS + 10UL) * 1000UL) delay(100);
    bool complete = started && !mp4.getRecordingState();

    mp4.end();
    aacToMp4.end();
    audioToAAC.end();
    aac.end();
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println(complete ? "[Voice] recording done" : "[Voice] recording failed");
    return complete;
}

void voiceCommand()
{
    taskState = "LISTENING";
    if (!recordClip()) {
        taskState = "IDLE";
        return;
    }
    taskState = "UNDERSTANDING VOICE";
    String prompt = commandFormat() +
                    "The user's request is the speech in the audio. Also add the field \"heard\" with the exact transcription, "
                    "e.g. {\"heard\":\"go to the kitchen\",\"cmd\":\"GOTO\",\"n\":1,\"arg\":\"Kitchen\",\"say\":\"Going to the kitchen\"}";
    String reply;
    if (!askAudio(prompt, reply)) {
        if (!abortTask) speak("Sorry, I could not understand.");
        taskState = "IDLE";
        return;
    }
    Action a;
    if (!parseAction(reply, a)) {
        speak("Sorry, I did not get that.");
        taskState = "IDLE";
        return;
    }
    executeAction(a);
}

// ============================== Command handling ==============================
bool parseAction(const String &reply, Action &a)
{
    JsonDocument d;
    String js = extractJson(reply);
    if (js.length() == 0 || deserializeJson(d, js)) {
        Serial.println("[Cmd] cannot parse: " + reply);
        return false;
    }
    a.cmd = jsonStr(d["cmd"]);
    a.cmd.toUpperCase();
    a.n = d["n"] | 1;
    if (a.n < 1) a.n = 1;
    if (a.n > 5) a.n = 5;
    a.arg = jsonStr(d["arg"]);
    a.say = jsonStr(d["say"]);
    String heard = jsonStr(d["heard"]);
    if (heard.length()) {
        lastHeard = heard;
        Serial.println("[Heard] " + heard);
    }
    if (a.cmd == "FORWARD") a.cmd = "F";
    if (a.cmd == "BACKWARD") a.cmd = "B";
    if (a.cmd == "LEFT") a.cmd = "L";
    if (a.cmd == "RIGHT") a.cmd = "R";
    if (a.cmd == "STOP") a.cmd = "S";
    return a.cmd.length() > 0;
}

// Fast local parsing for simple commands (works even when the LLM server is down).
bool parseLocal(String t, Action &a)
{
    t.trim();
    String l = t;
    l.toLowerCase();
    a.n = 1;
    a.arg = "";
    a.say = "";
    if (l.length() == 1 && String("fblrsv").indexOf(l[0]) >= 0) {
        a.cmd = (l[0] == 'v') ? String("LISTEN") : String((char)toupper(l[0]));
        return true;
    }
    if (l == "stop") { a.cmd = "S"; return true; }
    if (l == "status" || l == "heading") { a.cmd = "STATUS"; return true; }
    if (l == "scene" || l == "look" || l == "where am i") { a.cmd = "SCENE"; return true; }
    if (l == "imu reset" || l == "reset imu" || l == "imu") { a.cmd = "IMURESET"; return true; }
    if (l == "listen" || l == "voice") { a.cmd = "LISTEN"; return true; }
    if (l.startsWith("go to ") || l.startsWith("goto ")) {
        a.cmd = "GOTO";
        a.arg = t.substring(l.startsWith("go to ") ? 6 : 5);
        a.arg.trim();
        String al = a.arg;
        al.toLowerCase();
        if (al.startsWith("the ")) a.arg = a.arg.substring(4);
        return a.arg.length() > 0;
    }
    if (l.startsWith("find ")) {
        a.cmd = "FIND";
        a.arg = t.substring(5);
        a.arg.trim();
        String al = a.arg;
        al.toLowerCase();
        if (al.startsWith("the ")) a.arg = a.arg.substring(4);
        return a.arg.length() > 0;
    }
    return false;
}

void handleCommand(const String &text, const String &src)
{
    Serial.println("[Cmd] (" + src + ") " + text);
    lastHeard = text;
    Action a;
    if (!parseLocal(text, a)) {    // not a simple command: let Gemma-4 translate it
        taskState = "UNDERSTANDING";
        String reply;
        if (!askText(commandFormat() + "User request: " + text, reply) || !parseAction(reply, a)) {
            if (!abortTask) speak("Sorry, I could not understand the command.");
            taskState = "IDLE";
            return;
        }
    }
    executeAction(a);    // simple F/B/L/R/S from web or serial have no "say": they move quietly
}

void executeAction(Action &a)
{
    Serial.println("[Action] cmd=" + a.cmd + " n=" + String(a.n) + " arg=" + a.arg);
    if (a.cmd == "F" || a.cmd == "B" || a.cmd == "L" || a.cmd == "R") {
        taskState = "MOVE " + a.cmd;
        if (a.say.length()) speak(a.say);
        for (int i = 0; i < a.n && !abortTask; i++) moveStep(a.cmd[0]);
    } else if (a.cmd == "S") {
        motorsStop();
        if (a.say.length()) speak(a.say);
    } else if (a.cmd == "GOTO") {
        navigateTo(a.arg);
    } else if (a.cmd == "FIND") {
        findObject(a.arg);
    } else if (a.cmd == "STATUS") {
        reportStatus();
    } else if (a.cmd == "IMURESET") {
        resetIMU();
    } else if (a.cmd == "SCENE") {
        reportScene();
    } else if (a.cmd == "LISTEN") {
        voiceCommand();
        return;
    } else {
        if (a.say.length()) speak(a.say);
    }
    taskState = "IDLE";
}

// ============================== Status (README items 2 & 3) ==============================
bool recognizeScene()
{
    String reply;
    if (!askImage(scenePrompt(), reply)) return false;
    JsonDocument d;
    if (deserializeJson(d, extractJson(reply))) return false;
    String p = jsonStr(d["place"]);
    if (p.length()) currentPlace = p;
    lastScene = jsonStr(d["scene"]);
    return true;
}

// "status": heading angle only (no camera, no server needed)
void reportStatus()
{
    taskState = "STATUS";
    readHeading(HEADING_WAIT_MS);
    if (dmpReady) speak("My heading is " + String(heading()) + " degrees.");
    else speak("My heading is unknown, the IMU is not available.");
}

// "scene": capture an image and let Gemma-4 recognize the place and scene
void reportScene()
{
    taskState = "SCENE";
    if (!recognizeScene()) {
        if (!abortTask) speak("Sorry, I could not recognize the scene.");
        return;
    }
    String msg;
    if (currentPlace.length() && !currentPlace.equalsIgnoreCase("unknown")) msg = "I am in the " + currentPlace + ".";
    if (lastScene.length()) msg += (msg.length() ? " " : "") + lastScene;
    speak(msg.length() ? msg : String("I cannot tell where I am."));
}

// ============================== Navigation (README items 5 & 7) ==============================
void navigateTo(String dest)
{
    dest.trim();
    taskState = "GOTO " + dest;
    speak("Going to the " + dest + ".");
    String history = "";
    int failures = 0;

    for (int step = 1; step <= NAV_MAX_STEPS; step++) {
        if (abortTask) { motorsStop(); return; }
        readHeading(HEADING_WAIT_MS);    // the navigation prompt tells Gemma-4 the heading
        String reply;
        if (!askImage(navPrompt(dest, step, history), reply)) {
            if (abortTask) return;
            if (++failures >= 3) { speak("I lost contact with my brain server."); return; }
            continue;
        }
        JsonDocument d;
        if (deserializeJson(d, extractJson(reply))) {
            if (++failures >= 3) { speak("I could not understand the navigator."); return; }
            continue;
        }
        failures = 0;
        String place = jsonStr(d["place"]);
        if (place.length()) currentPlace = place;
        lastScene = jsonStr(d["say"]);
        bool arrived = jsonBool(d["arrived"]);
        char m = moveChar(jsonStr(d["move"]));

        if (arrived || m == 'S' || currentPlace.equalsIgnoreCase(dest) || currentPlace.equalsIgnoreCase("the " + dest)) {
            motorsStop();
            currentPlace = dest;
            speak("I arrived at the " + dest + ".");
            return;
        }
        moveStep(m);
        history += m;
        if (history.length() > 10) history = history.substring(history.length() - 10);
    }
    motorsStop();
    speak("Sorry, I could not reach the " + dest + ".");
}

// ============================== Find object (README item 8) ==============================
void findObject(String target)
{
    target.trim();
    taskState = "FIND " + target;
    speak("Looking for the " + target + ".");
    int turns = 0;
    const int turnsPerCircle = 360 / TURN_STEP_DEG;
    int failures = 0;

    for (int look = 1; look <= FIND_MAX_STEPS; look++) {
        if (abortTask) { motorsStop(); return; }
        String reply;
        if (!askImage(findPrompt(target), reply)) {
            if (abortTask) return;
            if (++failures >= 3) { speak("I lost contact with my brain server."); return; }
            continue;
        }
        JsonDocument d;
        if (deserializeJson(d, extractJson(reply))) {
            if (++failures >= 3) return;
            continue;
        }
        failures = 0;
        lastScene = jsonStr(d["scene"]);
        if (jsonBool(d["found"])) {
            motorsStop();    // README: stop the RoboCar when the target is found
            String where = jsonStr(d["where"]);
            speak("I found the " + target + (where.length() && where != "none" && where != "center" ? " on the " + where : String("")) + "!");
            return;
        }
        if (turns < turnsPerCircle) {
            moveStep('R');    // look around in 45 degree steps
            turns++;
        } else if (jsonBool(d["clear"])) {
            moveStep('F');    // full circle done: move to a new spot and look around again
            moveStep('F');
            turns = 0;
        } else {
            moveStep('R');
        }
    }
    motorsStop();
    speak("Sorry, I could not find the " + target + ".");
}

// ============================== Inputs: Serial + Web ==============================
void queueCommand(const String &cmd, const String &src)
{
    String c = cmd;
    c.trim();
    if (c.length() == 0) return;
    String l = c;
    l.toLowerCase();
    if (l == "s" || l == "stop") {
        motorsStop();    // stop immediately, even in the middle of a task
        pendingCmd = "";
    }
    if (taskState != "IDLE") abortTask = true;    // a new command interrupts the running task
    if (l != "s" && l != "stop") {
        pendingCmd = c;
        pendingSrc = src;
    } else if (taskState == "IDLE") {
        pendingCmd = c;
        pendingSrc = src;
    }
}

void printHelp()
{
    Serial.println("Serial commands (115200 baud, any line ending):\n"
                   "  F B L R S        forward / backward / turn left / turn right (1 step) / stop\n"
                   "  V | listen       record a 3 s voice command\n"
                   "  status           speak the heading angle\n"
                   "  scene | look     capture an image, recognize the place + scene\n"
                   "  imu reset        re-initialize + calibrate the MPU6050, read the heading\n"
                   "  go to <place>    e.g. go to kitchen\n"
                   "  find <object>    e.g. find the dog\n"
                   "  help | ?         this list\n"
                   "  anything else is sent to Gemma-4, e.g. turn left twice");
}

void submitSerialLine()
{
    String line = serialLine;
    serialLine = "";
    line.trim();
    if (!line.length()) return;
    // Echo at once, so it is visible that the line arrived even while a task runs.
    Serial.println("[Serial] " + line + (taskState != "IDLE" ? "   (interrupts: " + taskState + ")" : String("")));
    if (line == "?" || line.equalsIgnoreCase("help")) printHelp();
    else queueCommand(line, "serial");
}

void pollSerial()
{
    static uint32_t lastChar = 0;
    while (Serial.available()) {
        int ch = Serial.read();
        if (ch < 0) break;
        lastChar = millis();
        if (ch == '\n' || ch == '\r') submitSerialLine();          // CR, LF or CRLF
        else if (ch >= 0x20 && serialLine.length() < 200) serialLine += (char)ch;
    }
#if SERIAL_IDLE_SUBMIT_MS > 0
    // Serial Monitor set to "No line ending": the whole line arrives at once, so take it
    // after a short pause instead of waiting forever for '\n'.
    if (serialLine.length() && millis() - lastChar > SERIAL_IDLE_SUBMIT_MS) submitSerialLine();
#endif
}

String statusJson()
{
    JsonDocument d;
    d["motion"] = motionState;
    if (dmpReady) d["heading"] = heading();
    else d["heading"] = "n/a";
    d["imu"] = imuState;
    d["imuResets"] = imuResets;
    d["yaw"] = (int)yawDeg;
    d["task"] = taskState;
    d["place"] = currentPlace;
    d["scene"] = lastScene;
    d["heard"] = lastHeard;
    d["reply"] = lastReply;
    d["voiceTrigger"] = voiceTrigger;
    d["ip"] = ipString();
    String s;
    serializeJson(d, s);
    return s;
}

// INDEX_HTML (web page) is in RoboCar_Gemma4.h


void sendHttp(int sock, int code, const char *type, const uint8_t *body, size_t len)
{
    const char *reason = code == 200 ? "OK" : code == 400 ? "Bad Request" : code == 404 ? "Not Found" :
                         code == 405 ? "Method Not Allowed" : "Service Unavailable";
    String h = "HTTP/1.1 " + String(code) + " " + reason + "\r\nContent-Type: " + type +
               "\r\nContent-Length: " + String(len) +
               "\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n";
    if (writeAll(sock, h) && len) writeAll(sock, body, len);
}

void sendText(int sock, int code, const char *text)
{
    sendHttp(sock, code, "text/plain", (const uint8_t *)text, strlen(text));
}

// Start / restart the listener whenever WiFi is (re)connected.
void maintainWeb()
{
    static uint32_t lastTry = 0;
    if (WiFi.status() != WL_CONNECTED) {
        if (webListener >= 0) {
            for (int i = 0; i < WEB_SLOTS; i++) {
                roboCloseSocket(webSock[i]);
                webSock[i] = -1;
            }
            roboCloseSocket(webListener);
            webListener = -1;
        }
        return;
    }
    if (webListener >= 0 || (lastTry && millis() - lastTry < 5000UL)) return;
    lastTry = millis();
    for (int i = 0; i < WEB_SLOTS; i++) webSock[i] = -1;
    webListener = roboListen(WEB_PORT);
    if (webListener >= 0) Serial.println("[Web] server at http://" + ipString() + "/");
    else Serial.println("[Web] HTTP server failed to start");
}

// Serve one request on a socket that already has data waiting.
void serveWeb(int sock)
{
    WiFiClient c((uint8_t)sock);    // used for reading only (see writeAll); closes the socket
    RoboHttpReader req(c, WEB_REQ_MS);
    String first;
    if (!req.headers(first)) {    // closed preconnect, garbage or too slow
        c.stop();
        return;
    }
    // "GET /path?query HTTP/1.1"
    int s1 = first.indexOf(' ');
    int s2 = first.indexOf(' ', s1 + 1);
    if (s1 <= 0 || s2 <= s1) {
        sendText(sock, 400, "bad request");
        c.stop();
        return;
    }
    String method = first.substring(0, s1);
    String url = first.substring(s1 + 1, s2);
    String path = url, query = "";
    int qm = url.indexOf('?');
    if (qm >= 0) { path = url.substring(0, qm); query = url.substring(qm + 1); }
    if (path != "/status.json") Serial.println("[Web] " + method + " " + url);    // status is polled every 2 s

    if (method != "GET") {
        sendText(sock, 405, "GET only");
    } else if ((path == "/" && req.accept.indexOf("text/html") < 0) || path == "/ready") {
        // curl / scripts (no "Accept: text/html"): plain-text ready message
        sendText(sock, 200, "RoboCar is ready\n");
    } else if (path == "/" || path == "/index.html") {
        sendHttp(sock, 200, "text/html; charset=utf-8", (const uint8_t *)INDEX_HTML, strlen(INDEX_HTML));
    } else if (path == "/status.json") {
        String s = statusJson();
        sendHttp(sock, 200, "application/json", (const uint8_t *)s.c_str(), s.length());
    } else if (path == "/snap.jpg") {
        // Copy the frame first: the camera keeps writing its buffers while a slow
        // client is still downloading, which would corrupt the JPEG.
        uint32_t addr = 0, len = 0;
        if (camRunning) Camera.getImage(CHANNEL, &addr, &len);    // paused only while speaking
        uint8_t *jpg = (addr && len) ? (uint8_t *)malloc(len) : NULL;
        if (jpg) {
            memcpy(jpg, (const void *)addr, len);
            sendHttp(sock, 200, "image/jpeg", jpg, len);
            free(jpg);
        } else {
            sendText(sock, 503, "no camera frame");
        }
    } else if (path == "/F" || path == "/B" || path == "/L" || path == "/R" || path == "/S" || path == "/V") {
        queueCommand(path.substring(1), "web");
        sendText(sock, 200, "OK");
    } else if (path == "/cmd") {
        String t;
        int at = query.startsWith("q=") ? 0 : query.indexOf("&q=");
        if (at >= 0) {
            int from = at == 0 ? 2 : at + 3;
            int to = query.indexOf('&', from);
            t = urlDecode(to < 0 ? query.substring(from) : query.substring(from, to));
        }
        t.trim();
        if (t.length()) {
            queueCommand(t, "web");
            sendText(sock, 200, "OK");
        } else {
            sendText(sock, 400, "use /cmd?q=<command>");
        }
    } else {
        sendText(sock, 404, "not found");
    }
    c.stop();
}

// Never blocks on idle sockets: browsers open "preconnect" connections that send
// nothing, so a socket is only read once select() says its request has arrived.
void pollWeb()
{
    maintainWeb();
    if (webListener < 0) return;
    for (int i = 0; i < WEB_SLOTS; i++) {
        if (webSock[i] >= 0) continue;
        int s = roboAccept(webListener, 500);
        if (s < 0) break;
        webSock[i] = s;
        webSince[i] = millis();
    }
    for (int i = 0; i < WEB_SLOTS; i++) {
        int s = webSock[i];
        if (s < 0) continue;
        int r = roboReadable(s);
        if (r == 0) {
            if (millis() - webSince[i] > WEB_IDLE_MS) {
                roboCloseSocket(s);
                webSock[i] = -1;
            }
            continue;
        }
        webSock[i] = -1;
        if (r < 0) roboCloseSocket(s);
        else serveWeb(s);    // the WiFiClient in serveWeb() closes the socket
    }
}

void pollInputs()
{
    static bool busy = false;    // no re-entry (pollInputs is called from inside waits)
    if (busy) return;
    busy = true;
    pollSerial();
    pollWeb();
    busy = false;
}

// ============================== setup / loop ==============================
void setup()
{
    Serial.begin(115200);
#if USE_IMU
    // Same order as WebServer_RoboCar_DRV8833_IMU: start I2C first, then give the
    // MPU6050 time (delay below) before mpu.initialize().
    Wire.begin();
    Wire.setClock(400000);
#endif

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // DRV8833
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(IN3_PIN, OUTPUT);
    pinMode(IN4_PIN, OUTPUT);
    motorsStop();
    Serial.println("Motors stopped");
    delay(1000);

    // MPU6050
    startIMU();

    // WiFi
    int status = WL_IDLE_STATUS;
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        delay(10000);
    }
    Serial.println("RoboCar IP: " + ipString());
    maintainWeb();    // HTTP server on port WEB_PORT

    // Camera (JPEG snapshots for Gemma-4)
    camConfig.setRotation(0);
    Camera.configVideoChannel(CHANNEL, camConfig);
    Camera.videoInit();
    cameraResume();

    // SD card (stays mounted: never call fs.end())
    sdReady = fs.begin();
    Serial.println(sdReady ? "SD card mounted" : "SD card mount FAILED (voice and TTS disabled)");

    // Audio: mic -> AAC -> audio-only MP4 (no VOE), plus the mic-level monitor
    audio.configAudio(configA);
    aac.configAudio(configA);
    mp4.configAudio(configA, CODEC_AAC);
    mp4.setRecordingDuration(RECORDING_SECONDS);
    mp4.setRecordingFileCount(1);
    mp4.setRecordingFileName(CLIP_BASE);
    mp4.setRecordingDataType(STORAGE_AUDIO);
    audioStreamer.registerInput(audio);
    audioStreamer.registerOutput(audio);
    audioToAAC.registerInput(audio);
    audioToAAC.registerOutput(aac);
    aacToMp4.registerInput(aac);
    aacToMp4.registerOutput(mp4);
    // audio.begin() is deferred to the first recording / mic monitor (audioBegin()), so
    // the MP3 player never has to take the codec over from a running audio module.

    voiceTrigger = MicProbe::has(audio, 0) && sdReady;
    Serial.println(voiceTrigger ? "Voice trigger: mic level" : "Voice trigger: use 'V' (serial) or the Voice web button");

    Serial.println("[Setup] camera settling (2 s)...");
    delay(2000);    // let the camera auto-exposure settle

    // README item 1: flash LED twice = RoboCar ready
    flashLED(2);
    Serial.println("RoboCar ready. Open http://" + ipString() + "/");
    speak("RoboCar is ready. My address is " + ipString() + ".");
    while (Serial.available()) Serial.read();    // drop anything typed during the long setup
    serialLine = "";
    printHelp();
    startListening();
}

void loop()
{
    pollInputs();    // no IMU read here: only status / turns / navigation read the heading

    if (pendingCmd.length()) {
        String cmd = pendingCmd, src = pendingSrc;
        pendingCmd = "";
        abortTask = false;
        stopListening();
        handleCommand(cmd, src);
        taskState = "IDLE";
        startListening();
        return;
    }

    // Hands-free voice command (only when the core provides Audio::micLevel())
    if (listening && millis() > quietUntil) {
        int level = MicProbe::level(audio, 0);
        if (level > TRIGGER_VOL) {
            Serial.println("[Voice] triggered, mic level " + String(level));
            abortTask = false;
            voiceCommand();
            taskState = "IDLE";
            startListening();
            return;
        }
    }

#if AUTO_SCENE_MS > 0
    if (millis() - lastAutoScene > AUTO_SCENE_MS) {
        lastAutoScene = millis();
        stopListening();
        recognizeScene();
        startListening();
    }
#endif
    delay(20);
}
