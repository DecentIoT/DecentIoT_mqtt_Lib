#include "DecentIoT.h"
#include <ArduinoJson.h>
#include "mqtt_root_ca.h"

DecentIoTClass DecentIoT;
DecentIoTClass &getDecentIoT() { return DecentIoT; }

DecentIoTClass::DecentIoTClass() : _port(1883),
                                   _useWebSocket(false),
                                   _mqtt(512)
{
#ifdef ESP8266
    _cert = nullptr;
#endif
}

void DecentIoTClass::begin(const char *projectId, const char *userId, const char *deviceId,
                           const char *mqttBroker, int mqttPort, const char *mqttUser, const char *mqttPass)
{
    _projectId = projectId;
    _userId = userId;
    _deviceId = deviceId;
    _broker = mqttBroker;
    _port = mqttPort;
    _username = mqttUser;
    _password = mqttPass;

    if (_port == 8884)
    {
        // WebSocket over TLS setup
        _useWebSocket = true;
        _setupWebSocket();
        
        // For WebSocket, we'll use the WebSocketsClient directly
        // The MQTT over WebSocket protocol will be handled manually
        String clientId = "DecentIoT-" + String(random(0xffff), HEX);
        Serial.printf("WebSocket MQTT setup for client: %s\n", clientId.c_str());
    }
    else if (_port == 8883)
    {
        // Regular MQTT over TLS
        _useWebSocket = false;
#ifdef ESP8266
        if (_cert == nullptr)
        {
            _cert = new BearSSL::X509List(root_ca);
            _client.setTrustAnchors(_cert);
            _client.setInsecure(); // For testing
        }
#elif defined(ESP32)
        _client.setCACert(root_ca);
#endif
        _mqtt.begin(_client);
        String clientId = "DecentIoT-" + String(random(0xffff), HEX);
        _mqtt.connect(clientId.c_str(), _username.c_str(), _password.c_str());
    }
}

void DecentIoTClass::onReceive(const char *pin, ReceiveCallback callback)
{
    _receiveHandlers.push_back({pin, callback});
    // Subscribe to the topic for this pin
    String topic = _getTopic(pin);
    if (_useWebSocket)
    {
        if (_ws.isConnected())
        {
            _sendMQTTSubscribe(topic);
        }
        // If not connected, subscription will be sent after connection
    }
    else
    {
        _mqtt.subscribe(topic.c_str());
    }
}

void DecentIoTClass::onSend(const char *pin, SendCallback callback)
{
    _sendHandlers.push_back({pin, callback});
    // Optionally, implement scheduling if needed
}

String DecentIoTClass::_getTopic(const char *pin) const
{
    return _projectId + "/users/" + _userId + "/datastreams/" + _deviceId + "/" + pin;
}

void DecentIoTClass::_handleMessage(const char *topic, const uint8_t *payload, unsigned int length)
{
    String topicStr(topic);
    String pin = topicStr.substring(topicStr.lastIndexOf('/') + 1);
    String message;
    for (unsigned int i = 0; i < length; ++i)
        message += (char)payload[i];

    // Try to parse as bool, int, float, string (in that order)
    DecentIoTValue v;
    if (message == "true" || message == "false")
    {
        v.type = DecentIoTValue::BOOL;
        v.boolValue = (message == "true");
    }
    else if (message.length() > 0 && isNumericString(message))
    {
        v.type = DecentIoTValue::INT;
        v.intValue = message.toInt();
    }
    else if (message.indexOf('.') != -1 && message.toFloat() != 0.0f)
    {
        v.type = DecentIoTValue::FLOAT;
        v.floatValue = message.toFloat();
    }
    else
    {
        v.type = DecentIoTValue::STRING;
        v.stringValue = message;
    }
    for (auto &handler : _receiveHandlers)
    {
        if (handler.id == pin)
        {
            handler.callback(v);
            break;
        }
    }
}

void DecentIoTClass::write(const char *pin, bool value)
{
    String topic = _getTopic(pin);
    const char *payload = value ? "true" : "false";
    if (_useWebSocket)
    {
        if (_ws.isConnected())
        {
            _sendMQTTPublish(topic, payload);
        }
        else
        {
            Serial.println("WebSocket not connected, skipping message");
        }
    }
    else
    {
        _mqtt.publish(topic.c_str(), payload, true);
    }
}
void DecentIoTClass::write(const char *pin, int value)
{
    String topic = _getTopic(pin);
    char buffer[16];
    sprintf(buffer, "%d", value);
    if (_useWebSocket)
    {
        if (_ws.isConnected())
        {
            _sendMQTTPublish(topic, buffer);
        }
        else
        {
            Serial.println("WebSocket not connected, skipping message");
        }
    }
    else
    {
        _mqtt.publish(topic.c_str(), buffer, true);
    }
}
void DecentIoTClass::write(const char *pin, float value)
{
    String topic = _getTopic(pin);
    char buffer[16];
    sprintf(buffer, "%f", value);
    if (_useWebSocket)
    {
        if (_ws.isConnected())
        {
            _sendMQTTPublish(topic, buffer);
        }
        else
        {
            Serial.println("WebSocket not connected, skipping message");
        }
    }
    else
    {
        _mqtt.publish(topic.c_str(), buffer, true);
    }
}
void DecentIoTClass::write(const char *pin, const char *value)
{
    String topic = _getTopic(pin);
    if (_useWebSocket)
    {
        if (_ws.isConnected())
        {
            _sendMQTTPublish(topic, value);
        }
        else
        {
            Serial.println("WebSocket not connected, skipping message");
        }
    }
    else
    {
        _mqtt.publish(topic.c_str(), value, true);
    }
}

void DecentIoTClass::publishStatus(const char *status)
{
    String topic = _projectId + "/users/" + _userId + "/datastreams/" + _deviceId + "/status";
    if (_useWebSocket)
    {
        if (_ws.isConnected())
        {
            _sendMQTTPublish(topic, status);
        }
        else
        {
            Serial.println("WebSocket not connected, skipping status");
        }
    }
    else
    {
        _mqtt.publish(topic.c_str(), status, true);
    }
}

void DecentIoTClass::run()
{
    if (_useWebSocket)
    {
        _ws.loop();
    }
    else
    {
        _mqtt.loop();
    }
    processScheduledTasks();
}

bool DecentIoTClass::connected()
{
    if (_useWebSocket)
    {
        return _ws.isConnected();
    }
    return _mqtt.connected();
}
void DecentIoTClass::disconnect()
{
    if (_useWebSocket)
    {
        _ws.disconnect();
    }
    else
    {
        _mqtt.disconnect();
    }
}
const char *DecentIoTClass::getStatus()
{
    if (_useWebSocket)
    {
        return _ws.isConnected() ? "connected" : "disconnected";
    }
    return _mqtt.connected() ? "connected" : "disconnected";
}
const char *DecentIoTClass::getLastError()
{
    // Return last error string if needed
    return "";
}

bool DecentIoTClass::isSecure() const
{
    return _port == 8883 || _port == 8884;
}

void DecentIoTClass::schedule(uint32_t interval, TaskCallback callback)
{
    String taskId = "task_" + String(millis());
    schedule(taskId, interval, callback);
}

void DecentIoTClass::schedule(String taskId, uint32_t interval, TaskCallback callback)
{
    ScheduledTask task;
    task.lastRun = 0;
    task.interval = interval;
    task.callback = callback;
    _scheduledTasks[taskId] = task;
}

void DecentIoTClass::scheduleOnce(uint32_t delay, TaskCallback callback)
{
    String taskId = "once_" + String(millis());
    ScheduledTask task;
    task.lastRun = millis();
    task.interval = delay;
    task.callback = callback;
    _scheduledTasks[taskId] = task;
}

void DecentIoTClass::processScheduledTasks()
{
    unsigned long currentTime = millis();
    for (auto it = _scheduledTasks.begin(); it != _scheduledTasks.end();)
    {
        if (currentTime - it->second.lastRun >= it->second.interval)
        {
            it->second.callback();
            it->second.lastRun = currentTime;

            // Remove one-time tasks
            if (it->first.indexOf("once_") == 0)
            {
                it = _scheduledTasks.erase(it);
            }
            else
            {
                ++it;
            }
        }
        else
        {
            ++it;
        }
    }
}

// Helper function to check if a string is numeric (ESP8266 compatible)
bool DecentIoTClass::isNumericString(const String &str)
{
    if (str.length() == 0)
        return false;

    // Check if it's a valid integer (including negative numbers)
    for (unsigned int i = 0; i < str.length(); i++)
    {
        char c = str.charAt(i);
        if (i == 0 && c == '-')
            continue; // Allow negative sign at start
        if (c < '0' || c > '9')
            return false;
    }
    return true;
}

void DecentIoTClass::_setupWebSocket()
{
    _ws.onEvent([this](WStype_t type, uint8_t *payload, size_t length)
                { this->_webSocketEvent(type, payload, length); });

#ifdef ESP8266
    if (_cert == nullptr)
    {
        _cert = new BearSSL::X509List(root_ca);
        _client.setTrustAnchors(_cert);
    }
#endif

    // For WebSocket, we use a different URL format
    String wsProtocol = "mqtt"; // MQTT over WebSocket protocol
    const uint8_t *fingerprint = nullptr;

    // Set WebSocket options for better stability
    _ws.setReconnectInterval(5000); // Reconnect every 5 seconds
    _ws.enableHeartbeat(15000, 3000, 2); // Heartbeat every 15s, timeout 3s, disconnect after 2 failures

    _ws.beginSSL(_broker.c_str(), _port, "/mqtt", fingerprint, wsProtocol.c_str());

    Serial.printf("WebSocket connecting to wss://%s:%d/mqtt\n", _broker.c_str(), _port);
}

void DecentIoTClass::_webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        Serial.println("WebSocket disconnected");
        break;
    case WStype_CONNECTED:
        Serial.println("WebSocket connected");
        // Send MQTT CONNECT packet
        _sendMQTTConnect();
        break;
    case WStype_TEXT:
        // Handle incoming MQTT messages
        _handleMQTTMessage(payload, length);
        break;
    case WStype_BIN:
        // Handle binary MQTT messages
        _handleMQTTMessage(payload, length);
        break;
    case WStype_ERROR:
        Serial.println("WebSocket error");
        break;
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    case WStype_PING:
    case WStype_PONG:
        // Handle other WebSocket events (not used in current implementation)
        break;
    }
}

// MQTT Protocol Implementation for WebSocket
void DecentIoTClass::_sendMQTTConnect()
{
    String clientId = "DecentIoT-" + String(random(0xffff), HEX);
    
    // MQTT CONNECT packet
    uint8_t packet[128];
    int pos = 0;
    
    // Fixed header
    packet[pos++] = 0x10; // CONNECT packet type
    pos++; // Length placeholder
    
    // Variable header
    packet[pos++] = 0x00; // Protocol name length MSB
    packet[pos++] = 0x04; // Protocol name length LSB
    packet[pos++] = 'M';
    packet[pos++] = 'Q';
    packet[pos++] = 'T';
    packet[pos++] = 'T';
    packet[pos++] = 0x04; // Protocol level
    packet[pos++] = 0xC2; // Connect flags (username, password, clean session)
    packet[pos++] = 0x00; // Keep alive MSB
    packet[pos++] = 0x3C; // Keep alive LSB (60 seconds)
    
    // Payload
    // Client ID
    packet[pos++] = (clientId.length() >> 8) & 0xFF;
    packet[pos++] = clientId.length() & 0xFF;
    for (int i = 0; i < clientId.length(); i++) {
        packet[pos++] = clientId[i];
    }
    
    // Username
    packet[pos++] = (_username.length() >> 8) & 0xFF;
    packet[pos++] = _username.length() & 0xFF;
    for (int i = 0; i < _username.length(); i++) {
        packet[pos++] = _username[i];
    }
    
    // Password
    packet[pos++] = (_password.length() >> 8) & 0xFF;
    packet[pos++] = _password.length() & 0xFF;
    for (int i = 0; i < _password.length(); i++) {
        packet[pos++] = _password[i];
    }
    
    // Set packet length
    packet[1] = pos - 2;
    
    // Send the packet
    _ws.sendBIN(packet, pos);
    Serial.println("MQTT CONNECT sent");
    
    // Subscribe to topics after connection
    delay(100); // Wait for CONNACK
    for (auto &handler : _receiveHandlers)
    {
        String topic = _getTopic(handler.id.c_str());
        _sendMQTTSubscribe(topic);
    }
}

void DecentIoTClass::_sendMQTTPublish(const String &topic, const String &payload)
{
    uint8_t packet[256];
    int pos = 0;
    
    // Fixed header
    packet[pos++] = 0x30; // PUBLISH packet type
    pos++; // Length placeholder
    
    // Variable header
    packet[pos++] = (topic.length() >> 8) & 0xFF;
    packet[pos++] = topic.length() & 0xFF;
    for (int i = 0; i < topic.length(); i++) {
        packet[pos++] = topic[i];
    }
    
    // Payload
    for (int i = 0; i < payload.length(); i++) {
        packet[pos++] = payload[i];
    }
    
    // Set packet length
    packet[1] = pos - 2;
    
    // Send the packet
    _ws.sendBIN(packet, pos);
}

void DecentIoTClass::_sendMQTTSubscribe(const String &topic)
{
    uint8_t packet[128];
    int pos = 0;
    
    // Fixed header
    packet[pos++] = 0x82; // SUBSCRIBE packet type
    pos++; // Length placeholder
    
    // Variable header
    packet[pos++] = 0x00; // Packet ID MSB
    packet[pos++] = 0x01; // Packet ID LSB
    
    // Payload
    packet[pos++] = (topic.length() >> 8) & 0xFF;
    packet[pos++] = topic.length() & 0xFF;
    for (int i = 0; i < topic.length(); i++) {
        packet[pos++] = topic[i];
    }
    packet[pos++] = 0x00; // QoS
    
    // Set packet length
    packet[1] = pos - 2;
    
    // Send the packet
    _ws.sendBIN(packet, pos);
    Serial.printf("MQTT SUBSCRIBE sent for: %s\n", topic.c_str());
}

void DecentIoTClass::_handleMQTTMessage(uint8_t *payload, size_t length)
{
    if (length < 2) return;
    
    uint8_t packetType = payload[0] >> 4;
    
    switch (packetType) {
        case 2: // CONNACK
            Serial.println("MQTT CONNACK received");
            break;
        case 3: // PUBLISH
            _handleMQTTPublish(payload, length);
            break;
        case 9: // SUBACK
            Serial.println("MQTT SUBACK received");
            break;
        default:
            Serial.printf("MQTT packet type %d received\n", packetType);
            break;
    }
}

void DecentIoTClass::_handleMQTTPublish(uint8_t *payload, size_t length)
{
    // Parse MQTT PUBLISH packet
    int pos = 2; // Skip fixed header
    
    // Read topic length
    int topicLength = (payload[pos] << 8) | payload[pos + 1];
    pos += 2;
    
    // Read topic
    String topic = "";
    for (int i = 0; i < topicLength; i++) {
        topic += (char)payload[pos + i];
    }
    pos += topicLength;
    
    // Read payload
    String message = "";
    for (int i = pos; i < length; i++) {
        message += (char)payload[i];
    }
    
    Serial.printf("MQTT PUBLISH: %s = %s\n", topic.c_str(), message.c_str());
    
    // Handle the message
    String pin = topic.substring(topic.lastIndexOf('/') + 1);
    _handleMessage(topic.c_str(), (const uint8_t *)message.c_str(), message.length());
}

DecentIoTClass::~DecentIoTClass()
{
#ifdef ESP8266
    if (_cert != nullptr)
    {
        delete _cert;
        _cert = nullptr;
    }
#endif
}