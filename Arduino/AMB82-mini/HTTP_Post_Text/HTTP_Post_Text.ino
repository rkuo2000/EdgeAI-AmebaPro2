/*
  AMB82-mini Post Text to Google Gemini
*/

#include "StreamIO.h"
#include "WiFi.h"
#include "Base64.h"
#include "ArduinoJson.h"


char ssid[] = "HITRON-DF90-2.4G";    // your network SSID (name)
char pass[] = "0972211921";        // your network password
int status = WL_IDLE_STATUS;

char server[] = "142.251.46.170";    // Gemini server IP address
#define PORT 443

const char API_Key[] = "AIzaSyCnRzbxgrMX1GjIHnN7U6EQVM8YKy9Ikw4";

String prompt = "What is the large language model?";

WiFiClient wifiClient;

const int buttonPin = 1;          // the number of the pushbutton pin
int buttonState;                          // variable for reading the pushbutton status
unsigned long buttonPressTime = 0;        // variable to store the time when button was pressed
bool buttonPressedFor2Seconds = false;    // flag to indicate if button is pressed for at least 2 seconds


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
        encodeTextandsendHttpPostRequest();
        digitalWrite(LED_BUILTIN, LOW);             
    }

    // Check if there are incoming bytes available from the server
    while (wifiClient.available()) {
        char c = wifiClient.read();
        Serial.write(c);
    }

    delay(10);
}

void encodeTextandsendHttpPostRequest()
{
    String jsonString;
    jsonString = '{ "contents": [{"parts": [{"text": '+ prompt + '}] }] }';
    char post_text[] = "POST /v1beta/models/gemini-1.5-flash:generateContent?key=";

    if (wifiClient.connect(server, PORT)) {
        wifiClient.print("POST /v1beta/models/gemini-1.5-flash:generateContent?key=");
        wifiClient.print(API_Key);
        wifiClient.println("Content-Type: application/json");    // Use appropriate content type
        wifiClient.print(jsonString);    // Send the Base64 encoded audio data directly
        wifiClient.println();             // Empty line indicates the end of headers        
        Serial.println("Binary sent");
    }
}
