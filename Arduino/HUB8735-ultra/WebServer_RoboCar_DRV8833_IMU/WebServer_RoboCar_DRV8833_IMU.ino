/*
   RoboCar : WebServer + DRV8833 + MPU6050
*/

#include <WiFi.h>
#include "I2Cdev.h"
#include <MPU6050_IMU_libraries/MPU6050_6Axis_MotionApps612.h>
#include "Wire.h"

char ssid[] = "TCFSTWIFI.ALL";    // your network SSID (name)
char pass[] = "035623116";        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;     // Indicator of Wifi status

WiFiServer server(80);

MPU6050 mpu;
//MPU6050 mpu(0x69); // <-- use for AD0 high

bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[128]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

String commandState = "stop";

// nSLEEP pin connected to Vcc
// nFAULT pin kept floating
#define IN1_PIN     7 // IN1 pin GPIO
#define IN2_PIN    11 // IN2 pin GPIO
#define IN3_PIN     8 // IN3 pin GPIO need 10K ohm pull-up
#define IN4_PIN    12 // IN4 pin GPIO

void init_imu()
{
    Serial.println("initializing MPU...");
    mpu.initialize();

    // verify connection
    Serial.println(F("Testing device connections..."));
    Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

    // load and configure the DMP
    Serial.println(F("Initializing DMP..."));
    devStatus = mpu.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(51);
    mpu.setYGyroOffset(8);
    mpu.setZGyroOffset(21);
    mpu.setXAccelOffset(1150);
    mpu.setYAccelOffset(-50);
    mpu.setZAccelOffset(1060);

    // make sure it worked (returns 0 if so)
    if (devStatus == 0) {
        // Calibration Time: generate offsets and calibrate our MPU6050
        mpu.CalibrateAccel(6);
        mpu.CalibrateGyro(6);
        Serial.println();
        mpu.PrintActiveOffsets();
        // turn on the DMP, now that it's ready
        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        mpuIntStatus = mpu.getIntStatus();
        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        Serial.println(F("DMP ready! Waiting for first interrupt..."));
        dmpReady = true;

        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();
    } else {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        Serial.print(F("DMP Initialization failed (code "));
        Serial.print(devStatus);
        Serial.println(F(")"));
    }
}

int  GetHeading()                                                                                                                                                   
{
    if (!dmpReady) {
        return(0);
    }
    // read a packet from FIFO
    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {    // Get the Latest packet
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
        Serial.print("Yaw = ");
        Serial.println(int((ypr[0] * 180/M_PI)));
        return(int((ypr[0] * 180/M_PI)) + 180);
    }
}

void cmd_F() {
  digitalWrite(IN1_PIN, HIGH); 
  digitalWrite(IN2_PIN, LOW); 
  digitalWrite(IN3_PIN, LOW); 
  digitalWrite(IN4_PIN, HIGH); 
}

void cmd_B() {
  digitalWrite(IN1_PIN, LOW); 
  digitalWrite(IN2_PIN, HIGH); 
  digitalWrite(IN3_PIN, HIGH); 
  digitalWrite(IN4_PIN, LOW);  
}

void cmd_R() {
  digitalWrite(IN1_PIN, HIGH); 
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(IN3_PIN, HIGH); 
  digitalWrite(IN4_PIN, LOW);
}

void cmd_L() {
  digitalWrite(IN1_PIN, LOW); 
  digitalWrite(IN2_PIN, HIGH);
  digitalWrite(IN3_PIN, LOW); 
  digitalWrite(IN4_PIN, HIGH);
}

void cmd_S() {
  digitalWrite(IN1_PIN, HIGH); 
  digitalWrite(IN2_PIN, HIGH);
  digitalWrite(IN3_PIN, HIGH); 
  digitalWrite(IN4_PIN, HIGH);  
}

void setup()
{
    Serial.begin(115200);        // initialize serial communication
    Wire.begin();                // configure I2C interface 
    Wire.setClock(400000); 

    pinMode(LED_BUILTIN, OUTPUT); // configure LED for output
    digitalWrite(LED_BUILTIN, LOW);

    // set 4 GPIOs to OUTPUT mode
    pinMode(IN1_PIN, OUTPUT);
    pinMode(IN2_PIN, OUTPUT);
    pinMode(IN3_PIN, OUTPUT);
    pinMode(IN4_PIN, OUTPUT);

    Serial.println("Motor0 & 1 Stop...");
    digitalWrite(IN1_PIN, HIGH); 
    digitalWrite(IN2_PIN, HIGH);
    digitalWrite(IN3_PIN, HIGH); 
    digitalWrite(IN4_PIN, HIGH);
    delay(1000);

    init_imu();

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to Network named: ");
        Serial.println(ssid);    // print the network name (SSID);

        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);
        // wait 10 seconds for connection:
        delay(10000);
    }
    server.begin();       // start the web server on port 80

    digitalWrite(LED_BUILTIN, HIGH); // web server & mpu are ready
}

void loop()
{ 
    WiFiClient client = server.available();    // listen for incoming clients

    if (client) {                          // if you get a client,
        Serial.println("new client");      // print a message out the serial port
        String currentLine = "";           // make a String to hold incoming data from the client        
        int Heading = GetHeading();
        while (client.connected()) {       // loop while the client's connected
            if (client.available()) {      // if there's bytes to read from the client,
                char c = client.read();    // read a byte, then
                Serial.write(c);           // print it out the serial monitor
                if (c == '\n') {           // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0) {
                        // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                        // and a content-type so the client knows what's coming, then a blank line:
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();

                        // Display the HTML web page
                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");
                        // web header
                        client.println("<body><h1>RoboCar</h1>");
                        // Display current state  
                        client.println("<p>Motion  = " + commandState + "</p>");
                        client.println("<p>Heading = " + String(Heading) +"</p>");                       

                        // The HTTP response ends with another blank line:
                        client.println();
                        // break out of the while loop:
                        break;
                    } else {    // if you got a newline, then clear currentLine:
                        currentLine = "";
                    }
                } else if (c != '\r') {    // if you got anything else but a carriage return character,
                    currentLine += c;      // add it to the end of the currentLine
                }

                // Check to see if the client request was "GET /H" or "GET /L":
                if (currentLine.endsWith("GET /F")) {
                    cmd_F();
                    commandState="FORWARD";
                }
                if (currentLine.endsWith("GET /B")) {
                    cmd_B();
                    commandState="BACKWARD";
                }
                if (currentLine.endsWith("GET /L")) {
                    cmd_L();
                    commandState="LEFT";
                }
                if (currentLine.endsWith("GET /R")) {
                    cmd_R();
                    commandState="RIGHT";
                }           
                if (currentLine.endsWith("GET /S")) {
                    cmd_S();
                    commandState="STOP";
                }                          
            }
        }
        // close the connection:
        client.stop();
        Serial.println("client disconnected");
    }
}
