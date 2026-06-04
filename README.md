# DecentIoT MQTT Library

A revolutionary IoT platform that combines a powerful Arduino library with a professional web dashboard. DecentIoT gives you the best of both worlds: **beautiful dashboards like existing platforms** + **complete control with your own MQTT broker**.

## 🚀 What Makes DecentIoT Special?

**DecentIoT** is a **complete IoT platform** that combines:
- 🌐 **Professional Web Dashboard** - Create beautiful dashboards with widgets
- 🔧 **Your Own Cloud** - Use your own MQTT broker (HiveMQ, AWS IoT, etc.)
- 📱 **Smart Arduino Library** - This library that connects everything

**No more vendor lock-in!** Use your own infrastructure while getting a professional dashboard experience.

## Features

- 🎯 **Virtual Pin Architecture** - P0, P1, P2, P3... represent virtual pins that map to your web dashboard widgets
- 🗺️ **GPS/Map Integration** - Send GPS location data as pipe‑delimited strings for map widgets (the library serializes to the required format).
- 🔒 **Your Own Cloud** - Connect to your own MQTT broker (HiveMQ, AWS IoT, etc.)
- 🌐 **Professional Dashboard** - Create beautiful, interactive dashboards with your own backend
- ⚡ **Easy to Use** - Simple macros and intuitive API design
- 🔄 **Real-time Communication** - Bidirectional data flow between device and cloud
- 🛠️ **Production Ready** - Robust, reliable, and well-tested
- 💰 **Cost Effective** - No subscription fees, use your own infrastructure
- 🔒 **Secure by Default** - Uses MQTT over SSL/TLS (port 8883) for encrypted communication

## How It Works

```
Physical Device          Virtual Pins          Web Dashboard
     ↓                       ↓                      ↓
  D6 (LED)    ←→    P0 (Virtual)    ←→    Button Widget
  A0 (Sensor) ←→    P1 (Virtual)    ←→    Gauge Widget
  A1 (Sensor) ←→    P2 (Virtual)    ←→    Chart Widget
```

1. **Set up your MQTT broker** (HiveMQ, AWS IoT, etc.)
2. **Create DecentIoT web dashboard** with widgets
3. **Map widgets to virtual pins** (P0, P1, P2, etc.)
4. **Use this library** to connect your device
5. **Send/receive data** through virtual pins

## Installation

### Arduino Library Manager (Recommended)
1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search for **"DecentIoT MQTT"**
4. Click **Install** (dependencies are automatically installed)

### Manual Installation
1. Download the latest release from GitHub
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library**
3. Select the downloaded zip file
4. **Manually install dependencies**:
   - `PubSubClient` by Nick O'Leary

## Security Features

🔒 **SSL/TLS Support**: Uses MQTT over SSL/TLS (port 8883) for encrypted communication  
🔒 **Your Own Infrastructure**: Data stays on your servers, not third-party platforms  
🔒 **Production Ready**: Safe for real-world IoT deployments  
🔒 **No Vendor Lock-in**: Complete control over your data and infrastructure  

For detailed security information, see [SECURITY.md](SECURITY.md).

## Quick Start

1. **Set up your MQTT broker** (HiveMQ, AWS IoT, etc.)
2. **Create DecentIoT web dashboard** with widgets
3. **Install this library** and connect your device
4. **Map virtual pins** (P0, P1, P2...) to dashboard widgets

**Ready to get started?** Check out our comprehensive guides:

- 📋 **[Complete Introduction Guide](guide/Introduction.md)** - Everything you need to know
- ⏰ **[Scheduling System Guide](guide/ScheduleGuidelines.md)** - Advanced features
- 🔧 **[Code Examples](examples/)** - Ready-to-use examples

## GPS/Map Integration

DecentIoT supports GPS tracking with automatic JSON serialization for map widgets:

```cpp
// Send GPS data automatically every 10 seconds (Dashboard integration)
DECENTIOT_SEND(P0, 10000) {
    // Feed incoming NMEA characters from your GPS serial (e.g., SoftwareSerial gpsSerial)
    while (gpsSerial.available()) {
        DecentIoT.feedGPS(gpsSerial.read());
    }

    if (DecentIoT.gps.hasFix()) {
        // Publish current GPS state on virtual pin P0
        DecentIoT.writeGPS(P0);
    } else {
        Serial.println("[GPS] Waiting for satellite fix...");
    }
}

// Manual GPS sending (using custom values or a GPSData struct)
DecentIoT.writeGPS(P0, latitude, longitude, altitude, speed, accuracy);
```

**GPS Data Structure:**
```json
{
  "latitude": 40.7128,
  "longitude": -74.0060,
  "altitude": 15.5,
  "speed": 45.2,
  "accuracy": 5.0,
  "timestamp": 1704067200
}
```

## Examples

The library comes with several ready-to-use examples:

1. **SimpleLED**: Basic LED control with virtual pins
2. **SensorExample**: DHT sensor with temperature/humidity
3. **SecureMQTTExample**: Complete MQTT setup example
4. **GPSExample**: GPS tracking with TinyGPSPlus integration
5. **SimpleGPSExample**: Basic GPS sending without external libraries

**📁 [View all examples](examples/)** - Copy, paste, and customize for your project

## Supported Hardware

- ESP32 (all variants)
- ESP8266
- More platforms coming soon...

## Dependencies

- `PubSubClient` - MQTT client functionality

**Installation:**
- **Arduino Library Manager**: Dependencies are automatically installed ✅
- **Install from ZIP**: Dependencies are NOT automatically installed ❌

## Contributing

We welcome contributions! Please feel free to submit a Pull Request.

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## Documentation

- 📋 **[Introduction Guide](guide/Introduction.md)** - Complete getting started guide
- ⏰ **[Scheduling Guide](guide/ScheduleGuidelines.md)** - Advanced scheduling system
- 🔧 **Examples** - Ready-to-use code examples

## Support

- Create an issue on GitHub
- Check out our documentation guides
- Join our community discussions

## Acknowledgments

- Built for the IoT community
- Special thanks to the open-source community for the amazing libraries that make this project possible

### Third-Party Libraries
This library uses the following open-source libraries:
- **PubSubClient** by Nick O'Leary (MIT License) - MQTT client functionality

- **ESP8266/ESP32 WiFi Libraries** by Espressif (LGPL v2.1) - WiFi connectivity

**Contributors are welcome!** If you'd like to contribute to DecentIoT, please feel free to submit issues, feature requests, or pull requests.

---



