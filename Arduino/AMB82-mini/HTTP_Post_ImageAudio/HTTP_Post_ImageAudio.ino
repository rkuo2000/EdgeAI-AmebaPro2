/*
 Client to Post Image+Audio to AmebaPro2 LLM/VLM server
*/

#define ARDUINOJSON_STRING_LENGTH_SIZE 4

#include "StreamIO.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "MP4Recording.h"
#include "WiFi.h"
#include "AmebaFatFS.h"
#include "Base64.h"
#include "ArduinoJson.h"

#define CHANNEL       0
#define AUDIOFILENAME "TestRecordingAudioOnly.mp4"

char ssid[] = "HITRON-DF90-2.4G";           // your network SSID (hotspot name)
char pass[] = "0972211921";        // your network password
int status = WL_IDLE_STATUS;

char server[] = "123.195.32.57";    // your server IP running HTTP server on PC
#define PORT 5000

AmebaFatFS fs;
WiFiClient wifiClient;

char buf[512];
String filepath;
File file;

// Default audio preset configurations:
// 0 :  8kHz Mono Analog Mic
// 1 : 16kHz Mono Analog Mic
// 2 :  8kHz Mono Digital PDM Mic
// 3 : 16kHz Mono Digital PDM Mic
AudioSetting configA(0);
Audio audio;
AAC aac;
MP4Recording mp4;
StreamIO audioStreamer1(1, 1);    // 1 Input Audio -> 1 Output AAC
StreamIO audioStreamer2(1, 1);    // 1 Input AAC -> 1 Output MP4

int encodedAudioLen;
char *encodedAudioData;

int encodedImageLen;
char *encodedImageData;

uint32_t img_addr = 0;
uint32_t img_len = 0;

VideoSetting config(VIDEO_FHD, CAM_FPS, VIDEO_JPEG, 1);

const int buttonPin = 1;          // the number of the pushbutton pin
int buttonState;                          // variable for reading the pushbutton status
unsigned long buttonPressTime = 0;        // variable to store the time when button was pressed
bool buttonPressedFor2Seconds = false;    // flag to indicate if button is pressed for at least 2 seconds
int recordingstate = -1;
int previousRecordingState = -1;


void setup()
{
    Serial.begin(115200);

    Camera.configVideoChannel(CHANNEL, config);
    Camera.videoInit();
    Camera.channelBegin(CHANNEL);

    // Connection to internet
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        delay(2000);
    }

    // list files under root directory
    fs.begin();

    // initialize the pushbutton pin as an input:
    pinMode(buttonPin, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_G, OUTPUT);

    // Configure audio peripheral for audio data output
    audio.configAudio(configA);
    audio.begin();
    // Configure AAC audio encoder
    aac.configAudio(configA);
    aac.begin();

    // Configure MP4 recording settings
    mp4.configAudio(configA, CODEC_AAC);
    mp4.setRecordingDuration(5);
    mp4.setRecordingFileCount(1);
    mp4.setRecordingFileName("TestRecordingAudioOnly");
    mp4.setRecordingDataType(STORAGE_AUDIO);    // Set MP4 to record audio only

    // Configure StreamIO object to stream data from audio channel to AAC encoder
    audioStreamer1.registerInput(audio);
    audioStreamer1.registerOutput(aac);
    if (audioStreamer1.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }

    // Configure StreamIO object to stream data from AAC encoder to MP4
    audioStreamer2.registerInput(aac);
    audioStreamer2.registerOutput(mp4);
    if (audioStreamer2.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }
}

void loop()
{
    // Button state
    int newButtonState = digitalRead(buttonPin);
    if (newButtonState != buttonState) {
        buttonPressTime = millis();
    }
    // update button state
    buttonState = newButtonState;

    // update recording state
    recordingstate = (int)(mp4.getRecordingState());

    // check if the button has been held for at least 2 seconds
    if (buttonState == HIGH && millis() - buttonPressTime >= 2000) {
        // button has been pressed for at least 2 seconds
        buttonPressedFor2Seconds = true;
    } else {
        // button was released before 2 seconds
        buttonPressedFor2Seconds = false;
    }
    // if button has been pressed for at least 2 seconds
    if (buttonPressedFor2Seconds) {
        if (recordingstate == 1) {
            digitalWrite(LED_BUILTIN, HIGH);
        } else {
            mp4.begin();
            Serial.println("Recording");
        }
        Camera.getImage(CHANNEL, &img_addr, &img_len);
        encodeJPG();
    }
    if (recordingstate == 1 && previousRecordingState == 0) {
        // Change from 0 to 1
        digitalWrite(LED_BUILTIN, HIGH);
    } else if (recordingstate == 0 && previousRecordingState == 1) {
        encodeMP4andsendHttpPostRequest();
        // Change from 1 to 0
        digitalWrite(LED_BUILTIN, LOW);
    }

    // Check if there are incoming bytes available from the server
    while (wifiClient.available()) {
        char c = wifiClient.read();
        Serial.write(c);
    }
    previousRecordingState = recordingstate;
    delay(10);
}

void encodeJPG()
{
    // Encode the file data as Base64
    encodedImageLen = base64_enc_len(img_len);
    encodedImageData = (char *)malloc(encodedImageLen);
    base64_encode(encodedImageData, (char *)img_addr, img_len);
}

void encodeMP4andsendHttpPostRequest()
{
    memset(buf, 0, sizeof(buf));
    fs.readDir(fs.getRootPath(), buf, sizeof(buf));
    filepath = String(fs.getRootPath()) + String(AUDIOFILENAME);

    uint8_t *fileinput;
    file = fs.open(filepath);
    unsigned int fileSize = file.size();
    fileinput = (uint8_t *)malloc(fileSize + 1);
    file.read(fileinput, fileSize);
    fileinput[fileSize] = '\0';
    file.close();

    // Encode the file data as Base64
    encodedAudioLen = base64_enc_len(fileSize);
    encodedAudioData = (char *)malloc(encodedAudioLen);
    base64_encode(encodedAudioData, (char *)fileinput, fileSize);

    JsonDocument doc;

    //Change "base64_string" to the key that you set in your server.
    doc["audio_base64_string"] = encodedAudioData;
    doc["image_base64_string"] = encodedImageData;

    String jsonString;
    serializeJson(doc,jsonString);

    if (wifiClient.connect(server, PORT)) {
        wifiClient.println("POST /imgau HTTP/1.1");
        wifiClient.println("Host: " + String(server));
        wifiClient.println("Content-Type: application/json");    // Use appropriate content type
        wifiClient.println("Content-Length: " + String(jsonString.length()));              // Specify the length of the content
        wifiClient.println("Connection: keep-alive");
        wifiClient.println();             // Empty line indicates the end of headers
        wifiClient.print(jsonString);    // Send the Base64 encoded audio data directly
        Serial.println("Binary sent");
    }
}
