/*
   DRV8833 dual DC-Motors Driving Test
*/

// nSLEEP pin connected to Vcc
// nFAULT pin kept floating
#define IN1_PIN     7 // IN1 pin GPIO
#define IN2_PIN    11 // IN2 pin GPIO
#define IN3_PIN     8 // IN3 pin GPIO
#define IN4_PIN    12 // IN4 pin GPIO

void setup()
{
    Serial.begin(115200);

    // set 4 GPIOs to OUTPUT mode
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(IN3_PIN, OUTPUT);
    pinMode(IN4_PIN, OUTPUT);

    Serial.println("DRV8833 GPIO test with dual DC-motors");
    Serial.println("-------------------------------------");
    Serial.print("Motor0: ");
    Serial.print("IN1 pin= "); Serial.print(IN1_PIN); Serial.print(",\t");
    Serial.print("IN2 pin= "); Serial.print(IN2_PIN); Serial.print(";\t");
    Serial.print("Motor1: ");
    Serial.print("IN3 pin= "); Serial.print(IN3_PIN); Serial.print(",\t");
    Serial.print("IN4 pin= "); Serial.print(IN4_PIN); Serial.println(";\t");

    Serial.println();

    Serial.println("Motor0 & 1 Stop...");
    digitalWrite(IN1_PIN, HIGH); 
    digitalWrite(IN2_PIN, HIGH);
    digitalWrite(IN3_PIN, HIGH); 
    digitalWrite(IN4_PIN, HIGH);
    delay(1000);

    Serial.println("Motor0 Rotating CW...");
    digitalWrite(IN1_PIN, HIGH); 
    digitalWrite(IN2_PIN, LOW);     
    delay(1000);
    Serial.println("Motor0 Rotating CCW...");
    digitalWrite(IN1_PIN, LOW); 
    digitalWrite(IN2_PIN, HIGH);     
    delay(1000);

    Serial.println("Motor0 Stop...");
    digitalWrite(IN1_PIN, HIGH); 
    digitalWrite(IN2_PIN, HIGH);
    delay(1000);

    Serial.println("Motor1 Rotating CW...");
    digitalWrite(IN3_PIN, LOW); 
    digitalWrite(IN4_PIN, HIGH);     
    delay(1000);
    Serial.println("Motor1 Rotating CCW...");
    digitalWrite(IN3_PIN, HIGH); 
    digitalWrite(IN4_PIN, LOW);     
    delay(1000);
    Serial.println("Motor0 Stop...");
    digitalWrite(IN3_PIN, HIGH); 
    digitalWrite(IN4_PIN, HIGH);
    delay(1000);
}

void loop()
{
}
