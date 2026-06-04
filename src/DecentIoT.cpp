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
    
    // Set up PubSubClient with larger buffer for reliability
    _pubsub.setBufferSize(512); // Increase from default 256 bytes
    _pubsub.setServer(_broker.c_str(), _port);
    _pubsub.setCallback([this](char* topic, byte* payload, unsigned int length) {
        _handleMessage(topic, payload, length);
    });
    
    String clientId = "DecentIoT-" + String(random(0xffff), HEX);
    
    // Try to connect with proper error handling
    Serial.println("🔗 Connecting to MQTT broker via TLS...");
    Serial.printf("[DecentIoT] Client ID: %s\n", clientId.c_str());
    
    if (_pubsub.connect(clientId.c_str(), _username.c_str(), _password.c_str())) {
        Serial.println("✅ MQTT TLS connection successful");
        _subscribeAllPubSub();
        _publishDeviceStatus(true); // true = online
        _wasWiFiConnected = true; // Mark WiFi as connected after successful MQTT connection
    } else {
        Serial.println("❌ MQTT TLS connection failed");
        Serial.printf("[DecentIoT] Connection state: %d\n", _pubsub.state());
        Serial.println("[DecentIoT] State codes: -4=timeout, -3=lost, -2=failed, -1=disconnected");
        Serial.println("[DecentIoT] 1=bad protocol, 2=bad client ID, 3=unavailable, 4=bad credentials, 5=unauthorized");
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
    return _projectId + "/users/" + _userId + "/datastreams/" + _deviceId + "/" + pin + "/value";
}

void DecentIoTClass::_handleMessage(const char *topic, const uint8_t *payload, unsigned int length)
{
    String topicStr(topic);
    //String pin = topicStr.substring(topicStr.lastIndexOf('/') + 1); // if not upto value
    int lastSlash = topicStr.lastIndexOf('/');
    int secondLastSlash = topicStr.lastIndexOf('/', lastSlash - 1);
    String pin = topicStr.substring(secondLastSlash + 1, lastSlash);
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

// ====================================================================
// DecentIoTGps: Zero-RAM NMEA GPS Parser Implementation
// ====================================================================

DecentIoTGps::DecentIoTGps() : _index(0), _hasFix(false), _latitude(0.0f), _longitude(0.0f), _altitude(NAN), _speed(NAN), _time("") {}

bool DecentIoTGps::encode(char c)
{
    if (c == '$')
    {
        _index = 0;
        _buffer[_index++] = c;
        return false;
    }
    
    if (_index == 0)
    {
        return false; // Waiting for start character '$'
    }
    
    if (c == '\r' || c == '\n')
    {
        if (_index > 0)
        {
            _buffer[_index] = '\0';
            _index = 0;
            // Check if sentence matches $--RMC (character 3, 4, 5 are 'R', 'M', 'C')
            if (strlen(_buffer) > 6 && _buffer[3] == 'R' && _buffer[4] == 'M' && _buffer[5] == 'C')
            {
                if (checkChecksum(_buffer))
                {
                    parseRMC(_buffer);
                    return true;
                }
            }
        }
        _index = 0;
        return false;
    }
    
    if (_index < sizeof(_buffer) - 1)
    {
        _buffer[_index++] = c;
    }
    else
    {
        _index = 0; // Overflow, reset
    }
    return false;
}

bool DecentIoTGps::checkChecksum(const char *sentence)
{
    const char *star = strchr(sentence, '*');
    if (!star)
    {
        return true; // Lenient if no checksum field
    }
    
    uint8_t calculated = 0;
    for (const char *p = sentence + 1; p < star; ++p)
    {
        calculated ^= *p;
    }
    
    char hex[3];
    hex[0] = star[1];
    hex[1] = star[2];
    hex[2] = '\0';
    uint8_t received = (uint8_t)strtol(hex, nullptr, 16);
    
    return calculated == received;
}

const char *DecentIoTGps::getField(const char *str, int fieldIndex, char *fieldBuffer, int maxLen)
{
    int currentField = 0;
    const char *p = str;
    
    while (*p && currentField < fieldIndex)
    {
        if (*p == ',')
        {
            currentField++;
        }
        p++;
    }
    
    if (currentField != fieldIndex)
    {
        fieldBuffer[0] = '\0';
        return nullptr;
    }
    
    int i = 0;
    while (*p && *p != ',' && *p != '*' && i < maxLen - 1)
    {
        fieldBuffer[i++] = *p++;
    }
    fieldBuffer[i] = '\0';
    return fieldBuffer;
}

float DecentIoTGps::parseDegree(const char *val, char dir)
{
    float raw = atof(val);
    int degrees = (int)(raw / 100);
    float minutes = raw - (degrees * 100);
    float decimal = degrees + (minutes / 60.0f);
    
    if (dir == 'S' || dir == 'W')
    {
        decimal = -decimal;
    }
    return decimal;
}

void DecentIoTGps::parseRMC(char *sentence)
{
    char field[32];
    
    // Field 2: Status (A = Active/Valid, V = Warning/Invalid)
    if (!getField(sentence, 2, field, sizeof(field)) || field[0] != 'A')
    {
        _hasFix = false;
        return;
    }
    
    // Field 1: UTC Time (hhmmss.sss)
    if (getField(sentence, 1, field, sizeof(field)) && strlen(field) >= 6)
    {
        char timeStr[7];
        strncpy(timeStr, field, 6);
        timeStr[6] = '\0';
        _time = String(timeStr);
    }
    
    char latField[32];
    char latDir[2];
    char lonField[32];
    char lonDir[2];
    
    bool hasLat = getField(sentence, 3, latField, sizeof(latField)) &&
                  getField(sentence, 4, latDir, sizeof(latDir)) &&
                  strlen(latField) > 0 && strlen(latDir) > 0;
                  
    bool hasLon = getField(sentence, 5, lonField, sizeof(lonField)) &&
                  getField(sentence, 6, lonDir, sizeof(lonDir)) &&
                  strlen(lonField) > 0 && strlen(lonDir) > 0;
                  
    if (hasLat && hasLon)
    {
        _latitude = parseDegree(latField, latDir[0]);
        _longitude = parseDegree(lonField, lonDir[0]);
        _hasFix = true;
    }
    else
    {
        _hasFix = false;
    }
    
    // Field 7: Speed in knots
    if (getField(sentence, 7, field, sizeof(field)) && strlen(field) > 0)
    {
        float speedKnots = atof(field);
        _speed = speedKnots * 1.852f; // convert knots to km/h
    }
    else
    {
        _speed = NAN;
    }
    
    _altitude = NAN;
}

// ====================================================================
// DecentIoTClass: GPS Implementation Overloads
// ====================================================================

void DecentIoTClass::feedGPS(char c)
{
    gps.encode(c);
}

void DecentIoTClass::writeGPS(const char *pin)
{
    if (!gps.hasFix())
    {
        Serial.println("⚠️  No GPS fix, skipping GPS message");
        return;
    }

    String timeStr = gps.time();
    String payload = String(gps.latitude(), 6) + "|" +
                     String(gps.longitude(), 6) + "|" +
                     (isnan(gps.altitude()) ? "" : String(gps.altitude(), 1)) + "|" +
                     (isnan(gps.speed()) ? "" : String(gps.speed(), 2)) + "|" +
                     timeStr;

    String topic = _getTopic(pin);
    if (_pubsub.connected())
    {
        _pubsub.publish(topic.c_str(), payload.c_str(), true);
        Serial.printf("[GPS] Sent native location: %s\n", payload.c_str());
    }
    else
    {
        Serial.println("⚠️  MQTT not connected, skipping GPS message");
    }
}

void DecentIoTClass::writeGPS(const char *pin, const GPSData &gpsData)
{
    if (!gpsData.isValid())
    {
        Serial.println("⚠️  Invalid GPS data, skipping GPS message");
        return;
    }

    time_t ts = gpsData.timestamp > 0 ? gpsData.timestamp : time(nullptr);
    struct tm *timeinfo = gmtime(&ts);
    char timeBuf[7];
    if (timeinfo)
    {
        sprintf(timeBuf, "%02d%02d%02d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    }
    else
    {
        strcpy(timeBuf, "000000");
    }

    String payload = String(gpsData.latitude, 6) + "|" +
                     String(gpsData.longitude, 6) + "|" +
                     (isnan(gpsData.altitude) ? "" : String(gpsData.altitude, 1)) + "|" +
                     (isnan(gpsData.speed) ? "" : String(gpsData.speed, 2)) + "|" +
                     String(timeBuf);

    String topic = _getTopic(pin);
    if (_pubsub.connected())
    {
        _pubsub.publish(topic.c_str(), payload.c_str(), true);
        Serial.printf("[GPS] Sent struct location: %s\n", payload.c_str());
    }
    else
    {
        Serial.println("⚠️  MQTT not connected, skipping GPS message");
    }
}

void DecentIoTClass::writeGPS(const char *pin, float latitude, float longitude,
                              float altitude, float speed, float accuracy)
{
    GPSData gpsData(latitude, longitude, altitude, speed, accuracy);
    writeGPS(pin, gpsData);
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
    unsigned long currentMillis = millis();
    bool wifiCurrentlyConnected = (WiFi.status() == WL_CONNECTED);
    
    // 1. If WiFi is down, can't do anything
    if (!wifiCurrentlyConnected)
    {
        if (_wasWiFiConnected)
        {
            _wasWiFiConnected = false;
            _pubsub.disconnect();
        }
        return;
    }
    
    // 2. WiFi is connected - check if we need to handle reconnection
    if (!_wasWiFiConnected)
    {
        // WiFi just came back online - force MQTT reconnection
        Serial.println("[DecentIoT] WiFi reconnected, attempting MQTT reconnection...");
        _wasWiFiConnected = true;
        _lastReconnectAttempt = 0;
        
        delay(2000); // Wait for network stability
        
        if (reconnectMQTT())
        {
            Serial.println("[DecentIoT] MQTT reconnected successfully");
            _subscribeAllPubSub();
            _publishDeviceStatus(true);
        }
        return;
    }
    
    // 3. WiFi is up and was up before - check MQTT connection
    if (!_pubsub.connected())
    {
        handleReconnection();
        return;
    }
    
    // 4. Everything is connected - process MQTT messages
    _pubsub.loop();
    
    // 5. Continue normal operations
    processScheduledTasks();
    
    // 6. Update device status periodically
    if (currentMillis - _lastStatusUpdate >= _statusUpdateInterval)
    {
        _publishDeviceStatus(true);
        _lastStatusUpdate = currentMillis;
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
    
    // Send just the timestamp - presence indicates online status
    time_t unixTimestamp = time(nullptr);
    String payload = String((unsigned long)unixTimestamp);
    
    // Use retained message so broker always has latest status
    if (_pubsub.connected()) {
        _pubsub.publish(topic.c_str(), payload.c_str(), true); // true = retained
        // Serial.printf("[STATUS] Device status updated: %lu (%s)\n", 
        //              (unsigned long)unixTimestamp, ctime(&unixTimestamp));
    }
}

void DecentIoTClass::handleReconnection()
{
    unsigned long currentMillis = millis();
    
    // Check if WiFi is connected first
    if (WiFi.status() != WL_CONNECTED)
    {
        return;
    }
    
    // Throttle reconnection attempts
    if (currentMillis - _lastReconnectAttempt < _reconnectInterval)
    {
        return;
    }
    
    _lastReconnectAttempt = currentMillis;
    
    // Try to reconnect (silent unless it succeeds or fails badly)
    if (reconnectMQTT())
    {
        Serial.println("[DecentIoT] MQTT reconnected");
        _subscribeAllPubSub();
        _publishDeviceStatus(true);
    }
}

bool DecentIoTClass::reconnectMQTT()
{
    // Clean disconnect and stop client
    _pubsub.disconnect();
    _client.stop();
    delay(1000);
    
    // Verify time is synchronized (critical for SSL/TLS)
    time_t now = time(nullptr);
    if (now < 24 * 3600)
    {
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        int retry = 0;
        while (now < 24 * 3600 && retry < 15)
        {
            delay(500);
            now = time(nullptr);
            retry++;
        }
        if (now < 24 * 3600)
        {
            Serial.println("[DecentIoT] WARNING: Time sync failed");
        }
    }
    
    // Reinitialize SSL/TLS
#ifdef ESP8266
    if (_cert != nullptr)
    {
        _client.setTrustAnchors(_cert);
    }
    _client.setInsecure();
#elif defined(ESP32)
    _client.setCACert(root_ca);
#endif
    
    // Reinitialize PubSubClient
    _pubsub.setClient(_client);
    _pubsub.setBufferSize(512);
    _pubsub.setServer(_broker.c_str(), _port);
    _pubsub.setCallback([this](char* topic, byte* payload, unsigned int length) {
        _handleMessage(topic, payload, length);
    });
    
    // Try to connect
    String clientId = "DecentIoT-" + String(random(0xffff), HEX);
    return _pubsub.connect(clientId.c_str(), _username.c_str(), _password.c_str());
}