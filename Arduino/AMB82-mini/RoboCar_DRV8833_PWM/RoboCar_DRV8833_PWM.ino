/*
   DRV8833 dual DC-Motors Driving Test
*/

// nSLEEP pin connected to Vcc
// nFAULT pin kept floating
#define IN1_PIN     5 // IN1 pin PWM
#define IN2_PIN     6 // IN2 pin PWM
#define IN3_PIN     7 // IN3 pin PWM
#define IN4_PIN     8 // IN4 pin PWM

#define FULLSPEED 255
#define HALFSPEED 128
#define STOPSPEED 0

void setup()
{
    Serial.begin(115200);
    // set 4 GPIOs to OUTPUT mode
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(IN3_PIN, OUTPUT);
    pinMode(IN4_PIN, OUTPUT);    

    Serial.println("DRV8833 GPIO test with dual DC-motors");

    Serial.println("Stop Motors...");
    stop();
    delay(1000);

    Serial.println("Forward...");
    forward();
    delay(1000);
    stop();
    delay(1000);

    Serial.println("Backward...");
    backward();
    delay(1000);
    stop();
    delay(1000);

    Serial.println("Turn-Left...");
    turnleft();
    delay(1000);
    stop();
    delay(1000);    

    Serial.println("Turn-Right...");
    turnright();
    delay(1000);
    stop();
    delay(1000); 
}

void forward(){
  analogWrite(IN1_PIN, FULLSPEED);
  analogWrite(IN2_PIN, STOPSPEED);

  analogWrite(IN3_PIN, FULLSPEED);
  analogWrite(IN4_PIN, STOPSPEED);  
}

void backward(){
  analogWrite(IN1_PIN, STOPSPEED);
  analogWrite(IN2_PIN, FULLSPEED);

  analogWrite(IN3_PIN, STOPSPEED);
  analogWrite(IN4_PIN, FULLSPEED);
}

void turnleft(){
  analogWrite(IN1_PIN, FULLSPEED);
  analogWrite(IN2_PIN, STOPSPEED);

  analogWrite(IN3_PIN, STOPSPEED);
  analogWrite(IN4_PIN, FULLSPEED);
}

void turnright(){
  analogWrite(IN1_PIN, STOPSPEED);
  analogWrite(IN2_PIN, FULLSPEED);

  analogWrite(IN3_PIN, FULLSPEED);
  analogWrite(IN4_PIN, STOPSPEED);
}

void stop(){
  analogWrite(IN1_PIN, STOPSPEED);
  analogWrite(IN2_PIN, STOPSPEED);

  analogWrite(IN3_PIN, STOPSPEED);
  analogWrite(IN4_PIN, STOPSPEED);
}

void loop()
{
}
