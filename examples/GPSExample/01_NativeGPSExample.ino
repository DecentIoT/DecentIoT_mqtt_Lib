#include <DecentIoT.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

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

// GPS Module Setup (ESP8266 NodeMCU)
// Connect GPS Module TX pin to ESP8266 D6 (GPIO14)
// Connect GPS Module RX pin to ESP8266 D5 (GPIO12)
SoftwareSerial gpsSerial(D6, D5); // RX, TX

// Send GPS location dynamically based on satellite fix status.
// The user can assign any virtual pin according to their choice.
DECENTIOT_SEND(P5, 10000) // Trigger every 10 seconds
{
    if (DecentIoT.gps.hasFix())
    {
        // One unified, zero-JSON call to pack "lat|lon|alt|speed|time" and publish to MQTT
        DecentIoT.writeGPS(P5);

        Serial.printf("[GPS] P5 Updated -> Lat: %.6f, Lon: %.6f, Speed: %.1f km/h, Time: %s\n",
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
    delay(2000); // Wait for Serial monitor to connect
    Serial.println("\n--- DecentIoT Real GPS Hardware Demo ---");

    // Initialize GPS Serial at 9600 baud rate (default for most GPS modules)
    gpsSerial.begin(9600);
    Serial.println("[GPS] SoftwareSerial initialized on D6 (RX) and D5 (TX) at 9600 baud");

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

    Serial.println("[GPS] Real GPS demo ready!");
}

void loop()
{
    // Handle WiFi reconnection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Disconnected! Reconnecting...");
        WiFi.reconnect();
        delay(5000);
    }

    // Feed raw bytes into the internal zero-RAM parser continuously
    while (gpsSerial.available() > 0) {
        DecentIoT.feedGPS(gpsSerial.read());
    }

    // Run DecentIoT (handles MQTT and scheduled tasks)
    DecentIoT.run();

    delay(10); // Small yield to prevent CPU starvation
}
