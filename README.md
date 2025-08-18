# DecentIoT MQTT Library

A secure and powerful MQTT library for IoT devices with built-in SSL/TLS certificate support. DecentIoT provides a simple way to connect IoT devices to MQTT brokers with automatic security validation, making it safe for production use in real-world projects.

## Features

- 🔒 **Built-in SSL/TLS Security** - Automatic certificate validation for secure connections
- 🌐 **Dual Port Support** - MQTT over SSL/TLS (8883) and WebSocket Secure (8884)
- 📡 **Real-time Communication** - Bi-directional data exchange
- 🛠️ **Type-Safe Callbacks** - Automatic data type conversion
- 📊 **JSON Support** - Built-in JSON parsing and creation
- ⚡ **Optimized for ESP32/ESP8266** - Efficient memory usage
- 🔄 **Production Ready** - Safe for real-world IoT deployments
- 🚫 **No Certificate Bypass** - Security cannot be disabled

## How It Works

1. Connect to your MQTT broker with SSL/TLS (port 8883) or WebSocket Secure (port 8884)
2. Register callbacks for incoming messages
3. Send data to specific topics
4. The library handles all security and connection management
5. Automatic certificate validation ensures secure communication

## Installation

1. Download the latest release from GitHub
2. In Arduino IDE: Sketch -> Include Library -> Add .ZIP Library
3. Select the downloaded zip file

## Security Features

🔒 **SSL/TLS Certificate Validation**: The library includes a Let's Encrypt root certificate that automatically validates MQTT broker certificates.

🔒 **Dual Port Support**: Supports both MQTT over SSL/TLS (port 8883) and WebSocket Secure (port 8884).

🔒 **No Certificate Bypass**: Unlike some libraries, DecentIoT enforces certificate validation - security cannot be disabled.

🔒 **Production Ready**: Safe for real-world IoT deployments where security is critical.

For detailed security information, see [SECURITY.md](SECURITY.md).

## Quick Start

Here's a simple example showing secure MQTT communication:

```cpp
#include <DecentIoT.h>
#include <WiFi.h>

// WiFi and MQTT credentials
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASS "your-wifi-password"
#define MQTT_BROKER "broker.hivemq.cloud"
#define MQTT_PORT 8883  // SSL port
#define MQTT_USER "your-username"
#define MQTT_PASS "your-password"

const int LED_PIN = 2;

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    
    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    // Initialize with SSL/TLS (certificate automatically used)
    DecentIoT.begin("project-id", "user-id", "device-id", 
                   MQTT_BROKER, MQTT_PORT, MQTT_USER, MQTT_PASS);
    
    // Register callback for LED control
    DecentIoT.onReceive("led", [](const DecentIoTValue& value) {
        bool ledState = value; // Automatic type conversion
        digitalWrite(LED_PIN, ledState);
        Serial.printf("LED: %s\n", ledState ? "ON" : "OFF");
    });
    
    // Send temperature data
    DecentIoT.onSend("temperature", []() {
        float temp = readTemperature();
        DecentIoT.write("temperature", temp);
    });
}

void loop() {
    DecentIoT.run(); // Handle MQTT operations
    delay(100);
}

float readTemperature() {
    // Your sensor code here
    return 25.0 + random(-5, 5);
}
```

## Component Paths

When you add components in the OpenIoT Dashboard, each component gets a unique path:

| Component Type | Example Path | Description |
|---------------|--------------|-------------|
| Toggle Button | `/projects/myProject/components/led_toggle_123` | Control an LED |
| Slider | `/projects/myProject/components/brightness_slider_456` | Control brightness |
| Gauge | `/projects/myProject/components/temp_gauge_789` | Display sensor data |
| Graph | `/projects/myProject/components/humidity_graph_012` | Show data history |

Use these exact paths in your code to ensure proper communication between your device and dashboard.

## Detailed Usage

### Initialization

```cpp
// Basic initialization
OpenIoT.begin(FIREBASE_URL, FIREBASE_AUTH, WIFI_SSID, WIFI_PASS);

// With additional options
OpenIoTConfig config;
config.deviceId = "unique-device-id";
config.updateInterval = 100;  // Update interval in ms
OpenIoT.begin(FIREBASE_URL, FIREBASE_AUTH, WIFI_SSID, WIFI_PASS, config);
```

### Path Callbacks

```cpp
// Digital output example
FIREBASE_PATH("/devices/mydevice/led") {
    bool value = param.asBool();
    digitalWrite(LED_PIN, value);
}

// Analog output example
FIREBASE_PATH("/devices/mydevice/brightness") {
    int value = param.asInt();
    analogWrite(LED_PIN, value);
}

// JSON object example
FIREBASE_PATH("/devices/mydevice/rgb") {
    int r = param["red"].asInt();
    int g = param["green"].asInt();
    int b = param["blue"].asInt();
    setRGBColor(r, g, b);
}
```

### Writing Data

```cpp
// Write single values
OpenIoT.write("/sensors/temperature", 25.5);
OpenIoT.write("/status/online", true);

// Write JSON objects
JsonObject data = OpenIoT.createObject();
data["temp"] = 25.5;
data["humidity"] = 60;
data["timestamp"] = time(nullptr);
OpenIoT.write("/sensors", data);
```

### Reading Data

```cpp
// Read single values
float temp = OpenIoT.read<float>("/sensors/temperature");
bool isOn = OpenIoT.read<bool>("/status/online");

// Read JSON objects
JsonObject data = OpenIoT.read("/sensors");
float temp = data["temp"].asFloat();
int humidity = data["humidity"].asInt();
```

## Firebase Setup

1. Create a Firebase project at [Firebase Console](https://console.firebase.google.com/)
2. Enable Realtime Database
3. Set up security rules for your database
4. Get your Firebase project credentials:
   - Database URL
   - Authentication token

Example security rules for testing:
```json
{
  "rules": {
    ".read": true,
    ".write": true
  }
}
```

## Examples

The library comes with several examples:

1. **Basic LED Control**: Control an LED through Firebase
2. **Sensor Monitoring**: Upload sensor data to Firebase
3. **RGB LED Control**: Control RGB LED with JSON data
4. **Multiple Devices**: Manage multiple devices
5. **Secure Communication**: Implementation with security best practices

Check the `examples` folder for complete code.

## Supported Hardware

- ESP32 (all variants)
- ESP8266
- More platforms coming soon...

## Dependencies

- ArduinoJson (>= 6.0.0)
- WiFi library for your board

## Contributing

We welcome contributions! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

- Create an issue on GitHub
- Join our Discord community
- Check out our documentation website

## Acknowledgments

- Inspired by the simplicity of Blynk
- Built for the IoT community
- Special thanks to all contributors

---

Made with ❤️ by the OpenIoT Team

