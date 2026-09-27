/*
Example : AMB82-mini send text (3 times) to Ollama server (Gemma4) on PC.
*/
#include <ArduinoJson.h> 
#include <WiFi.h>
JsonDocument doc;
WiFiClient client;

uint32_t img_addr = 0;
uint32_t img_len = 0;

char ssid[] = "HITRON-DF90-2.4G";    // your network SSID (name)
char pass[] = "0972211921";        // your network password
int status = WL_IDLE_STATUS;     // Indicator of Wifi status

const char *myDomain = "192.168.0.199"; // Ollama Server IP address
String model = "gemma4:e2b";
String ollama_key = "f667e17c75bf48a99a27b75c20063efd";

int send_count = 0;

void setup()
{
    Serial.begin(115200);

    // Attempt to connect to WiFi network
    while (status != WL_CONNECTED) {
        Serial.print("\r\nAttempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }
    send_Ollama("What model are you?");
}

void loop()
{
}

void send_Ollama(String prompt)
{
    String getResponse = "", Feedback = "";
    Serial.println("Connect to " + String(myDomain));
    if (client.connect(myDomain, 11434)) {
        Serial.println("Connection successful");

        //String Data = "{\"model\": \"" + model + "\", \"messages\": [{\"role\": \"user\",\"content\": \"" + prompt + "\"}] }";
        String Data = "{\"model\": \"" + model + "\", \"messages\": [{\"role\": \"user\",\"content\": [{ \"type\": \"text\", \"text\": \"" + prompt + "\"}]}]}";
        Serial.println("POST");
        client.println("POST /v1/chat/completions HTTP/1.1");
        client.println("Host: " + String(myDomain));
        client.println("Authorization: Bearer " + ollama_key);
        client.println("Content-Type: application/json; charset=utf-8");
        client.println("Content-Length: " + String(Data.length()));
        client.println("Connection: close");
        Serial.println("CLOSE");
        client.println();

        unsigned int Index;
        for (Index = 0; Index < Data.length(); Index = Index + 1024) {
            client.print(Data.substring(Index, Index + 1024));
        }

        Serial.println("Receive");
        uint32_t waitTime = 10000;
        uint32_t startTime = millis();
        boolean state = false;
        boolean markState = false;

        while ((startTime + waitTime) > millis()) {
            Serial.print(".");
            delay(1000);
        }

        while (client.available()) {
            char c = client.read();
            if (String(c) == "{") {
                markState = true;
            }
            if (state == true && markState == true) {
                Feedback += String(c);
            }
            if (c == '\n') {
                if (getResponse.length() == 0) {
                    state = true;
                }
                getResponse = "";
            } else if (c != '\r') {
                getResponse += String(c);
            }
        }

        if (Feedback.length() > 0) {
            deserializeJson(doc, Feedback);
            String content = doc["choices"][0]["message"]["content"];
            Serial.println(content);           
        }
        Serial.println();
        client.stop();
    }
}
