/*
 Press button for 2 sec to capture image into SDcard
 Example guide:
 https://www.amebaiot.com/en/amebapro2-arduino-video-jpeg-sdcard/
*/

#include "VideoStream.h"
#include "AmebaFatFS.h"

#define CHANNEL  0
#define FILENAME "image.jpg"

VideoSetting config(VIDEO_FHD, CAM_FPS, VIDEO_JPEG, 1);

uint32_t img_addr = 0;
uint32_t img_len = 0;

AmebaFatFS fs;
File file;

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

    // list files under root directory
    fs.begin();
    file = fs.open(String(fs.getRootPath()) + String(FILENAME));

    // initialize the pushbutton pin as an input:
    pinMode(buttonPin, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_G, OUTPUT);
    Serial.println("Please press button for 2 seconds to catpure image !");
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
      Serial.println("Catpuring Image...");
      Camera.getImage(CHANNEL, &img_addr, &img_len);
      file.write((uint8_t *)img_addr, img_len);
      file.close();
      fs.end();
      Serial.println("Image stored into SDcard !");
      buttonPressedFor2Seconds = false;
      delay(500);
    }
}
