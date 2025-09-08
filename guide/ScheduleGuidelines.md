# DecentIoT Scheduling System - Complete Guidelines

## 📋 Overview

The DecentIoT library provides a comprehensive scheduling system that allows you to create recurring tasks, one-time tasks, and manage them efficiently. This system is available in both the **Firebase** and **MQTT** versions of the library.

## 🎯 Scheduling Features

### 1. **Recurring Tasks** - Tasks that run repeatedly at specified intervals
### 2. **One-Time Tasks** - Tasks that run once after a specified delay
### 3. **Task Cancellation** - Ability to cancel scheduled tasks
### 4. **Send Task Management** - Special handling for scheduled send operations
### 5. **Automatic Cleanup** - One-time tasks are automatically removed after execution

---

## 🔧 Available Methods

### **Basic Scheduling Methods**

#### `schedule(uint32_t interval, TaskCallback callback)`
Creates a recurring task with an auto-generated ID.

**Parameters:**
- `interval` - Time interval in milliseconds between executions
- `callback` - Function to execute

**Example:**
```cpp
// Schedule a task to run every 5 seconds
DecentIoT.schedule(5000, []() {
    Serial.println("This runs every 5 seconds");
    DecentIoT.write("P0", 25.5); // Send to any pin you want to schedule
});
```

#### `schedule(String taskId, uint32_t interval, TaskCallback callback)`
Creates a recurring task with a custom ID for better control.

**Parameters:**
- `taskId` - Custom identifier for the task
- `interval` - Time interval in milliseconds between executions
- `callback` - Function to execute

**Example:**
```cpp
// Schedule a sensor reading task with custom ID
DecentIoT.schedule("sensor_reading", 10000, []() {
    float temp = readTemperature();
    DecentIoT.write("P5", temp);
    Serial.printf("Temperature: %.2f°C\n", temp);
});
```

#### `scheduleOnce(uint32_t delay, TaskCallback callback)`
Creates a one-time task that runs once after the specified delay.

**Parameters:**
- `delay` - Delay in milliseconds before execution
- `callback` - Function to execute

**Example:**
```cpp
// Schedule a one-time task to run after 30 seconds
DecentIoT.scheduleOnce(30000, []() {
    Serial.println("This runs only once after 30 seconds");
    DecentIoT.write("P0", 1);  // Send ready signal to P0
});
```

### **Task Management Methods**

#### `cancel(String taskId)`
Cancels a scheduled task by its **exact task ID**.

**Parameters:**
- `taskId` - ID of the task to cancel

**Example:**
```cpp
// Cancel a specific task
DecentIoT.cancel("sensor_reading");
DecentIoT.cancel("send_P1");  // Also works for pin-based tasks
```

#### `cancelSend(const char *pin)`
Cancels scheduled send operations for a specific **pin** (easier method for pin-based tasks).

**Parameters:**
- `pin` - Pin name to cancel send operations for

**Example:**
```cpp
// Cancel scheduled sends for P1 pin
DecentIoT.cancelSend("P1");  // Cancels "send_P1" task
```

### **Key Difference: `.cancel()` vs `.cancelSend()`**

| Method | Works With | Task ID Format | Use Case |
|--------|------------|----------------|----------|
| `.cancelSend("P1")` | Pin-based tasks only | Pin names (P1, P2, P3) | Simple pin-based cancellation |
| `.cancel("send_P1")` | Any task | Exact task ID string | General task cancellation |
| `.cancel("my_task")` | Any task | Exact task ID string | Custom task cancellation |

**Recommendation**: Use `.cancelSend("P1")` for pin-based tasks (simpler) and `.cancel("task_id")` for custom tasks!

---

## 📚 Complete Usage Examples

### **Example 1: Pin-Based Scheduling (Recommended Approach)**

```cpp
#include <DecentIoT.h>
#include <WiFi.h>

// Pin definitions
#define LED_PIN 12
#define RGB_PIN 13

// Pin-based scheduled tasks using DECENTIOT_SEND macro
DECENTIOT_SEND(P1, 15000)  // Send temperature every 15 seconds
{
    int temperature = random(0, 101);
    DecentIoT.write(P1, temperature);
    Serial.printf("[P1] Temperature = %d\n", temperature);
}

DECENTIOT_SEND(P2, 20000)  // Send humidity every 20 seconds
{
    int humidity = random(0, 101);
    DecentIoT.write(P2, humidity);
    Serial.printf("[P2] Humidity = %d\n", humidity);
}

DECENTIOT_SEND(P3, 10000)  // Send system status every 10 seconds
{
    DecentIoT.write(P3, millis() / 1000);
    Serial.printf("[P3] Uptime = %lu seconds\n", millis() / 1000);
}

// Pin-based receive handlers
DECENTIOT_RECEIVE(P0)
{
    digitalWrite(LED_PIN, value);
    Serial.printf("[P0] LED state = %s\n", value ? "ON" : "OFF");
}

DECENTIOT_RECEIVE(P4)
{
    int brightness = value;
    analogWrite(RGB_PIN, brightness);
    Serial.printf("[P4] RGB Brightness = %d\n", brightness);
}

void setup() {
    Serial.begin(115200);
    
    // Initialize pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(RGB_PIN, OUTPUT);
    
    // Connect to WiFi
    WiFi.begin("your-ssid", "your-password");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    // Initialize DecentIoT
    DecentIoT.begin("project-id", "user-id", "device-id", 
                   "broker-url", 8883, "username", "password");
    
    // The DECENTIOT_SEND macros above automatically create scheduled tasks:
    // - "send_P1" task (every 15 seconds)
    // - "send_P2" task (every 20 seconds)  
    // - "send_P3" task (every 10 seconds)
}

void loop() {
    DecentIoT.run(); // Processes all scheduled tasks
    delay(100);
}
```

### **Example 2: Handler-Scheduling Integration (Advanced)**

```cpp
void setup() {
    // ... WiFi and DecentIoT initialization ...
    
    // Method 1: Schedule a send handler with custom ID
    DecentIoT.schedule("humidity_sender", 20000, []() {
        int humidity = random(0, 101);
        DecentIoT.write("P2", humidity);  // Send to P2 pin
        Serial.printf("[P2] Humidity = %d\n", humidity);
    });
    
    // Method 2: Schedule a temperature monitoring task
    DecentIoT.schedule("temperature_monitor", 15000, []() {
        float temp = readTemperature();
        DecentIoT.write("P1", temp);  // Send to P1 pin
        Serial.printf("[P1] Temperature = %.2f°C\n", temp);
    });
    
    // Method 3: Schedule system status updates
    DecentIoT.schedule("system_status", 30000, []() {
        DecentIoT.write("P3", ESP.getFreeHeap());  // Send free memory to P3
        DecentIoT.write("P4", millis() / 1000);    // Send uptime to P4
        Serial.println("System status updated");
    });
    
    // Method 4: One-time startup task
    DecentIoT.scheduleOnce("startup_complete", 5000, []() {
        DecentIoT.write("P0", 1);  // Send ready signal to P0
        Serial.println("Device startup completed");
    });
}

void loop() {
    DecentIoT.run();
    
    // Example: Cancel humidity sending after 5 minutes
    if (millis() > 300000) {  // 5 minutes
        DecentIoT.cancel("humidity_sender");
        Serial.println("Humidity sending cancelled");
    }
    
    // Example: Cancel temperature monitoring when sensor fails
    if (sensorFailed) {
        DecentIoT.cancel("temperature_monitor");
        Serial.println("Temperature monitoring stopped due to sensor failure");
    }
}
```

### **Example 3: Pin-Based Scheduling with Dynamic Management**

```cpp
void setup() {
    // ... WiFi and DecentIoT initialization ...
    
    // Start with scheduled sending using DECENTIOT_SEND macro
    DECENTIOT_SEND(P1, 15000)  // Send temperature every 15 seconds
    {
        int temperature = random(0, 101);
        DecentIoT.write(P1, temperature);
        Serial.printf("[P1] Temperature = %d\n", temperature);
    }
    
    DECENTIOT_SEND(P2, 20000)  // Send humidity every 20 seconds
    {
        int humidity = random(0, 101);
        DecentIoT.write(P2, humidity);
        Serial.printf("[P2] Humidity = %d\n", humidity);
    }
    
    // Schedule a system check task
    DecentIoT.schedule("system_check", 60000, []() {
        DecentIoT.write("P3", ESP.getFreeHeap());  // Send free memory to P3
        Serial.println("System check completed");
    });
}

void loop() {
    DecentIoT.run();
    
    // Example: Cancel P1 sending when temperature is too high
    if (temperature > 50) {
        DecentIoT.cancelSend("P1");
        Serial.println("P1 sending cancelled due to high temperature");
    }
    
    // Example: Restart P1 sending when temperature is normal
    if (temperature <= 50 && !DecentIoT.isScheduled("send_P1")) {
        // Note: You can't restart a cancelled DECENTIOT_SEND task
        // Use custom scheduling instead:
        DecentIoT.schedule("temp_restart", 15000, []() {
            int temperature = random(0, 101);
            DecentIoT.write(P1, temperature);
            Serial.printf("[P1] Temperature = %d\n", temperature);
        });
    }
    
    // Example: Cancel system check when in low power mode
    if (lowPowerMode) {
        DecentIoT.cancel("system_check");
        Serial.println("System check cancelled - low power mode");
    }
}
```

### **Example 4: Advanced Task Management with Conditional Logic**

```cpp
void setup() {
    // ... WiFi and DecentIoT initialization ...
    
    // Create multiple scheduled tasks with different purposes
    DecentIoT.schedule("heartbeat", 10000, []() {
        DecentIoT.write("P0", millis());  // Send heartbeat to P0
        Serial.println("Heartbeat sent");
    });
    
    DecentIoT.schedule("data_sync", 60000, []() {
        syncDataToCloud();
        DecentIoT.write("P1", 1);  // Send sync status to P1
        Serial.println("Data synchronized");
    });
    
    // Schedule a one-time startup task
    DecentIoT.scheduleOnce("startup", 5000, []() {
        DecentIoT.write("P2", 1);  // Send ready signal to P2
        Serial.println("Device startup complete");
    });
    
    // Schedule a delayed shutdown task (can be cancelled)
    DecentIoT.scheduleOnce("auto_shutdown", 300000, []() { // 5 minutes
        DecentIoT.write("P3", 0);  // Send shutdown signal to P3
        Serial.println("Auto-shutdown triggered");
    });
}

void loop() {
    DecentIoT.run();
    
    // Example: Cancel tasks based on conditions
    if (someCondition) {
        DecentIoT.cancel("data_sync");
        Serial.println("Data sync cancelled");
    }
    
    // Example: Cancel auto-shutdown if user is active
    if (userActive) {
        DecentIoT.cancel("auto_shutdown");
        Serial.println("Auto-shutdown cancelled - user active");
    }
    
    delay(100);
}
```


---

## 🔗 Handler-Scheduling Integration

### **Understanding the Relationship**

In DecentIoT, you can combine **handlers** (for receiving data) with **scheduling** (for sending data) to create powerful IoT applications:

```cpp
// Pin-based receive handlers (for incoming data)
DECENTIOT_RECEIVE(P0)  // Handle incoming LED control
{
    digitalWrite(LED_PIN, value);
    Serial.printf("[P0] LED state = %s\n", value ? "ON" : "OFF");
}

// Pin-based scheduled send tasks (for outgoing data)
DECENTIOT_SEND(P1, 15000)  // Send temperature every 15 seconds
{
    int temperature = random(0, 101);
    DecentIoT.write(P1, temperature);
    Serial.printf("[P1] Temperature = %d\n", temperature);
}
```

### **Key Integration Points**

1. **Pin Consistency**: Use the same pin names (P0, P1, P2, etc.) for both handlers and scheduled tasks
2. **Task ID Generation**: Scheduled send tasks automatically get IDs like "send_P1", "send_P2"
3. **Cancellation Methods**: Use `cancelSend("P1")` to stop scheduled sending to a specific pin
4. **Dynamic Management**: Start/stop scheduled tasks based on conditions or user input

### **Real-World Integration Example**

```cpp
// Pin-based handlers and scheduled tasks working together
DECENTIOT_RECEIVE(P0)  // Handle LED control from cloud
{
    digitalWrite(LED_PIN, value);
    Serial.printf("[P0] LED controlled from cloud: %s\n", value ? "ON" : "OFF");
}

DECENTIOT_SEND(P1, 15000)  // Send temperature to cloud every 15 seconds
{
    int temperature = random(0, 101);
    DecentIoT.write(P1, temperature);
    Serial.printf("[P1] Temperature sent to cloud: %d\n", temperature);
}

DECENTIOT_SEND(P2, 20000)  // Send humidity to cloud every 20 seconds
{
    int humidity = random(0, 101);
    DecentIoT.write(P2, humidity);
    Serial.printf("[P2] Humidity sent to cloud: %d\n", humidity);
}

void setup() {
    // ... initialization ...
    // Handlers and scheduled tasks are automatically registered
}

void loop() {
    DecentIoT.run();
    
    // Example: Stop temperature sending when LED is off
    if (digitalRead(LED_PIN) == LOW) {
        DecentIoT.cancelSend("P1");
        Serial.println("Temperature sending stopped - LED is off");
    }
    
    // Example: Restart temperature sending when LED is on
    if (digitalRead(LED_PIN) == HIGH && !DecentIoT.isScheduled("send_P1")) {
        // Restart using custom scheduling (since DECENTIOT_SEND can't be restarted)
        DecentIoT.schedule("temp_restart", 15000, []() {
            int temperature = random(0, 101);
            DecentIoT.write(P1, temperature);
            Serial.printf("[P1] Temperature sent to cloud: %d\n", temperature);
        });
    }
}
```

### **Handler-Scheduling Best Practices**

1. **Use Pin-Based Approach**: Always use P0, P1, P2, etc. for consistency
2. **Plan Your Pins**: Assign specific purposes to each pin (P0=LED, P1=Temperature, P2=Humidity, etc.)
3. **Dynamic Control**: Use `cancelSend()` and `schedule()` to control when data is sent
4. **Error Handling**: Check sensor availability before sending data
5. **Power Management**: Cancel unnecessary tasks in low power mode

---

## 🎯 Pin-Based vs Custom Task ID Scheduling

### **Pin-Based Scheduling (Recommended)**
This is the **primary approach** used in DecentIoT. It's simple, intuitive, and follows the pin-based architecture:

```cpp
// Pin-based scheduled tasks
DECENTIOT_SEND(P1, 15000)  // Creates task ID "send_P1"
{
    DecentIoT.write(P1, random(100));
}

DECENTIOT_SEND(P2, 20000)  // Creates task ID "send_P2"
{
    DecentIoT.write(P2, millis());
}

// Pin-based task cancellation
DecentIoT.cancelSend("P1");  // Cancels "send_P1" task
DecentIoT.cancelSend("P2");  // Cancels "send_P2" task
```

**Benefits:**
- ✅ **Simple and intuitive** - P1, P2, P3, etc.
- ✅ **Automatic task ID generation** - "send_P1", "send_P2"
- ✅ **Easy cancellation** - `cancelSend("P1")`
- ✅ **Consistent with pin architecture** - Matches your P0, P1, P2 system

### **Custom Task ID Scheduling (Advanced)**
For complex applications that need more control:

```cpp
// Custom task IDs
DecentIoT.schedule("sensor_reading", 15000, []() {
    // Custom task with ID "sensor_reading"
});

DecentIoT.schedule("system_check", 30000, []() {
    // Custom task with ID "system_check"
});

// Custom task cancellation
DecentIoT.cancel("sensor_reading");
DecentIoT.cancel("system_check");
```

**When to use:**
- 🔧 **Complex applications** with many tasks
- 🔧 **Dynamic task management** - creating/canceling tasks at runtime
- 🔧 **Non-pin related tasks** - system maintenance, logging, etc.

### **Mixed Approach (Best of Both)**
```cpp
// Pin-based tasks (for data sending)
DECENTIOT_SEND(P1, 15000) { /* temperature */ }
DECENTIOT_SEND(P2, 20000) { /* humidity */ }

// Custom tasks (for system operations)
DecentIoT.schedule("maintenance", 3600000, []() {
    // System maintenance every hour
});

DecentIoT.schedule("backup", 1800000, []() {
    // Data backup every 30 minutes
});

// Cancellation
DecentIoT.cancelSend("P1");        // Cancel pin-based task
DecentIoT.cancel("maintenance");   // Cancel custom task
```

---

## ⚡ Best Practices

### **1. Pin-Based Scheduling (Recommended)**
Use the pin-based approach for most applications:

```cpp
// ✅ Good: Pin-based scheduling
DECENTIOT_SEND(P1, 15000) { /* temperature */ }
DECENTIOT_SEND(P2, 20000) { /* humidity */ }
DECENTIOT_SEND(P3, 10000) { /* status */ }

// ✅ Good: Pin-based cancellation
DecentIoT.cancelSend("P1");
DecentIoT.cancelSend("P2");
```

### **2. Handler-Scheduling Integration**
Combine handlers and scheduled tasks effectively:

```cpp
// ✅ Good: Pin-based handlers and scheduled tasks
DECENTIOT_RECEIVE(P0) { /* handle LED control */ }
DECENTIOT_SEND(P1, 15000) { /* send temperature */ }
DECENTIOT_SEND(P2, 20000) { /* send humidity */ }

// ✅ Good: Dynamic task management
if (sensorFailed) {
    DecentIoT.cancelSend("P1");  // Stop temperature sending
}

// ✅ Good: Restart tasks when needed
if (sensorRecovered && !DecentIoT.isScheduled("send_P1")) {
    DecentIoT.schedule("temp_restart", 15000, []() {
        DecentIoT.write(P1, readTemperature());
    });
}
```

### **3. Custom Task ID Naming (When Needed)**
Use descriptive, consistent naming for custom task IDs:

```cpp
// ✅ Good naming
DecentIoT.schedule("sensor_temperature", 30000, callback);
DecentIoT.schedule("sensor_humidity", 45000, callback);
DecentIoT.schedule("system_heartbeat", 10000, callback);

// ❌ Avoid generic names
DecentIoT.schedule("task1", 30000, callback);
DecentIoT.schedule("temp", 30000, callback);
```

### **4. Task ID Management**
Understand how task IDs work in both approaches:

```cpp
// Pin-based approach: Automatic task ID generation
DECENTIOT_SEND(P1, 15000) { /* code */ }  // Creates task ID: "send_P1"
DECENTIOT_SEND(P2, 20000) { /* code */ }  // Creates task ID: "send_P2"

// Custom approach: Manual task ID specification
DecentIoT.schedule("my_custom_task", 10000, callback);  // Creates task ID: "my_custom_task"

// Cancellation methods
DecentIoT.cancelSend("P1");           // Cancels "send_P1" (easier)
DecentIoT.cancel("send_P1");          // Cancels "send_P1" (also works)
DecentIoT.cancel("my_custom_task");   // Cancels "my_custom_task"
```

### **5. Interval Selection**
Choose appropriate intervals based on your needs:

```cpp
// ✅ Good intervals for pin-based tasks
DECENTIOT_SEND(P1, 10000) { /* heartbeat */ }        // 10 seconds
DECENTIOT_SEND(P2, 30000) { /* sensors */ }          // 30 seconds
DECENTIOT_SEND(P3, 120000) { /* status */ }          // 2 minutes

// ✅ Good intervals for custom tasks
DecentIoT.schedule("maintenance", 3600000, callback);    // 1 hour

// ❌ Avoid too frequent tasks (unless necessary)
DECENTIOT_SEND(P1, 100) { /* code */ };            // Too frequent
```

### **6. Memory Management**
Be mindful of memory usage with many scheduled tasks:

```cpp
// ✅ Good: Cancel unused pin-based tasks
void stopSensorMonitoring() {
    DecentIoT.cancelSend("P1");  // Cancel temperature sensor
    DecentIoT.cancelSend("P2");  // Cancel humidity sensor
}

// ✅ Good: Cancel unused custom tasks
void stopCustomTasks() {
    DecentIoT.cancel("sensor_temperature");
    DecentIoT.cancel("sensor_humidity");
}

// ✅ Good: Use one-time tasks for temporary operations
DecentIoT.scheduleOnce(5000, []() {
    // One-time operation
});
```

### **7. Error Handling**
Always include error handling in your scheduled tasks:

```cpp
DecentIoT.schedule("sensor_reading", 30000, []() {
    if (sensorAvailable()) {
        float value = readSensor();
        if (!isnan(value)) {
            DecentIoT.write("P1", value);  // Send to P1 pin
        } else {
            Serial.println("Sensor reading failed");
        }
    } else {
        Serial.println("Sensor not available");
    }
});
```

---

## 🚀 Real-World Example (Based on Your Firmware)

Here's a complete example based on your actual firmware code:

```cpp
#include <DecentIoT.h>
#include <ESP8266WiFi.h>

// Pin definitions
#define LED_PIN D6  // D6 = GPIO12 on ESP8266
#define RGB_PIN D7  // D7 = GPIO13 on ESP8266

// Pin-based scheduled tasks (exactly like your firmware)
DECENTIOT_SEND(P1, 15000)  // Send temperature every 15 seconds
{
    int temperature = random(0, 101);
    DecentIoT.write(P1, temperature);
    Serial.printf("[P1] Temperature = %d\n", temperature);
}

DECENTIOT_SEND(P2, 15000)  // Send humidity every 15 seconds
{
    int humidity = random(0, 101);
    DecentIoT.write(P2, humidity);
    Serial.printf("[P2] Humidity = %d\n", humidity);
}

DECENTIOT_SEND(P4, 10000)  // Send status message every 10 seconds
{
    static int count = 0;
    String message = "ESP8266 Connected! Count: " + String(count++);
    DecentIoT.write(P4, message.c_str());
    Serial.println("[P4] Sent: " + message);
}

// Pin-based receive handlers
DECENTIOT_RECEIVE(P0)
{
    digitalWrite(LED_PIN, value);
    Serial.printf("[P0] LED state = %s\n", value ? "ON" : "OFF");
}

DECENTIOT_RECEIVE(P3)
{
    int sliderValue = value;
    int pwmValue = sliderValue * 255 / 100;
    analogWrite(RGB_PIN, pwmValue);
    Serial.printf("[P3] RGB Brightness = %d (slider: %d)\n", pwmValue, sliderValue);
}

void setup() {
    Serial.begin(115200);
    
    // Initialize pins
    pinMode(LED_PIN, OUTPUT);
    pinMode(RGB_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Connect to WiFi
    WiFi.begin("your-ssid", "your-password");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    // Initialize DecentIoT
    DecentIoT.begin("project-id", "user-id", "device-id", 
                   "broker-url", 8883, "username", "password");
    
    // The DECENTIOT_SEND macros automatically create these scheduled tasks:
    // - "send_P1" (every 15 seconds) - Temperature
    // - "send_P2" (every 15 seconds) - Humidity  
    // - "send_P4" (every 10 seconds) - Status message
}

void loop() {
    DecentIoT.run(); // Processes all scheduled tasks
    delay(10);
}

// Example: Dynamic task management
void stopTemperatureMonitoring() {
    DecentIoT.cancelSend("P1");  // Stops temperature sending
    Serial.println("Temperature monitoring stopped");
}

void startTemperatureMonitoring() {
    // Note: You can't restart a cancelled DECENTIOT_SEND task
    // You would need to use custom scheduling instead:
    DecentIoT.schedule("temp_monitor", 15000, []() {
        int temperature = random(0, 101);
        DecentIoT.write("P1", temperature);
        Serial.printf("[P1] Temperature = %d\n", temperature);
    });
}
```

**Key Points from Your Firmware:**
- ✅ **Pin-based scheduling** with `DECENTIOT_SEND(P1, 15000)`
- ✅ **Automatic task ID generation** - "send_P1", "send_P2", "send_P4"
- ✅ **Mixed intervals** - 15 seconds for sensors, 10 seconds for status
- ✅ **Pin-based cancellation** - `cancelSend("P1")` to stop temperature monitoring

---

## 🔍 Task Lifecycle

### **Recurring Tasks**
1. **Creation** - Task is created with specified interval
2. **Execution** - Task runs at regular intervals
3. **Continuation** - Task continues until manually cancelled
4. **Cancellation** - Task is removed from scheduler

### **One-Time Tasks**
1. **Creation** - Task is created with specified delay
2. **Waiting** - Task waits for the delay period
3. **Execution** - Task runs once
4. **Auto-Cleanup** - Task is automatically removed

---

## 🚨 Common Pitfalls and Solutions

### **Pitfall 1: Too Many Tasks**
```cpp
// ❌ Problem: Creating too many tasks
for (int i = 0; i < 100; i++) {
    DecentIoT.schedule("task_" + String(i), 1000, callback);
}

// ✅ Solution: Use fewer, more efficient tasks
DecentIoT.schedule("batch_processing", 1000, []() {
    // Process multiple items in one task
    for (int i = 0; i < 100; i++) {
        processItem(i);
    }
});
```

### **Pitfall 2: Blocking Tasks**
```cpp
// ❌ Problem: Blocking operations in scheduled tasks
DecentIoT.schedule("network_request", 5000, []() {
    delay(10000); // This blocks the entire system!
    makeNetworkRequest();
});

// ✅ Solution: Use non-blocking operations
DecentIoT.schedule("network_request", 5000, []() {
    startNetworkRequest(); // Non-blocking
});
```

### **Pitfall 3: Memory Leaks**
```cpp
// ❌ Problem: Not cancelling unused tasks
void setup() {
    DecentIoT.schedule("temp_sensor", 30000, callback);
    // Task continues even if not needed
}

// ✅ Solution: Cancel tasks when not needed
void stopMonitoring() {
    DecentIoT.cancel("temp_sensor");
}
```

---

## 📊 Performance Considerations

### **Task Processing Overhead**
- Each scheduled task adds minimal overhead
- `processScheduledTasks()` is called from `DecentIoT.run()`
- Avoid creating hundreds of tasks unnecessarily

### **Memory Usage**
- Each task uses approximately 20-30 bytes of RAM
- One-time tasks are automatically cleaned up
- Recurring tasks persist until manually cancelled

### **Timing Accuracy**
- Tasks are processed during `DecentIoT.run()` calls
- Accuracy depends on how frequently you call `run()`
- For best accuracy, call `run()` frequently in your main loop

---

## 🔧 Advanced Usage

### **Dynamic Task Management**
```cpp
class TaskManager {
private:
    String currentTaskId;
    
public:
    void startMonitoring() {
        currentTaskId = "monitoring_" + String(millis());
        DecentIoT.schedule(currentTaskId, 5000, []() {
            // Monitoring logic
        });
    }
    
    void stopMonitoring() {
        if (currentTaskId.length() > 0) {
            DecentIoT.cancel(currentTaskId);
            currentTaskId = "";
        }
    }
};
```

### **Conditional Task Execution**
```cpp
void setup() {
    DecentIoT.schedule("conditional_task", 10000, []() {
        if (shouldRunTask()) {
            executeTask();
        } else {
            Serial.println("Task skipped due to condition");
        }
    });
}
```

### **Task Chaining**
```cpp
void setup() {
    DecentIoT.scheduleOnce(5000, []() {
        Serial.println("First task completed");
        
        // Schedule next task
        DecentIoT.scheduleOnce(3000, []() {
            Serial.println("Second task completed");
            
            // Schedule final task
            DecentIoT.scheduleOnce(2000, []() {
                Serial.println("All tasks completed");
            });
        });
    });
}
```

---

## 📝 Summary

The DecentIoT scheduling system provides:

- ✅ **Pin-based scheduling** - Simple `DECENTIOT_SEND(P1, 15000)` approach
- ✅ **Custom task scheduling** - Advanced `schedule("custom_id", 15000, callback)` approach
- ✅ **Handler-scheduling integration** - Combine `DECENTIOT_RECEIVE(P0)` with `DECENTIOT_SEND(P1, 15000)`
- ✅ **Automatic task ID generation** - "send_P1", "send_P2" for pin-based tasks
- ✅ **Task management** - Cancel tasks by pin (`cancelSend("P1")`) or custom ID (`cancel("custom_id")`)
- ✅ **Dynamic task control** - Start/stop tasks based on conditions
- ✅ **Automatic cleanup** - One-time tasks are auto-removed
- ✅ **Memory efficient** - Minimal overhead per task
- ✅ **Production ready** - Robust and reliable

### **🎯 Quick Reference:**

**Pin-Based (Recommended):**
```cpp
DECENTIOT_SEND(P1, 15000) { /* code */ }  // Creates "send_P1" task
DecentIoT.cancelSend("P1");               // Cancels "send_P1" task
```

**Handler-Scheduling Integration:**
```cpp
DECENTIOT_RECEIVE(P0) { /* handle incoming data */ }  // Handle cloud commands
DECENTIOT_SEND(P1, 15000) { /* send data */ }         // Send data to cloud
DecentIoT.cancelSend("P1");                           // Stop sending to P1
```

**Custom Task ID (Advanced):**
```cpp
DecentIoT.schedule("my_task", 15000, callback);  // Creates "my_task" task
DecentIoT.cancel("my_task");                     // Cancels "my_task" task
```

Use the **pin-based approach** for most applications - it's simpler and more intuitive!

---

*For more information, visit the DecentIoT documentation or check the examples in the library.*
