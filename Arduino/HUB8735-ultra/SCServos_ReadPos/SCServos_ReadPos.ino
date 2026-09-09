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

#define SERVO_ID 24

void setup()
{
  Serial.begin(115200);
  Serial2.begin(1000000, SERIAL_8N1);
  sms_sts.pSerial = &Serial2;
  delay(1000);
  Serial.println();
  Serial.print("SERVO_ID = ");
  Serial.println(SERVO_ID);
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
  if(sms_sts.FeedBack(SERVO_ID)!=-1){
  /*
    Pos = sms_sts.ReadPos(-1);
    Speed = sms_sts.ReadSpeed(-1);
    Load = sms_sts.ReadLoad(-1);
    Voltage = sms_sts.ReadVoltage(-1);
    Temper = sms_sts.ReadTemper(-1);
    Move = sms_sts.ReadMove(-1);
    Current = sms_sts.ReadCurrent(-1);
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
  }else{
    Serial.println("FeedBack err");
    delay(500);
  }
  
  Pos = sms_sts.ReadPos(SERVO_ID);
  if(Pos!=-1){
    Serial.print("Servo position:");
    Serial.println(Pos, DEC);
    delay(10);
  }else{
    Serial.println("read position err");
    delay(500);
  }
  
  Voltage = sms_sts.ReadVoltage(SERVO_ID);
  if(Voltage!=-1){
	Serial.print("Servo Voltage:");
    Serial.println(Voltage, DEC);
    delay(10);
  }else{
    Serial.println("read Voltage err");
    delay(500);
  }
  
  Temper = sms_sts.ReadTemper(SERVO_ID);
  if(Temper!=-1){
    Serial.print("Servo temperature:");
    Serial.println(Temper, DEC);
    delay(10);
  }else{
    Serial.println("read temperature err");
    delay(500);    
  }

  Speed = sms_sts.ReadSpeed(SERVO_ID);
  if(Speed!=-1){
    Serial.print("Servo Speed:");
    Serial.println(Speed, DEC);
    delay(10);
  }else{
    Serial.println("read Speed err");
    delay(500);    
  }
  
  Load = sms_sts.ReadLoad(SERVO_ID);
  if(Load!=-1){
    Serial.print("Servo Load:");
    Serial.println(Load, DEC);
    delay(10);
  }else{
    Serial.println("read Load err");
    delay(500);    
  }
  
  Current = sms_sts.ReadCurrent(SERVO_ID);
  if(Current!=-1){
    Serial.print("Servo Current:");
    Serial.println(Current, DEC);
    delay(10);
  }else{
    Serial.println("read Current err");
    delay(500);    
  }

  Move = sms_sts.ReadMove(SERVO_ID);
  if(Move!=-1){
    Serial.print("Servo Move:");
    Serial.println(Move, DEC);
    delay(10);
  }else{
    Serial.println("read Move err");
    delay(500);    
  }
  Serial.println();
  delay(3000);
}
