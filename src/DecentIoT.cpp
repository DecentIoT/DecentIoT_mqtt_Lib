#include "DecentIoT.h"
#include <ArduinoJson.h>
#include "mqtt_root_ca.h"

DecentIoTClass DecentIoT;
DecentIoTClass &getDecentIoT() { return DecentIoT; }

DecentIoTClass::DecentIoTClass() : _port(1883),
                                   _useWebSocket(false),
                                   _wsClient(nullptr),
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
        _ws.beginSSL(_broker.c_str(), _port, "/mqtt");

        if (_wsClient == nullptr)
        {
            _wsClient = new WebSocketClient(_ws);
        }

        _mqtt.begin(*_wsClient);
        String clientId = "DecentIoT-" + String(random(0xffff), HEX);
        _mqtt.connect(clientId.c_str(), _username.c_str(), _password.c_str());

        _mqtt.onMessage([this](String &topic, String &payload)
                        { this->_handleMessage(topic.c_str(), (const uint8_t *)payload.c_str(), payload.length()); });
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
    _mqtt.subscribe(topic.c_str());
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
    _mqtt.publish(topic.c_str(), payload, true);
}
void DecentIoTClass::write(const char *pin, int value)
{
    String topic = _getTopic(pin);
    char buffer[16];
    sprintf(buffer, "%d", value);
    _mqtt.publish(topic.c_str(), buffer, true);
}
void DecentIoTClass::write(const char *pin, float value)
{
    String topic = _getTopic(pin);
    char buffer[16];
    sprintf(buffer, "%f", value);
    _mqtt.publish(topic.c_str(), buffer, true);
}
void DecentIoTClass::write(const char *pin, const char *value)
{
    String topic = _getTopic(pin);
    _mqtt.publish(topic.c_str(), value, true);
}

void DecentIoTClass::publishStatus(const char *status)
{
    String topic = _projectId + "/users/" + _userId + "/datastreams/" + _deviceId + "/status";
    _mqtt.publish(topic.c_str(), status, true);
}

void DecentIoTClass::run()
{
    if (_useWebSocket)
    {
        _ws.loop();
    }
    _mqtt.loop();
    processScheduledTasks();
}

bool DecentIoTClass::connected()
{
    return _mqtt.connected();
}
void DecentIoTClass::disconnect()
{
    _mqtt.disconnect();
}
const char *DecentIoTClass::getStatus()
{
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

    _webSocket.beginSSL(_broker.c_str(), _port, "/mqtt", fingerprint, wsProtocol.c_str());

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
        // Subscribe to topics after connection
        for (auto &handler : _receiveHandlers)
        {
            String topic = _getTopic(handler.id.c_str());
            // Send MQTT subscribe message via WebSocket
            // This is a simplified implementation - in practice you'd need proper MQTT over WebSocket protocol
            Serial.printf("Subscribing to: %s\n", topic.c_str());
        }
        break;
    case WStype_TEXT:
        // Handle incoming MQTT messages
        // Parse MQTT message from WebSocket payload
        // This is a simplified implementation
        Serial.printf("Received WebSocket message: %s\n", payload);
        break;
    case WStype_BIN:
        // Handle binary messages if needed
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

DecentIoTClass::~DecentIoTClass()
{
    if (_wsClient != nullptr)
    {
        delete _wsClient;
        _wsClient = nullptr;
    }
#ifdef ESP8266
    if (_cert != nullptr)
    {
        delete _cert;
        _cert = nullptr;
    }
#endif
}