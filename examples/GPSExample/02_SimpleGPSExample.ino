#include <DecentIoT.h>
#include <WiFi.h>

// MQTT Broker settings
#define MQTT_BROKER "your-broker.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USERNAME "your-mqtt-username"
#define MQTT_PASSWORD "your-mqtt-password"
#define PROJECT_ID "my-iot-project"
#define USER_ID "user123"
#define DEVICE_ID "esp32-simple-gps"
// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

// Simulated GPS data (replace with real GPS sensor data)
float currentLat = 40.7128;   // New York City
float currentLng = -74.0060;
float currentSpeed = 25.5;    // km/h
float currentAccuracy = 5.0;  // meters

// Send GPS location every 10 seconds
DECENTIOT_SEND_GPS(P0, 10000)
{
    // Simulate GPS movement (replace with real GPS data)
    currentLat += random(-10, 11) * 0.0001;   // Small random movement
    currentLng += random(-10, 11) * 0.0001;
    currentSpeed = random(0, 80);              // Random speed 0-80 km/h
    currentAccuracy = random(3, 15);           // Random accuracy 3-15m

    // Method 1: Send GPS data using individual parameters
    DecentIoT.writeGPS(P0, currentLat, currentLng, NAN, currentSpeed, currentAccuracy);

    Serial.printf("[GPS] Sent: Lat=%.6f, Lng=%.6f, Speed=%.1f km/h, Acc=%.1fm\n",
                 currentLat, currentLng, currentSpeed, currentAccuracy);
}

// Alternative: Send GPS data with altitude
DECENTIOT_SEND(P1, 15000)  // Every 15 seconds
{
    // Method 2: Send GPS data using GPSData struct
    GPSData location;
    location.latitude = currentLat;
    location.longitude = currentLng;
    location.altitude = 15.5;  // Fixed altitude for demo
    location.speed = currentSpeed;
    location.accuracy = currentAccuracy;

    DecentIoT.writeGPS(P1, location);

    Serial.printf("[GPS] Struct: Lat=%.6f, Lng=%.6f, Alt=%.1fm, Speed=%.1f km/h\n",
                 location.latitude, location.longitude, location.altitude, location.speed);
}


void setup()
{
    Serial.begin(115200);
    Serial.println("\n--- DecentIoT Simple GPS Demo ---");

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("[WiFi] Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[WiFi] Connected!");

    // Initialize DecentIoT
    DecentIoT.begin(MQTT_BROKER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD, PROJECT_ID, USER_ID, DEVICE_ID);

    Serial.println("[GPS] Simple GPS demo ready!");
    Serial.println("[GPS] Sending simulated GPS data every 10-15 seconds");
    Serial.println("[GPS] Use P2 to manually trigger GPS updates");
}

void loop()
{
    // Handle WiFi reconnection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Disconnected! Reconnecting...");
        WiFi.reconnect();
        delay(5000);
    }

    // Run DecentIoT (handles MQTT and scheduled GPS updates)
    DecentIoT.run();

    delay(100);
}