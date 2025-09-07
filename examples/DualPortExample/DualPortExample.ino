#include <DecentIoT.h>
#include <WiFi.h>

// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

// MQTT Broker settings (HiveMQ Cloud example)
#define MQTT_BROKER "your-broker.hivemq.cloud"
#define MQTT_USERNAME "your-mqtt-username"
#define MQTT_PASSWORD "your-mqtt-password"

// Project settings
#define PROJECT_ID "my-iot-project"
#define USER_ID "user123"
#define DEVICE_ID "esp32-device-001"

// Pin definitions
const int LED_PIN = 2;

// MQTT over SSL/TLS (port 8883) - the only supported connection type
#define USE_PORT_8883  // MQTT over SSL/TLS

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    
    Serial.println("DecentIoT MQTT TLS Example");
    Serial.println("===========================");
    
    // Connect to WiFi
    Serial.println("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    
    // MQTT over SSL/TLS connection
    int mqttPort = 8883;
    String connectionType = "MQTT over SSL/TLS";
    Serial.println("Using port 8883: MQTT over SSL/TLS");
    
    Serial.printf("Connection type: %s\n", connectionType.c_str());
    Serial.printf("Broker: %s:%d\n", MQTT_BROKER, mqttPort);
    
    // Initialize DecentIoT with SSL/TLS
    DecentIoT.begin(PROJECT_ID, USER_ID, DEVICE_ID, 
                   MQTT_BROKER, mqttPort, MQTT_USERNAME, MQTT_PASSWORD);
    
    Serial.println("Connecting to MQTT broker...");
    
    // Wait for connection
    while (!DecentIoT.connected()) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nConnected to MQTT broker!");
    
    // Register handlers for incoming messages
    DecentIoT.onReceive("led", [](const DecentIoTValue& value) {
        bool ledState = value; // Automatic conversion
        digitalWrite(LED_PIN, ledState);
        Serial.printf("LED set to: %s\n", ledState ? "ON" : "OFF");
    });
    
    DecentIoT.onReceive("brightness", [](const DecentIoTValue& value) {
        int brightness = value; // Automatic conversion
        analogWrite(LED_PIN, brightness);
        Serial.printf("LED brightness set to: %d\n", brightness);
    });
    
    // Register handlers for sending data
    DecentIoT.onSend("temperature", []() {
        float temp = readTemperature();
        DecentIoT.write("temperature", temp);
        Serial.printf("Temperature sent: %.2f°C\n", temp);
    });
    
    DecentIoT.onSend("humidity", []() {
        float humidity = readHumidity();
        DecentIoT.write("humidity", humidity);
        Serial.printf("Humidity sent: %.1f%%\n", humidity);
    });
    
    // Publish initial status
    DecentIoT.publishStatus("online");
    Serial.println("Device is online and ready!");
    Serial.printf("Security: %s\n", DecentIoT.isSecure() ? "SSL/TLS Enabled" : "Insecure");
}

void loop() {
    // Handle MQTT operations
    DecentIoT.run();
    
    // Send sensor data every 30 seconds
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 30000) {
        DecentIoT.write("temperature", readTemperature());
        DecentIoT.write("humidity", readHumidity());
        lastSend = millis();
    }
    
    delay(100);
}

// Helper function to read temperature (simulated)
float readTemperature() {
    // Simulate temperature reading
    // Replace with your actual sensor code
    return 25.0 + random(-5, 5) + (random(0, 100) / 100.0);
}

// Helper function to read humidity (simulated)
float readHumidity() {
    // Simulate humidity reading
    // Replace with your actual sensor code
    return 60.0 + random(-10, 10) + (random(0, 100) / 100.0);
}
