# 📡 DecentIoT MQTT Library – GPS Module Guide

## 🎯 Overview
The **GPS module** adds zero‑RAM NMEA parsing and convenient helpers to the DecentIoT MQTT library. It enables you to:
- Parse raw NMEA sentences directly from a GPS receiver (e.g. UART, SoftwareSerial).
- Access latitude, longitude, altitude, speed, timestamp and accuracy.
- Publish GPS data to MQTT in a pipe‑delimited string or via a structured `GPSData` object.
- Use the built‑in `DecentIoTGps` parser **without any external dependencies** (no TinyGPS++, no extra heap allocation).

## Prerequisites
- **ESP8266** (NodeMCU, Wemos D1 Mini, etc.)
- A GPS module that streams NMEA sentences over a serial interface (e.g., `SoftwareSerial`).
- A Firebase Realtime Database project with the **Database URL** and **Database Secret** (or API key) ready.
- The **DecentIoT Firebase library** already added to your PlatformIO project (`lib_deps` or local copy).

## Wiring
| GPS Pin | ESP8266 Pin |
|---------|-------------|
| VCC     | 3.3 V / Vin (or external 3.3 V regulator) |
| GND     | GND |
| TX      | D5 (GPIO14) – **RX** for ESP8266 |
| RX      | D6 (GPIO12) – **TX** for ESP8266 |

> **Tip:** Power the GPS module from a dedicated 3.3 V regulator if you experience brown‑outs during the SSL handshake.


### `DecentIoTGps`
A lightweight, **zero‑RAM** NMEA parser that only keeps the last valid RMC sentence.
- No dynamic memory allocation – the entire parser lives in a fixed 83‑byte buffer.
- Exposes `encode(char c)` which should be fed one byte at a time (e.g. from `Serial.read()`).
- After a complete RMC sentence is parsed you can query the fields via the getter methods:
```cpp
bool   hasFix()    const;   // true if a valid fix was obtained
float  latitude()  const;
float  longitude() const;
float  altitude()  const;   // may be NAN
float  speed()     const;   // km/h, may be NAN
String time()      const;   // hhmmss UTC string
```
---
## 🚀 Using GPS with `DecentIoTClass`
### 1️⃣ Include the header
```cpp
#include "DecentIoT.h"
```
### 2️⃣ Feed raw NMEA data
```cpp
// Example: reading from Serial (UART) on ESP8266/ESP32
void loop() {
    while (Serial.available()) {
        char c = Serial.read();
        DecentIoT.feedGPS(c);           // <-- feeds the parser byte‑by‑byte
    }
    // … other MQTT handling …
}
```
### 3️⃣ Publish the raw parser state (pipe‑delimited string)
```cpp
// Publish the current GPS state on pin "P0"
DecentIoT.writeGPS("P0");
```
The library builds a payload like:
```
latitude|longitude|altitude|speed|timestamp
```
*Empty fields are omitted when the corresponding value is `NAN`.*

### 4️⃣ Publish a custom `GPSData` struct
```cpp
GPSData myPos(37.7749, -122.4194, 15.2, 45.0, 3.0);
DecentIoT.writeGPS("P0", myPos);
```
### 5️⃣ Publish individual coordinates without a struct
```cpp
DecentIoT.writeGPS("P0", 37.7749, -122.4194);
```
You can still pass optional altitude, speed and accuracy as extra arguments.
---
## 📚 Full API Reference (excerpt)
| Method | Description |
|--------|-------------|
| `void feedGPS(char c)` | Feed a single NMEA character to the internal parser. |
| `void writeGPS(const char *pin)` | Publish the **internal parser state** (`DecentIoTGps`) as a pipe‑delimited string. |
| `void writeGPS(const char *pin, const GPSData &gpsData)` | Publish a **user‑provided** `GPSData` struct. |
| `void writeGPS(const char *pin, float lat, float lng, float altitude = NAN, float speed = NAN, float accuracy = NAN)` | Publish coordinates directly. |
| `bool DecentIoTGps::encode(char c)` | Returns `true` when a complete, checksum‑validated RMC sentence has been parsed. |
| `bool DecentIoTGps::hasFix() const` | Indicates whether a valid GPS fix is available. |
| `float DecentIoTGps::latitude() const` | Latitude in decimal degrees. |
| `float DecentIoTGps::longitude() const` | Longitude in decimal degrees. |
| `float DecentIoTGps::altitude() const` | Altitude in meters (*may be NAN*). |
| `float DecentIoTGps::speed() const` | Speed in km/h (*may be NAN*). |
| `String DecentIoTGps::time() const` | UTC time string `hhmmss`. |
---
## 🛠️ Example Sketch (ESP8266)
```cpp
#include <ESP8266WiFi.h>
#include "DecentIoT.h"
#include <SoftwareSerial.h>

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASS";

// GPS module on pins D6 (RX) and D5 (TX)
SoftwareSerial gpsSerial(D6, D5); // RX, TX

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) delay(500);

    // Initialise DecentIoT (MQTT broker, project, user, device)
    DecentIoT.begin("mqtt.example.com", 1883, "mqtt_user", "mqtt_pass",
                    "demoProject", "demoUser", "demoDevice");

    // Start GPS serial communication
    gpsSerial.begin(9600);
}

// Feed GPS data and publish to DecentIoT dashboard every 10 s
DECENTIOT_SEND(P5, 10000) {
    // Feed incoming NMEA characters
    while (gpsSerial.available()) {
        DecentIoT.feedGPS(gpsSerial.read());
    }

    if (DecentIoT.gps.hasFix()) {
        // Publish the current GPS state on virtual pin P5 (dashboard integration)
        DecentIoT.writeGPS(P5);
    // Example: retrieve individual coordinates
    float lat = DecentIoT.gps.latitude();
    float lng = DecentIoT.gps.longitude();
    } else {
        Serial.println("[GPS] Waiting for satellite fix...");
    }
}

```
---
## 📦 Build & Integration
1. **Clone the repository** and place it under your Arduino/PlatformIO `src/` folder.
2. The library already includes the required **WiFiClientSecure** and **PubSubClient** dependencies – no extra `#include`s are needed.
3. Ensure your board’s UART pins are wired to the GPS module (RX → TX of GPS, TX → RX of GPS). Adjust the `Serial1` (or `SoftwareSerial`) instance accordingly.
4. Compile and upload – the GPS parser works out‑of‑the‑box, requiring only the `feedGPS` call in your main loop.
---
## ❓ FAQ
**Q: Does the parser consume RAM?**
- A: Only 83 bytes for the internal buffer plus a few `float`s. It is deliberately **zero‑RAM**.

**Q: Which NMEA sentences are supported?**
- A: Currently only the `$--RMC` sentence (recommended minimum data) is parsed. It provides time, fix status, latitude, longitude, speed and date.

**Q: What happens if the GPS has no fix?**
- `hasFix()` returns `false`. `writeGPS()` will emit a warning (`⚠️  No GPS fix, skipping GPS message`) and no payload is sent.

**Q: Can I use TinyGPS++ together with this library?**
- **Yes.** If you already use TinyGPS++ for richer parsing (e.g., additional fields like altitude, speed, course, HDOP), `DecentIoT.writeGPS_TinyGPSPlus()` provides a convenient bridge. It accepts a `TinyGPSPlus` instance, extracts the relevant data, builds a `GPSData` struct, and forwards it to the MQTT payload, so you don’t have to manually map each field. For simple use‑cases where you only need the built‑in parser, the regular `writeGPS()` handler is sufficient.
---

## 🔧 Manual conversion without helper
```cpp
// From TinyGPS++ (or any other source) you have latitude/longitude etc.
GPSData myPos;
myPos.latitude  = gps.location.lat();
myPos.longitude = gps.location.lng();
if (gps.altitude.isValid())  myPos.altitude = gps.altitude.meters();
if (gps.speed.isValid())     myPos.speed    = gps.speed.kmph();
if (gps.hdop.isValid())      myPos.accuracy = gps.hdop.hdop() * 5.0f;
myPos.timestamp = time(nullptr);

DecentIoT.writeGPS("P5", myPos);   // same result as the helper
```

## 📌 Summary
- The GPS module is **self‑contained**, **zero‑RAM**, and provides both a low‑level parser (`DecentIoTGps`) and high‑level publishing helpers (`DecentIoTClass::writeGPS`).
- Feed raw data via `feedGPS(char)`, then publish using one of the three overloads of `writeGPS`.
- Use the `GPSData` struct for custom payloads or simply call `writeGPS(pin)` to broadcast the internal parser’s state.

Happy coding! 🚀
