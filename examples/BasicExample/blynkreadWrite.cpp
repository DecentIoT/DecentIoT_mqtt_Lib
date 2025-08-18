/*
// Blynk credentials
#define BLYNK_TEMPLATE_ID "TMPL6XgXgXgXg"
#define BLYNK_TEMPLATE_NAME "IoT Solution"
#define BLYNK_AUTH_TOKEN "XgXgXgXgXgXgXgXgXgXgXgXgXgXgXgXg"

// Include Blynk library
#include <BlynkSimpleEsp32.h>

// Pin definitions
#define LED_PIN_1 2
#define LED_PIN_2 4
#define DHT_PIN 5
#define DHT_TYPE DHT11

// Blynk virtual pins
#define LED_1_VPIN V1
#define LED_2_VPIN V2
#define TEMP_VPIN V3
#define HUM_VPIN V4

// Initialize DHT sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Blynk timer
BlynkTimer timer;

// Function to read sensor data
void readSensorData() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
        Blynk.virtualWrite(TEMP_VPIN, temperature);
        Blynk.virtualWrite(HUM_VPIN, humidity);
    }
}

// Blynk virtual pin handlers
BLYNK_WRITE(LED_1_VPIN) {
    int value = param.asInt();
    digitalWrite(LED_PIN_1, value);
}

BLYNK_WRITE(LED_2_VPIN) {
    int value = param.asInt();
    digitalWrite(LED_PIN_2, value);
}

void setup() {
    Serial.begin(115200);

    // Initialize pins
    pinMode(LED_PIN_1, OUTPUT);
    pinMode(LED_PIN_2, OUTPUT);

    // Initialize DHT
    dht.begin();

    // Initialize Blynk
    Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS);

    // Setup timer for sensor reading
    timer.setInterval(2000L, readSensorData);
}

void loop() {
    Blynk.run();
    timer.run();
}
*/