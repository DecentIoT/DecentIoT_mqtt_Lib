# DecentIoT Security Documentation

## SSL/TLS Certificate Support

DecentIoT includes built-in SSL/TLS certificate validation to ensure secure connections to MQTT brokers. This is crucial for production use where security is a major concern.

### How It Works

1. **Automatic Certificate Usage**: When you specify port 8883 (SSL/TLS), the library automatically uses the included root certificate.

2. **Certificate Validation**: The library validates the MQTT broker's certificate against the trusted root certificate, preventing man-in-the-middle attacks.

3. **No Certificate Ignoring**: Unlike some libraries that allow certificate validation to be disabled, DecentIoT enforces certificate validation for security.

4. **SSL/TLS Support**: The library supports MQTT over SSL/TLS (port 8883) for secure IoT device communication.

### Included Certificate

The library includes a Let's Encrypt root certificate that works with most cloud MQTT brokers including:
- HiveMQ Cloud
- AWS IoT Core
- Google Cloud IoT
- Azure IoT Hub
- And other brokers using Let's Encrypt certificates

### Usage Example

```cpp
#include <DecentIoT.h>

void setup() {
    // Connect to WiFi first
    WiFi.begin(ssid, password);
    
    // MQTT over SSL/TLS (port 8883)
    DecentIoT.begin(
        "project-id", 
        "user-id", 
        "device-id", 
        "broker.hivemq.cloud", 
        8883,  // SSL port - certificate will be used automatically
        "username", 
        "password"
    );
}
```

### Security Benefits

1. **Encrypted Communication**: All MQTT messages are encrypted in transit
2. **Certificate Validation**: Prevents connection to fake or compromised brokers
3. **Production Ready**: Safe for real-world IoT deployments
4. **No Configuration Required**: Certificate is automatically applied

### For Different Brokers

If you're using a different MQTT broker that doesn't use Let's Encrypt certificates, you can:

1. **Replace the certificate**: Edit `src/mqtt_root_ca.cpp` with your broker's root certificate
2. **Add multiple certificates**: Modify the library to support multiple certificates
3. **Contact us**: We can help add support for your specific broker

### Best Practices

1. **Always use SSL/TLS**: Use port 8883 for secure connections in production
2. **Keep certificates updated**: Regularly update the root certificate
3. **Use strong passwords**: Ensure your MQTT broker credentials are secure
4. **Monitor connections**: Log connection status and any certificate errors

### Troubleshooting

If you encounter certificate errors:

1. **Check broker URL**: Ensure you're connecting to the correct broker
2. **Verify port**: Make sure you're using port 8883 for SSL/TLS
3. **Check certificate**: Verify the broker uses a supported certificate authority
4. **Update library**: Ensure you're using the latest version with updated certificates
5. **Connection issues**: Ensure your broker supports MQTT over SSL/TLS

### Certificate Details

The included certificate is:
- **Issuer**: Let's Encrypt Authority X3
- **Valid for**: Most cloud MQTT brokers
- **Type**: Root certificate
- **Format**: PEM encoded

This certificate ensures your IoT devices can securely connect to popular MQTT brokers without compromising security.
