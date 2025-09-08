#include <DecentIoT.h>
#include <WiFi.h>

// MQTT Broker settings (using HiveMQ Cloud as example)
#define MQTT_BROKER "your-broker.hivemq.cloud"
#define MQTT_PORT 8883  // SSL port
#define MQTT_USERNAME "your-mqtt-username"
#define MQTT_PASSWORD "your-mqtt-password"
// Project settings
#define PROJECT_ID "my-iot-project"
#define USER_ID "user123"
#define DEVICE_ID "esp32-device-001"
// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

// Pin definitions
#define LED_PIN D6  
#define TEMP_SENSOR 36 // ADC1_CH0 on ESP32

// LED state handler
DECENTIOT_RECEIVE(P0)
{
  digitalWrite(LED_PIN, value);
  Serial.printf("[P0] LED state = %s\n", value ? "ON" : "OFF");
}

DECENTIOT_SEND(P1, 15000)
{
  // Generate random temperature value between 0-100
  int temperature = random(0, 101);
  DecentIoT.write(P1, temperature);
  Serial.printf("[P1] Temperature = %d\n", temperature);
}

DECENTIOT_SEND(P2, 15000)
{
  // Generate random humidity value between 0-100
  int humidity = random(0, 101);
  DecentIoT.write(P2, humidity);
  Serial.printf("[P2] Humidity = %d\n", humidity);
}

DECENTIOT_RECEIVE(P3)
{
  int sliderValue = value;
  int pwmValue = sliderValue * 255 / 100;
  analogWrite(rgbPin, pwmValue);
  Serial.printf("[P3] RGB Brightness = %d (slider: %d)\n", pwmValue, sliderValue);
}

// Sending a message from device to cloud
DECENTIOT_SEND(P4, 10000)
{
  static int count = 0;
  String message = "ESP8266 Connected! Count: " + String(count++);
  DecentIoT.write(P4, message.c_str());
  Serial.println("[P4] Sent: " + message);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\n--- Initializing DecentIoT Device ---");

  // Initialize LED pin
  pinMode(ledPin, OUTPUT);
  pinMode(rgbPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(ledPin, LOW); // Ensure LED is off initially
  digitalWrite(LED_BUILTIN, LOW);

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
  // User checks WiFi and reconnects if needed
  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("[WiFi] Disconnected! Reconnecting...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    // Optionally: wait for connection, show status, etc.
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
      delay(500);
      Serial.print(".");
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WiFi reconnected! Restarting device...");
      delay(1000);
      ESP.restart();
    }
  }

  DecentIoT.run();
  delay(10);
}