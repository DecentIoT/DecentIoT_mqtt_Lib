#include <DecentIoT.h>
#include <WiFi.h>

// MQTT Broker settings (using HiveMQ Cloud as example)
#define MQTT_BROKER "your-broker.hivemq.cloud"
#define MQTT_PORT 8883  // SSL port
#define MQTT_USERNAME "your-mqtt-username"
#define MQTT_PASSWORD "your-mqtt-password"
#define PROJECT_ID "my-iot-project"
#define USER_ID "user123"
#define DEVICE_ID "esp32-device-001"

// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

// Pin definitions
const int ledPin = 2;

// LED state handler
DECENTIOT_RECEIVE(P0)
{
  digitalWrite(ledPin, value);
  Serial.printf("[P0] LED state = %s\n", value ? "ON" : "OFF");
}


void setup()
{
  Serial.begin(115200);
  Serial.println("\n--- Initializing DecentIoT Device ---");

  // Initialize LED pin
  pinMode(ledPin, OUTPUT);  
  digitalWrite(ledPin, LOW); // Ensure LED is off initially


  // User handles WiFi connection
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  Serial.println("WiFi connected!");
  digitalWrite(LED_BUILTIN, HIGH);

  // Now start DecentIoT (cloud connection)
  DecentIoT.begin(MQTT_BROKER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD, PROJECT_ID, USER_ID, DEVICE_ID);
}

void loop()
{
  DecentIoT.run();
  delay(10);
}