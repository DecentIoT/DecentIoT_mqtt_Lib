#include <DecentIoT.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#define GPS_SERIAL swSerial
SoftwareSerial swSerial(D6, D5); // RX, TX
#elif defined(ESP32)
#include <WiFi.h>
#define GPS_SERIAL Serial2
#else
#include <WiFi.h>
#endif

// MQTT Broker settings
#define MQTT_BROKER "your-broker.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USERNAME "your-mqtt-username"
#define MQTT_PASSWORD "your-mqtt-password"
#define PROJECT_ID "my-iot-project"
#define USER_ID "user123"
#define DEVICE_ID "my-device-id"

// WiFi credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"

// Define a Virtual Pin for GPS tracking (P4 is used here as an example)
DECENTIOT_SEND(P4, 10000) // Send GPS data every 10 seconds
{
    // Check if the native GPS parser has a valid satellite fix
    if (DecentIoT.gps.hasFix())
    {
        // Pack and publish coordinates natively (latitude|longitude|altitude|speed|time)
        DecentIoT.writeGPS(P4);

        Serial.printf("[GPS] P4 Published -> Lat: %.6f, Lon: %.6f, Speed: %.1f km/h, Time: %s\n",
                      DecentIoT.gps.latitude(),
                      DecentIoT.gps.longitude(),
                      DecentIoT.gps.speed(),
                      DecentIoT.gps.time().c_str());
    }
    else
    {
        Serial.println("[GPS] Waiting for satellite fix...");
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- DecentIoT Native GPS Example ---");

    // Initialize GPS Serial connection (9600 is standard for NEO-6M / NEO-8M)
#ifdef ESP8266
    GPS_SERIAL.begin(9600);
#elif defined(ESP32)
    GPS_SERIAL.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
#endif
    Serial.println("[GPS] GPS Serial receiver initialized");

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("[WiFi] Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n[WiFi] Connected!");

    // Initialize DecentIoT connection
    DecentIoT.begin(MQTT_BROKER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD,
                   PROJECT_ID, USER_ID, DEVICE_ID);

    Serial.println("[GPS] Initialization complete. Waiting for GPS signals...");
}

void loop()
{
    // Handle WiFi reconnection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Disconnected! Reconnecting...");
        WiFi.reconnect();
        delay(5000);
    }

    // Feed raw serial bytes into the internal zero-RAM NMEA parser continuously
#if defined(ESP8266) || defined(ESP32)
    while (GPS_SERIAL.available() > 0) {
        DecentIoT.feedGPS(GPS_SERIAL.read());
    }
#endif

    // Run DecentIoT task loop (handles MQTT packets and telemetry scheduler)
    DecentIoT.run();

    delay(10);
}
