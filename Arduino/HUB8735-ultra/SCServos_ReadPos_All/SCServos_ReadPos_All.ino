/*
Read back all feedback parameters: position, speed, load, voltage, temperature, movement status;
The FeedBack function reads back the servo parameters in the buffer, and the Readxxx (-1) function returns the corresponding servo state in the buffer;
Function Readxxx (ID), ID=1 returns the FeedBack buffer parameter; ID > 0, and directly returns the specified ID rudder state by reading the instruction.
There is no need to call the FeedBack function.
*/

// the UART used to control servos.
// GPIO 18 - S_RXD, GPIO 19 - S_TXD, as default.
#include <SCServo.h>

SMS_STS sms_sts;

void setup()
{
  Serial.begin(115200);
  Serial2.begin(1000000, SERIAL_8N1);
  sms_sts.pSerial = &Serial2;
  delay(1000);
  Serial.println();
}

void loop()
{ 
  int Pos;
  int Speed;
  int Load;
  int Voltage;
  int Temper;
  int Move;
  int Current;
  for (int id=20; id<34; id++) {    
    if(sms_sts.FeedBack(id)!=-1){
      Pos = sms_sts.ReadPos(id);
  /*
    Speed = sms_sts.ReadSpeed(-1);
    Load = sms_sts.ReadLoad(-1);
    Voltage = sms_sts.ReadVoltage(-1);
    Temper = sms_sts.ReadTemper(-1);
    Move = sms_sts.ReadMove(-1);
    Current = sms_sts.ReadCurrent(-1);
  */
    Serial.print("ID: ");
    Serial.print(id);
    Serial.print("  Pos: ");
    Serial.println(Pos); 
  /*
    Serial.print("Position:");
    Serial.println(Pos);
    Serial.print("Speed:");
    Serial.println(Speed);
    Serial.print("Load:");
    Serial.println(Load);
    Serial.print("Voltage:");
    Serial.println(Voltage);
    Serial.print("Temper:");
    Serial.println(Temper);
    Serial.print("Move:");
    Serial.println(Move);
    Serial.print("Current:");
    Serial.println(Current);
  */
      delay(10);
    } else {
      Serial.print("ID: ");
      Serial.print(id);
      Serial.println("  ReadPos err");
      delay(500);
    }
    delay(500);
  }
  Serial.println();
}
