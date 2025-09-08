/*
  DecentIoT MQTT Library
  Copyright 2025 MD Jannatul Nayem
  
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
  
      http://www.apache.org/licenses/LICENSE-2.0
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "DecentIoT.h"
#include <ArduinoJson.h>
#include "mqtt_root_ca.h"
#include <time.h>  // Add this for time functions

DecentIoTClass DecentIoT;
DecentIoTClass &getDecentIoT() { return DecentIoT; }

DecentIoTClass::DecentIoTClass() : _port(1883),
                                   _pubsub(_client)
{
#ifdef ESP8266
    _cert = nullptr;
#endif
}

void DecentIoTClass::begin(const char *mqttBroker, int mqttPort, const char *mqttUser, const char *mqttPass,
                           const char *projectId, const char *userId, const char *deviceId)
{
    _projectId = projectId;
    _userId = userId;
    _deviceId = deviceId;
    _broker = mqttBroker;
    _port = mqttPort;
    _username = mqttUser;
    _password = mqttPass;

    // Sync time with NTP server (same as Firebase library)
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("[DecentIoT] Waiting for NTP time sync...");
    time_t now = 0;
    int retry = 0;
    while (now < 24 * 3600 && retry < 10) {
        Serial.print(".");
        delay(500);
        now = time(nullptr);
        retry++;
    }
    Serial.println();
    if (now > 24 * 3600) {
        Serial.printf("[DecentIoT] Time synced: %s", ctime(&now));
    } else {
        Serial.println("[DecentIoT] Failed to sync time");
    }

    // MQTT over TLS using PubSubClient (port 8883)
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
    
    // Set up PubSubClient
    _pubsub.setServer(_broker.c_str(), _port);
    _pubsub.setCallback([this](char* topic, byte* payload, unsigned int length) {
        _handleMessage(topic, payload, length);
    });
    
    String clientId = "DecentIoT-" + String(random(0xffff), HEX);
    
    // Try to connect with proper error handling
    Serial.println("🔗 Connecting to MQTT broker via TLS...");
    
    if (_pubsub.connect(clientId.c_str(), _username.c_str(), _password.c_str())) {
        Serial.println("✅ MQTT TLS connection successful");
        _subscribeAllPubSub();
        _publishDeviceStatus(true); // true = online
    } else {
        Serial.println("❌ MQTT TLS connection failed");
    }
}

void DecentIoTClass::onReceive(const char *pin, ReceiveCallback callback)
{
    _receiveHandlers.push_back({pin, callback});
    // For PubSubClient, subscribe after connection!
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
    if (_pubsub.connected())
    {
        _pubsub.publish(topic.c_str(), payload, true);
    }
    else
    {
        Serial.println("⚠️  MQTT not connected, skipping message");
    }
}
void DecentIoTClass::write(const char *pin, int value)
{
    String topic = _getTopic(pin);
    char buffer[16];
    sprintf(buffer, "%d", value);
    if (_pubsub.connected())
    {
        _pubsub.publish(topic.c_str(), buffer, true);
    }
    else
    {
        Serial.println("⚠️  MQTT not connected, skipping message");
    }
}
void DecentIoTClass::write(const char *pin, float value)
{
    String topic = _getTopic(pin);
    char buffer[16];
    sprintf(buffer, "%f", value);
    if (_pubsub.connected())
    {
        _pubsub.publish(topic.c_str(), buffer, true);
    }
    else
    {
        Serial.println("⚠️  MQTT not connected, skipping message");
    }
}
void DecentIoTClass::write(const char *pin, const char *value)
{
    String topic = _getTopic(pin);
    if (_pubsub.connected())
    {
        _pubsub.publish(topic.c_str(), value, true);
    }
    else
    {
        Serial.println("⚠️  MQTT not connected, skipping message");
    }
}

void DecentIoTClass::publishStatus(const char *status)
{
    String topic = _projectId + "/users/" + _userId + "/datastreams/" + _deviceId + "/status";
    if (_pubsub.connected())
    {
        _pubsub.publish(topic.c_str(), status, true);
    }
    else
    {
        Serial.println("⚠️  MQTT not connected, skipping status");
    }
}

void DecentIoTClass::run()
{
    _pubsub.loop();
    processScheduledTasks();
    unsigned long now = millis();
    if (now - _lastStatusUpdate > _statusUpdateInterval) {
        _publishDeviceStatus(true); // true = online
        _lastStatusUpdate = now;
    }
}

bool DecentIoTClass::connected()
{
    return _pubsub.connected();
}
void DecentIoTClass::disconnect()
{
    _pubsub.disconnect();
    _publishDeviceStatus(false);
}
const char *DecentIoTClass::getStatus()
{
    return _pubsub.connected() ? "connected" : "disconnected";
}
const char *DecentIoTClass::getLastError()
{
    // Return last error string if needed
    return "";
}

bool DecentIoTClass::isSecure() const
{
    return _port == 8883;
}

void DecentIoTClass::schedule(uint32_t interval, TaskCallback callback)
{
    String taskId = "task_" + String(millis());
    schedule(taskId, interval, callback);
}

void DecentIoTClass::schedule(String taskId, uint32_t interval, TaskCallback callback)
{
    _scheduledTasks[taskId] = {0, interval, callback};
}

void DecentIoTClass::scheduleOnce(uint32_t delay, TaskCallback callback)
{
    String taskId = "once_" + String(millis());
    _scheduledTasks[taskId] = {millis(), delay, callback};
}

void DecentIoTClass::cancel(String taskId)
{
    _scheduledTasks.erase(taskId);
}

void DecentIoTClass::cancelSend(const char *pin)
{
    // Cancel any scheduled send tasks for this pin
    String taskId = String("send_") + pin;
    _scheduledTasks.erase(taskId);
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

void DecentIoTClass::_subscribeAllPubSub()
{
    for (auto &handler : _receiveHandlers) {
        String topic = _getTopic(handler.id.c_str());
        _pubsub.subscribe(topic.c_str());
    }
}

void DecentIoTClass::_publishDeviceStatus(bool online) {
    String topic = _projectId + "/users/" + _userId + "/datastreams/" + _deviceId + "/status";
    
    // Use proper time sync like Firebase library
    time_t unixTimestamp = time(nullptr);
    String payload = "{\"s\":" + String(online ? 1 : 0) + ",\"t\":" + String((unsigned long)unixTimestamp) + "}";
    
    // Use retained message so broker always has latest status
    if (_pubsub.connected()) {
        _pubsub.publish(topic.c_str(), payload.c_str(), true); // true = retained
        // Serial.printf("[STATUS] Device status updated: s=%d, t=%lu (%s)\n", 
        //              online ? 1 : 0, (unsigned long)unixTimestamp, ctime(&unixTimestamp));
    }
}