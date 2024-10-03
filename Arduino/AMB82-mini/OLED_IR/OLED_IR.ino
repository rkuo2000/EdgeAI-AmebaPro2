#include <Wire.h>
#include <Adafruit_OLED_libraries/Adafruit_GFX.h>
#include <Adafruit_OLED_libraries/Adafruit_SSD1306.h>
#include <VL53L0X_IR_libraries/VL53L0X.h>

#define SCREEN_WIDTH  128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64     // OLED display height, in pixels
#define OLED_RESET     -1      // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C    ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

VL53L0X sensor;

void setup()
{
    Serial.begin(115200);
    //Wire.begin();

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ;    // Don't proceed, loop forever
    }
    display.clearDisplay();

    display.setTextSize(1);                 // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);    // Draw white text
    display.setCursor(0, 0);                // Start at top-left corner
    display.println("OLED+VL53L0X+MPU6050");
    display.display();

    sensor.setTimeout(500);
    if (!sensor.init()) {
        Serial.println("Failed to detect and initialize sensor!");
        while (1) {
        }
    }
    sensor.startContinuous();
}

void loop()
{
    int distance = sensor.readRangeContinuousMillimeters();
    
    String d = String(distance);
    Serial.println(distance);
    
    String s = "IR: "+d+" mm";
    display.clearDisplay();
    display.setCursor(0,8);
    display.println(s);
    display.display();
    
    if (sensor.timeoutOccurred()) {
        Serial.println(" TIMEOUT");
    }
}
