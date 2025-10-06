/**
 * @file main.cpp
 * @brief MEGA2560 - Simple Sensor Broadcaster
 *
 * Job: ONLY read sensors -> send JSON to ESP
 * NO storage, NO logic, NO commands - just broadcast raw data
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SystemConfig.h"

// ESP Communication
#define ESP_SERIAL ESP8266_SERIAL

// Timing
unsigned long lastSampleTime = 0;
unsigned long checkup = 0;

// Simulated sensors
float simulatedTemp = 25.0;
float simulatedWeight = 100.0;

// Read sensors and return JSON

String readSensors()
{
    // Simulate sensor changes
    simulatedTemp += random(-100, 100) / 100.0;
    simulatedTemp = constrain(simulatedTemp, -50.0, 150.0);

    simulatedWeight += random(-500, 500) / 100.0;
    simulatedWeight = constrain(simulatedWeight, 0.0, 1000.0);

    // Create JSON
    StaticJsonDocument<200> doc;
    doc["temp"] = round(simulatedTemp * 100) / 100.0;
    doc["weight"] = round(simulatedWeight * 100) / 100.0;
    doc["ts"] = millis();

    String json;
    serializeJson(doc, json);
    return json;
}

// Display ALL ESP messages for debugging
void processESPMessages()
{
    while (ESP_SERIAL.available())
    {
        String msg = ESP_SERIAL.readStringUntil('\n');
        msg.trim();

        // Bounds check to prevent memory overflow
        if (msg.length() > 256) {
            ESP_SERIAL.flush();  // Clear corrupt data
            Serial.println(F("[ESP] Message too large, discarded"));
            continue;
        }

        if (msg.length() > 0)
        {
            // Show everything ESP sends
            Serial.println("[ESP] " + msg);
        }
    }
}


void setup()
{
    Serial.begin(115200);
    ESP_SERIAL.begin(ESP8266_BAUDRATE);
    delay(1000);

    Serial.println(F("\n================================"));
    Serial.println(F("MEGA2560 - Sensor Broadcaster"));
    Serial.println(F("================================"));
    Serial.println(F("Job: Read sensors -> Send JSON"));
    Serial.println(F("ESP handles all storage & upload"));
    Serial.println(F("================================\n"));

    // Wait for ESP to boot
    Serial.println(F("Waiting for ESP8266 to boot..."));
    delay(3000);

    // Clear boot garbage (74880 baud messages)
    while (ESP_SERIAL.available()) {
        ESP_SERIAL.read();
    }

    Serial.println(F("System ready\n"));
}

void loop()
{
    unsigned long currentTime = millis();

    // Read sensors every SAMPLE_INTERVAL (5 seconds) and send JSON
    if (currentTime - lastSampleTime >= SAMPLE_INTERVAL)
    {
        lastSampleTime = currentTime;

        String json = readSensors();

        // Send to ESP (no response expected)
        ESP_SERIAL.println(json);

        // Display locally
        Serial.print(F("[SENSOR] "));
        Serial.print(simulatedTemp, 2);
        Serial.print(F("°C, "));
        Serial.print(simulatedWeight, 2);
        Serial.print(F("kg -> Sent to ESP"));
        Serial.println();
    }
    processESPMessages();
    if (currentTime - checkup >= 10)
    {
        checkup = currentTime;
    }

    delay(10);
}


