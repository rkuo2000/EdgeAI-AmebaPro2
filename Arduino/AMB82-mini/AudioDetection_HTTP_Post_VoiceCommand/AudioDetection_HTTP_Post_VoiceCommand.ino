/*

 Example guide:
 https://www.amebaiot.com/en/amebapro2-arduino-neuralnework-face-audio-classification/
  
 1. run Server on PC:
    cd ~/Arduino/examples/AMB82-MINI/src
    python AmebaPro2_Whisper_LLaVA_server.py
    or
    python AmebaPro2_Whisper_Phi3_server.py
    
 2. run this client on AMB82-mini
    say "Attention" to AMB82-mini first, wait for LED light up then speak voice-command to robot
    ex. char COMMAND1[256] = "Please go to the kitch.";

*/

#include "string.h"
#include "StreamIO.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "MP4Recording.h"
#include "WiFi.h"
#include "AmebaFatFS.h"
#include "Base64.h"
#include "ArduinoJson.h"
#include "SPI.h"
#include "NNAudioClassification.h"
#include "AudioClassList.h"

// NN audio classification requires 16KHz
AudioSetting configA(16000, 1, USE_AUDIO_AMIC);    // Sample rate, Channel count, Mic type
Audio audio;
NNAudioClassification audioNN;
StreamIO audioStreamerNN(1, 1);    // 1 Input Audio -> 1 Output Audio Classification

int max_id = 0;
int max_prob = 0;

#define FILENAME "TestRecordingAudioOnly.mp4"

char ssid[] = "Your_SSID";            // your network SSID (Home WiFi or Smartphone Hotspot)
char pass[] = "Your_Password";        // your network password
int status = WL_IDLE_STATUS;

char server[] = "123.195.32.57";   // the server IP running HTTP server on PC
#define PORT 5000

AmebaFatFS fs;
WiFiClient wifiClient;

char buf[512];
char *p;
String filepath;
File file;

// Default audio preset configurations:
// 0 :  8kHz Mono Analog Mic
// 1 : 16kHz Mono Analog Mic
// 2 :  8kHz Mono Digital PDM Mic
// 3 : 16kHz Mono Digital PDM Mic
AAC aac;
MP4Recording mp4;
StreamIO audioStreamer1(1, 1);    // 1 Input Audio -> 1 Output AAC
StreamIO audioStreamer2(1, 1);    // 1 Input AAC -> 1 Output MP4

const int buttonPin = 1;          // the number of the pushbutton pin
int buttonState;                          // variable for reading the pushbutton status
unsigned long buttonPressTime = 0;        // variable to store the time when button was pressed
bool buttonPressedFor2Seconds = false;    // flag to indicate if button is pressed for at least 2 seconds
int recordingstate = -1;
int previousRecordingState = -1;
bool blinkstate = false;

// Receiving Buffer
#define OFFSET 94+4            // skip HTTP response headers + 4 ("ASR:")
#define PAGE_CHAR_LENGTH 26*15 // Font 2 = 26 characters per line, total 14 lines
char textbuffer[256];             // text buffer
char command[256];
char COMMAND1[256] = "Please go to the kitch.";

int  textcount = 0;
int  skipcount = 0;

void setup()
{
    Serial.begin(115200);

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

    // Configure audio peripheral for audio data format
    audio.configAudio(configA);
    audio.begin();
    // Configure AAC audio encoder
    aac.configAudio(configA);
    aac.begin();    

    audioNN.configAudio(configA);
    audioNN.setResultCallback(ACPostProcess);
    audioNN.modelSelect(AUDIO_CLASSIFICATION, NA_MODEL, NA_MODEL, NA_MODEL, DEFAULT_YAMNET);
    audioNN.begin();

    // Configure StreamIO object to stream data from audio to audio classification
    audioStreamerNN.registerInput(audio);
    audioStreamerNN.registerOutput(audioNN);
    if (audioStreamerNN.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }
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
/*
    // check if the button has been held for at least 2 seconds
    if (buttonState == HIGH && millis() - buttonPressTime >= 2000) {
        // button has been pressed for at least 2 seconds
        buttonPressedFor2Seconds = true;
    } else if (voiceActivated){
        // Speech detected
        buttonPressedFor2Seconds = true;
        voiceActivated = false;
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
            tft.clr();
            tft.setCursor(0,0);
        }
    }
*/    
    if (recordingstate == 1 && previousRecordingState == 0) {
        // Change from 0 to 1
        digitalWrite(LED_BUILTIN, HIGH);
    } else if (recordingstate == 0 && previousRecordingState == 1) {
        encodeMP4andsendHttpPostRequest();
        // Change from 1 to 0
        digitalWrite(LED_BUILTIN, LOW);  
    }
    // Check if there are incoming bytes available from the server
    textcount =0;    
    skipcount =0;
    while (wifiClient.available()) {
        char c = wifiClient.read();
        Serial.write(c);
        if (skipcount>=OFFSET) {
          textbuffer[textcount]=c;
          textcount++;
        }
        skipcount++;
    }
    if (skipcount>=OFFSET) {
        strcpy(command, textbuffer);
        Serial.println(command);
        textbuffer[0]=(char)0; // clear textbuffer
    }
    if (strcmp(command,COMMAND1)==0) {
      Serial.println("VOICE COMMAND Matched !!!");
      blink(1);
      command[0]=(char)0;
    }
    previousRecordingState = recordingstate;
}

void blink(int times){
    for (int i=0;i<times;i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(300);
      digitalWrite(LED_BUILTIN, LOW);
      delay(300);
    }
}

// User callback function
void ACPostProcess(std::vector<AudioClassificationResult> results)
{
    digitalWrite(LED_G, HIGH);
    printf("No of Audio Detected = %d\r\n", audioNN.getResultCount());
    
    if (audioNN.getResultCount() > 0) {
        max_id =0;
        max_prob =0;
        for (int i = 0; i < audioNN.getResultCount(); i++) {
            AudioClassificationResult audio_item = results[i];
            int class_id = (int)audio_item.classID();
            if (audioNames[class_id].filter) {
                int prob = audio_item.score();
                printf("%d class %d, score: %d, audio name: %s\r\n", i, class_id, prob, audioNames[class_id].audioName);
                if (prob > max_prob) {
                  max_id   = class_id;
                  max_prob = prob;
                }
            }
        }
        printf("MAX class %d, prob: %d, class_name: %s\r\n", max_id, max_prob, audioNames[max_id].audioName);
        
        if (max_id==0 && max_prob>80 && !recordingstate) {       
          digitalWrite(LED_BUILTIN, HIGH);
          mp4.begin();
          Serial.println("Recording Audio...............");

        }    
    }
    digitalWrite(LED_G, LOW);
}

void encodeMP4andsendHttpPostRequest()
{
    memset(buf, 0, sizeof(buf));
    fs.readDir(fs.getRootPath(), buf, sizeof(buf));
    filepath = String(fs.getRootPath()) + String(FILENAME);
    p = buf;
    while (strlen(p) > 0) {
        /* list out file name image will be saved as "TestRecordingAudioOnly.mp4"*/
        if (strstr(p, FILENAME) != NULL) {
            Serial.println("[INFO] Found 'TestRecordingAudioOnly.mp4' in the string.");
            Serial.println("[INFO] Processing file...");
        } else {
            // Serial.println("Substring 'image.jpg' not found in the
            // string.");
        }
        p += strlen(p) + 1;
    }
    uint8_t *fileinput;
    file = fs.open(filepath);
    unsigned int fileSize = file.size();
    fileinput = (uint8_t *)malloc(fileSize + 1);
    file.read(fileinput, fileSize);
    fileinput[fileSize] = '\0';
    file.close();

    // Encode the file data as Base64
    int encodedLen = base64_enc_len(fileSize);
    char *encodedData = (char *)malloc(encodedLen);
    base64_encode(encodedData, (char *)fileinput, fileSize);

    JsonDocument doc;

    //Change "base64_string" to the key that you set in your server.
    doc["base64_string"] = encodedData;
    String jsonString;
    serializeJson(doc,jsonString);

    if (wifiClient.connect(server, PORT)) {
        wifiClient.println("POST /asr HTTP/1.1"); // asr = speech recognition, audio = to LLM/VLM server
        wifiClient.println("Host: " + String(server));
        wifiClient.println("Content-Type: application/json");    // Use appropriate content type
        wifiClient.println("Content-Length: " + String(jsonString.length()));              // Specify the length of the content
        wifiClient.println("Connection: keep-alive");
        wifiClient.println();             // Empty line indicates the end of headers
        wifiClient.print(jsonString);    // Send the Base64 encoded audio data directly
        
        Serial.println("Binary sent");
    }
}
