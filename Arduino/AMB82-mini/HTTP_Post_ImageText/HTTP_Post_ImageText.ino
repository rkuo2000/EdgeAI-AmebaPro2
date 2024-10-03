/*
 Client to Post Image+Text to AmebaPro2 LLM/VLM server
*/

#define ARDUINOJSON_STRING_LENGTH_SIZE 4

#include "StreamIO.h"
#include "WiFi.h"
#include "AmebaFatFS.h"
#include "Base64.h"
#include "ArduinoJson.h"

#define CHANNEL       0
#define AUDIOFILENAME "TestRecordingAudioOnly.mp4"

char ssid[] = "KUO";    // your network SSID (name)
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

const int buttonPin = 1;          // the number of the pushbutton pin
int buttonState;                          // variable for reading the pushbutton status
unsigned long buttonPressTime = 0;        // variable to store the time when button was pressed
bool buttonPressedFor2Seconds = false;    // flag to indicate if button is pressed for at least 2 seconds


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
        digitalWrite(LED_BUILTIN, LOW);             
    }

    // Check if there are incoming bytes available from the server
    while (wifiClient.available()) {
        char c = wifiClient.read();
        Serial.write(c);
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
    }
}
