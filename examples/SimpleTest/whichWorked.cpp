#include <WiFi.h>
#include <OpenIoT.h>
#include <DHT.h>

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""
#define DEVICE_ID "test_device"

// HiveMQ Cluster settings
#define MQTT_BROKER "a643d147838c4c378363bc81f3051065.s1.eu.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USERNAME "Test0001"
#define MQTT_PASSWORD "Test0001"

// Pin definitions from diagram
#define LED1_PIN 13    // Green LED
#define LED2_PIN 12    // Yellow LED
#define DHT_PIN 4      // DHT22 sensor pin from diagram.json
#define DHT_TYPE DHT22 // Assuming DHT22 based on common use with ESP32

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Function to read sensor data and publish via MQTT
void readAndPublishSensorData()
{
  // Generate random values for testing
  float temperature = random(0, 101) / 1.0; // Random float between 0 and 100
  float humidity = random(0, 101) / 1.0;    // Random float between 0 and 100

  // Keep check for isnan to avoid potential issues, though random won't produce NaN
  if (!isnan(temperature))
  {
    OpenIoT::writeComponent("P2_Temp", temperature);
  }

  if (!isnan(humidity))
  {
    OpenIoT::writeComponent("P2_Hum", humidity);
  }
}

void setup()
{
  Serial.begin(115200);

  // Disable verbose SSL debug messages
  esp_log_level_set("*", ESP_LOG_ERROR);
  esp_log_level_set("ssl_client", ESP_LOG_ERROR);

  // Initialize pins and set default state
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW); // LEDs start off
  digitalWrite(LED2_PIN, LOW); // LEDs start off

  // Initialize DHT sensor (still needed for begin() call if using DHT library)
  dht.begin();

  // Initialize random seed
  randomSeed(analogRead(0)); // Use unconnected analog pin for randomness

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");

  // Initialize OpenIoT library with HiveMQ credentials
  OpenIoT::begin(DEVICE_ID, MQTT_BROKER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);

  // Add components that the device will manage (LEDS for receiving commands, sensors for publishing data)
  OpenIoT::addComponent("P0", "Green LED", COMPONENT_BOOLEAN);      // LED component - receives commands
  OpenIoT::addComponent("P1", "Yellow LED", COMPONENT_BOOLEAN);     // LED component - receives commands
  OpenIoT::addComponent("P2_Temp", "Temperature", COMPONENT_FLOAT); // Sensor component - publishes data
  OpenIoT::addComponent("P2_Hum", "Humidity", COMPONENT_FLOAT);     // Sensor component - publishes data

  // Set up callbacks for components that receive commands (LEDs)
  OpenIoT::onComponent("P0", static_cast<BooleanCallback>([](bool value)
                                                          {
                                                            digitalWrite(LED1_PIN, value);
                                                            Serial.print("Green LED: ");
                                                            Serial.println(value ? "ON" : "OFF"); }));

  OpenIoT::onComponent("P1", static_cast<BooleanCallback>([](bool value)
                                                          {
                                                            digitalWrite(LED2_PIN, value);
                                                            Serial.print("Yellow LED: ");
                                                            Serial.println(value ? "ON" : "OFF"); }));
}

void loop()
{
  // Keep the OpenIoT library running to handle MQTT messages and reconnections
  OpenIoT::run();

  // Read and publish sensor data periodically
  static unsigned long lastSensorUpdate = 0;
  const unsigned long SENSOR_UPDATE_INTERVAL = 5000; // Update sensor every 5 seconds (adjust as needed)

  if (millis() - lastSensorUpdate > SENSOR_UPDATE_INTERVAL)
  {
    // Only read and publish if connected to avoid blocking
    if (OpenIoT::connected())
    {
      readAndPublishSensorData();
    }
    lastSensorUpdate = millis();
  }
}