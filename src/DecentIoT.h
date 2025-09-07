#pragma once

// Disable WebSocket debug output globally - MUST be before any includes
#define NODEBUG_WEBSOCKETS

#include <Arduino.h>
#include <vector>
#include <map>
#include <functional>

// Platform-specific includes and declarations
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <WiFiClientSecure.h>
#else
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif

#include <WebSocketsClient.h>
#include <MQTT.h>
#include <PubSubClient.h>



struct DecentIoTValue
{
    enum Type
    {
        BOOL,
        INT,
        FLOAT,
        STRING
    } type;
    bool boolValue;
    int intValue;
    float floatValue;
    String stringValue;

    // Implicit conversion operators
    operator bool() const
    {
        if (type == BOOL)
            return boolValue;
        if (type == INT)
            return intValue != 0;
        if (type == FLOAT)
            return floatValue != 0.0f;
        if (type == STRING)
            return stringValue == "true" || stringValue == "1";
        return false;
    }
    operator int() const
    {
        if (type == INT)
            return intValue;
        if (type == BOOL)
            return boolValue ? 1 : 0;
        if (type == FLOAT)
            return static_cast<int>(floatValue);
        if (type == STRING)
            return stringValue.toInt();
        return 0;
    }
    operator float() const
    {
        if (type == FLOAT)
            return floatValue;
        if (type == INT)
            return static_cast<float>(intValue);
        if (type == BOOL)
            return boolValue ? 1.0f : 0.0f;
        if (type == STRING)
            return stringValue.toFloat();
        return 0.0f;
    }
    operator String() const
    {
        if (type == STRING)
            return stringValue;
        if (type == BOOL)
            return boolValue ? "true" : "false";
        if (type == INT)
            return String(intValue);
        if (type == FLOAT)
            return String(floatValue);
        return "";
    }
    operator uint8_t() const
    {
        if (type == BOOL)
            return boolValue ? HIGH : LOW;
        if (type == INT)
            return intValue;
        if (type == FLOAT)
            return static_cast<uint8_t>(floatValue);
        if (type == STRING)
            return (stringValue == "true" || stringValue == "1") ? HIGH : LOW;
        return 0;
    }
};
using ReceiveCallback = std::function<void(const DecentIoTValue &value)>;
using SendCallback = std::function<void()>;
using TaskCallback = std::function<void()>;

struct ReceiveHandler
{
    String id;
    ReceiveCallback callback;
};
struct SendHandler
{
    String id;
    SendCallback callback;
};

// Scheduled task structure
struct ScheduledTask
{
    unsigned long lastRun;
    unsigned long interval;
    TaskCallback callback;
};

class DecentIoTClass
{
private:
    // Member variables - order matters for constructor
    String _projectId;
    String _userId;
    String _deviceId;
    String _broker;
    int _port;
    String _username;
    String _password;
    bool _useWebSocket;
    WiFiClientSecure _client;
    WebSocketsClient _ws;
    MQTTClient _mqtt;  // For WebSocket (port 8884)
    PubSubClient _pubsub;  // For TLS (port 8883)
    std::vector<ReceiveHandler> _receiveHandlers;
    std::vector<SendHandler> _sendHandlers;
    std::map<String, ScheduledTask> _scheduledTasks;

#ifdef ESP8266
    BearSSL::X509List *_cert;
#endif

public:
    DecentIoTClass();
    ~DecentIoTClass(); // Add this line
    void begin(const char *mqttBroker, int mqttPort, const char *mqttUser, const char *mqttPass, const char *projectId, const char *userId, const char *deviceId);
    void onReceive(const char *pin, ReceiveCallback callback);
    void onSend(const char *pin, SendCallback callback);
    void run();
    void write(const char *pin, bool value);
    void write(const char *pin, int value);
    void write(const char *pin, float value);
    void write(const char *pin, const char *value);
    void publishStatus(const char *status); // for heartbeat/status
    bool connected();
    void disconnect();
    const char *getStatus();
    const char *getLastError();
    bool isSecure() const; // Check if SSL/TLS is being used
    void schedule(uint32_t interval, TaskCallback callback);
    void schedule(String taskId, uint32_t interval, TaskCallback callback);
    void scheduleOnce(uint32_t delay, TaskCallback callback);
    void setCACert(const char *cert); 
    void _subscribeAllPubSub();   // this can/should be in under private

private:
    String _getTopic(const char *pin) const;
    void _handleMessage(const char *topic, const uint8_t *payload, unsigned int length);
    void _setupWebSocket();
    void _webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
    void _sendMQTTConnect();
    void _sendMQTTPublish(const String &topic, const String &payload);
    void _sendMQTTSubscribe(const String &topic);
    void _handleMQTTMessage(uint8_t *payload, size_t length);
    void _handleMQTTPublish(uint8_t *payload, size_t length);
    void processScheduledTasks();
    bool isNumericString(const String &str);
    unsigned long _lastStatusUpdate = 0;
    const unsigned long _statusUpdateInterval = 30000; // 30 seconds
    void _publishDeviceStatus(bool online);
};

extern DecentIoTClass DecentIoT;
DecentIoTClass &getDecentIoT();

// Macro for user-friendly receive handler definition
#define DECENTIOT_RECEIVE(PIN_NAME)                                                                                            \
    void DECENTIOT_RECEIVE_HANDLER_##PIN_NAME(const DecentIoTValue &value);                                                    \
    static DecentIoTReceiveRegistrar _decentiot_receive_registrar_##PIN_NAME(#PIN_NAME, DECENTIOT_RECEIVE_HANDLER_##PIN_NAME); \
    void DECENTIOT_RECEIVE_HANDLER_##PIN_NAME(const DecentIoTValue &value)

// Macro for user-friendly send handler definition with optional interval
#define DECENTIOT_SEND(PIN_NAME, ...)                                                                                                \
    void DECENTIOT_SEND_HANDLER_##PIN_NAME();                                                                                        \
    static DecentIoTSendRegistrar _decentiot_send_registrar_##PIN_NAME(#PIN_NAME, DECENTIOT_SEND_HANDLER_##PIN_NAME, ##__VA_ARGS__); \
    void DECENTIOT_SEND_HANDLER_##PIN_NAME()

// Helper class to register receive handlers at static initialization time
class DecentIoTReceiveRegistrar
{
public:
    DecentIoTReceiveRegistrar(const char *pin, ReceiveCallback cb)
    {
        getDecentIoT().onReceive(pin, cb);
    }
};
// Helper class to register send handlers at static initialization time
class DecentIoTSendRegistrar
{
public:
    DecentIoTSendRegistrar(const char *pin, SendCallback cb, uint32_t interval = 0)
    {
        if (interval > 0)
        {
            // If interval is provided, create a scheduled task
            String taskId = String("send_") + pin;
            getDecentIoT().schedule(taskId, interval, cb);
        }
        else
        {
            // If no interval, just register the callback
            getDecentIoT().onSend(pin, cb);
        }
    }
};

// Pin definitions (same as OpenIoT for consistency)
#define P0 "P0"
#define P1 "P1"
#define P2 "P2"
#define P3 "P3"
#define P4 "P4"
#define P5 "P5"
#define P6 "P6"
#define P7 "P7"
#define P8 "P8"
#define P9 "P9"
#define P10 "P10"
#define P11 "P11"
#define P12 "P12"
#define P13 "P13"
#define P14 "P14"
#define P15 "P15"
#define P16 "P16"
#define P17 "P17"
#define P18 "P18"
#define P19 "P19"
#define P20 "P20"
#define P21 "P21"
#define P22 "P22"
#define P23 "P23"
#define P24 "P24"
#define P25 "P25"
#define P26 "P26"
#define P27 "P27"
#define P28 "P28"
#define P29 "P29"
#define P30 "P30"
#define P31 "P31"
#define P32 "P32"
#define P33 "P33"
#define P34 "P34"
#define P35 "P35"
#define P36 "P36"
#define P37 "P37"
#define P38 "P38"
#define P39 "P39"
#define P40 "P40"
#define P41 "P41"
#define P42 "P42"
#define P43 "P43"
#define P44 "P44"
#define P45 "P45"
#define P46 "P46"
#define P47 "P47"
#define P48 "P48"
#define P49 "P49"
#define P50 "P50"