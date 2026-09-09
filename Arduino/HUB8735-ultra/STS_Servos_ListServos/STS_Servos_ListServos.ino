// List all servos on bus
//
// This code is meant to be run on a board with more than one serial port
// (one for the servos, one to print the value back to the host)
//
// This code was tested on an Arduino Mega

#include "STSServoDriver.h"

STSServoDriver servos;

void setup() {
  Serial.begin(115200);
  Serial.println();
  // Try to connect with the servos, on Serial1, using pin 20 as direction pin

  if (!servos.init(20, &Serial2))
  {
    Serial.println("No Servo detected");
  }
  else
  {
    Serial.println("Listing Servos :");
    for (int i = 0; i < 0xFE; i++)
    {
      if (servos.ping(i))
      {
        Serial.print(" - ");
        Serial.println(static_cast<int>(i));
      }
    }
  }
}

void loop()
{
}
