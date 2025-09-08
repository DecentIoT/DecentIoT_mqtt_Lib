/*
  DecentIoT MQTT Library - Simple DHT Sensor Example
  
  Hardware:
  - DHT22 sensor on pin D4
  - LED on pin D6
  
  Virtual Pins:
  - P0: LED Control (receive)
  - P1: Temperature (send)
  - P2: Humidity (send)
*/

#include <DecentIoT.h>
#include <ESP8266WiFi.h>
#include <DHT.h>

// Configuration
#define MQTT_BROKER "your-mqtt-cluster-url"
#define MQTT_PORT 8883
#define MQTT_USERNAME "broker-access-username"
#define MQTT_PASSWORD "broker-access-pass"
#define PROJECT_ID "your-project-id"
#define USER_ID "your-user-id"
#define DEVICE_ID "your-device-id"
#define WIFI_SSID "your-wifi-name"
#define WIFI_PASS "wifi-password"

// Hardware pins
#define LED_PIN D6
#define DHT_PIN D4

// Create DHT sensor
DHT dht(DHT_PIN, DHT22);

// LED control from dashboard
DECENTIOT_RECEIVE(P0)
{
    digitalWrite(LED_PIN, value);
    Serial.printf("[P0]LED: %s\n", value ? "ON" : "OFF");
}

// Send temperature every 10 seconds
DECENTIOT_SEND(P1, 10000)
{
    float temp = dht.readTemperature();
    DecentIoT.write(P1, temp);
    Serial.printf("[P1] Temperature: %.1f°C\n", temp);
}

// Send humidity every 10 seconds
DECENTIOT_SEND(P2, 10000)
{
    float humidity = dht.readHumidity();
    DecentIoT.write(P2, humidity);
    Serial.printf("[P2] Humidity: %.1f%%\n", humidity);
}

void setup()
{
    Serial.begin(115200);
    
    // Initialize pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize DHT sensor
    dht.begin();
    
    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("[WiFi] connected!");
    
    // Initialize DecentIoT
    DecentIoT.begin(MQTT_BROKER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD, PROJECT_ID, USER_ID, DEVICE_ID);
    
}

void loop()
{
    DecentIoT.run();
    delay(10);
}

/*
  Dashboard Setup:
  1. Create project in DecentIoT web dashboard
  2. Add MQTT broker credentials
  3. Create device and datastreams
  4. Add widgets:
     - P0: Button/Switch for LED control
     - P1: Gauge for temperature
     - P2: Gauge for humidity
  
  Hardware:
  - DHT22: VCC→3.3V, GND→GND, Data→D4
  - LED: Anode→D6, Cathode→GND (with resistor)
*/
