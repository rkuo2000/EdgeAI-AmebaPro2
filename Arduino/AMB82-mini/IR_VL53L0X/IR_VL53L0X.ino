#include <Wire.h>
#include <VL53L0x_IR_libraries/VL53L0X.h>

VL53L0X sensor;

void setup()
{
    Serial.begin(115200);
    //Wire.begin();
    Wire1.begin();

    sensor.setTimeout(500);
    if (!sensor.init()) {
        Serial.println("Failed to detect and initialize sensor!");
        while (1) {
        }
    }

    // Start continuous back-to-back mode (take readings as
    // fast as possible).  To use continuous timed mode
    // instead, provide a desired inter-measurement period in
    // ms (e.g. sensor.startContinuous(100)).
    sensor.startContinuous();
}

void loop()
{
    int distance = sensor.readRangeContinuousMillimeters();
    Serial.print(distance);
    
    if (sensor.timeoutOccurred()) {
        Serial.print(" TIMEOUT");
    }

    Serial.println();
}
