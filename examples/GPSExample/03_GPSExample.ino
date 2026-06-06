#include <DecentIoT.h>
#include <WiFi.h>
#include <TinyGPS++.h>  // Popular GPS library for NEO-6M, NEO-8N modules
#include <HardwareSerial.h>

// MQTT Broker settings (using HiveMQ Cloud as example)
#define MQTT_BROKER "your-broker.hivemq.cloud"
#define MQTT_PORT 8883  // SSL port
#define MQTT_USERNAME "your-mqtt-username"
#define MQTT_PASSWORD "your-mqtt-password"
#define PROJECT_ID "my-iot-project"
#define USER_ID "user123"
#define DEVICE_ID "esp32-gps-device"
// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

// GPS Hardware Setup
#define GPS_SERIAL_NUM 2  // Use Serial2 for GPS
#define GPS_RX_PIN 16     // GPS RX pin
#define GPS_TX_PIN 17     // GPS TX pin
#define GPS_BAUD 9600     // GPS module baud rate

// GPS objects
TinyGPSPlus gps;
HardwareSerial gpsSerial(GPS_SERIAL_NUM);

// GPS Data sending with automatic interval (every 10 seconds)
DECENTIOT_SEND_GPS(P0, 10000)
{
    if (gps.location.isValid()) {
        // Use TinyGPSPlus helper method (automatic field mapping)
        DecentIoT.writeGPS_TinyGPSPlus(P0, gps);
    } else {
        Serial.println("[GPS] Location not valid yet");
    }
}

// Alternative: Manual GPS data sending (custom logic)
DECENTIOT_SEND(P1, 5000)  // Send every 5 seconds
{
    if (gps.location.isValid()) {
        // Manual GPS data with custom fields
        float lat = gps.location.lat();
        float lng = gps.location.lng();
        float alt = gps.altitude.isValid() ? gps.altitude.meters() : NAN;
        float speed = gps.speed.isValid() ? gps.speed.kmph() : NAN;
        
        // Send GPS data using structured method
        DecentIoT.writeGPS(P1, lat, lng, alt, speed);

        Serial.printf("[GPS] Manual: Lat=%.6f, Lng=%.6f, Alt=%.1f, Speed=%.1f\n",
                     lat, lng, alt, speed);
    }
}

// Get GPS data from cloud
DECENTIOT_RECEIVE(P2)
{
    if (value) {  // Button pressed
        if (gps.location.isValid()) {
            // Send GPS data using GPSData struct
            GPSData location;
            location.latitude = gps.location.lat();
            location.longitude = gps.location.lng();
            location.altitude = gps.altitude.meters();
            location.speed = gps.speed.kmph();
            
            DecentIoT.writeGPS(P0, location);
            Serial.println("[GPS] Manual GPS data sent via button press");
        } else {
            Serial.println("[GPS] GPS not ready for manual send");
        }
    }
}


void setup()
{
    Serial.begin(115200);
    Serial.println("\n--- DecentIoT GPS Tracking Device ---");

    // Initialize GPS serial
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("[GPS] GPS serial initialized");

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("[WiFi] Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[WiFi] Connected successfully!");
    Serial.printf("[WiFi] IP Address: %s\n", WiFi.localIP().toString().c_str());

    // Initialize DecentIoT
    DecentIoT.begin(MQTT_BROKER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD, PROJECT_ID, USER_ID, DEVICE_ID);

    Serial.println("[DecentIoT] GPS tracking device ready!");
    Serial.println("[GPS] Waiting for satellite fix...");
}

void loop()
{
    // Handle WiFi reconnection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Disconnected! Reconnecting...");
        WiFi.reconnect();
        delay(5000);
    }

    // Process GPS data
    while (gpsSerial.available() > 0) {
        char c = gpsSerial.read();
        if (gps.encode(c)) {
            // GPS data was successfully parsed
            if (gps.location.isUpdated()) {
                // GPS location was updated - this happens once per second with valid fix
                Serial.printf("[GPS] Updated: Lat=%.6f, Lng=%.6f, Sat=%d, HDOP=%.1f\n",
                             gps.location.lat(), gps.location.lng(),
                             gps.satellites.value(), gps.hdop.hdop());
            }
        }
    }

    // Show GPS status every 10 seconds
    static unsigned long lastGpsStatus = 0;
    if (millis() - lastGpsStatus > 10000) {
        lastGpsStatus = millis();
        Serial.printf("[GPS] Status: Valid=%s, Satellites=%d, Age=%dms\n",
                     gps.location.isValid() ? "YES" : "NO",
                     gps.satellites.value(),
                     gps.location.age());
    }

    // Run DecentIoT (handles MQTT, scheduling, etc.)
    DecentIoT.run();

    delay(10);  // Small delay to prevent overwhelming the CPU
}