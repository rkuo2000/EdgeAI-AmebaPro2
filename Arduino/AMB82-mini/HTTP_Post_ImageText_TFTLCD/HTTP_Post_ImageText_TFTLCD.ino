/*
 Client to Post Image+Text to AmebaPro2 LLM/VLM server
*/

#define ARDUINOJSON_STRING_LENGTH_SIZE 4

#include "StreamIO.h"
#include "WiFi.h"
#include "AmebaFatFS.h"
#include "Base64.h"
#include "ArduinoJson.h"
#include "SPI.h"
#include "AmebaILI9341.h"

#define TFT_RESET 5
#define TFT_DC    4
#define TFT_CS    SPI_SS

AmebaILI9341 tft = AmebaILI9341(TFT_CS, TFT_DC, TFT_RESET);

#define ILI9341_SPI_FREQUENCY 20000000

#define FILENAME "TestRecordingAudioOnly.mp4"
#define CHANNEL       0

char ssid[] = "HITRON-DF90-2.4G";    // your network SSID (name)
char pass[] = "0972211921";        // your network password
int status = WL_IDLE_STATUS;

char server[] = "123.195.32.57";    // your server IP running HTTP server on PC
#define PORT 5000

AmebaFatFS fs;
WiFiClient wifiClient;

char prompt[] = "what do you see in this image?";

int encodedImageLen;
char *encodedImageData;

uint32_t img_addr = 0;
uint32_t img_len = 0;

VideoSetting config(VIDEO_FHD, CAM_FPS, VIDEO_JPEG, 1);


char buf[512];
char *p;
String filepath;
File file;

const int buttonPin = 1;          // the number of the pushbutton pin
int buttonState;                          // variable for reading the pushbutton status
unsigned long buttonPressTime = 0;        // variable to store the time when button was pressed
bool buttonPressedFor2Seconds = false;    // flag to indicate if button is pressed for at least 2 seconds

// Receiving Buffer
#define OFFSET 94              // skip HTTP response headers
#define PAGE_CHAR_LENGTH 26*15 // Font 2 = 26 characters per line, total 14 lines
char textbuffer[26*15*3];      // 3 pages buffer 
int  textcount = 0;
int  skipcount = 0;
int  displaycount =0;
//int  pos=0; // for finding "LLM:"

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

    SPI.setDefaultFrequency(ILI9341_SPI_FREQUENCY);
    tft.begin();
    tft.clr();
    tft.setCursor(0, 0);
    tft.setForeground(ILI9341_WHITE);
    tft.setFontSize(2);
    tft.setRotation(1);
    tft.println("Portable Gemini:");

    // list files under root directory
    fs.begin();

    // initialize the pushbutton pin as an input:
    pinMode(buttonPin, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_G, OUTPUT);

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
        digitalWrite(LED_BUILTIN, HIGH);    
        Serial.println("Image Capturing...");
        Camera.getImage(CHANNEL, &img_addr, &img_len);
        encodeImageandsendHttpPostRequest();           
    }

    // Check if there are incoming bytes available from the server
    textcount =0;    
    skipcount =0;
    while (wifiClient.available()) {
        char c = wifiClient.read();
        textbuffer[textcount] = c;
        textcount++;
        skipcount++;
        Serial.write(c);
        if (skipcount>OFFSET) tft.print(c);
    }

    delay(10);
}

void encodeImageandsendHttpPostRequest()
{
    // Encode the file data as Base64
    encodedImageLen = base64_enc_len(img_len);
    encodedImageData = (char *)malloc(encodedImageLen);
    base64_encode(encodedImageData, (char *)img_addr, img_len);

    Serial.println(prompt);
    JsonDocument doc;

    //Change "base64_string" to the key that you set in your server.
    doc["text_base64_string"] = prompt;
    doc["image_base64_string"] = encodedImageData;

    String jsonString;
    serializeJson(doc,jsonString);

    if (wifiClient.connect(server, PORT)) {
        wifiClient.println("POST /imgtxt HTTP/1.1");
        wifiClient.println("Host: " + String(server));
        wifiClient.println("Content-Type: application/json");    // Use appropriate content type
        wifiClient.println("Content-Length: " + String(jsonString.length()));              // Specify the length of the content
        wifiClient.println("Connection: keep-alive");
        wifiClient.println();             // Empty line indicates the end of headers
        wifiClient.print(jsonString);    // Send the Base64 encoded audio data directly
        Serial.println("Binary sent");
        digitalWrite(LED_BUILTIN, LOW);
        
        tft.println("Voice sent to Gemini server !");
        tft.println("wait a moment for Gemini.....");        
    }
    delay(1000);
    tft.clr();
    tft.setCursor(0,0);    
}
