/**
 * @file esp8266_main.cpp
 * @brief ESP8266 - WiFi Upload Manager with LocalStorage
 *
 * Job: Receive JSON from MEGA -> Save to EEPROM -> Upload to Firebase at 75%
 */

#ifdef ESP8266

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "SystemConfig.h"
#include "SensorData.h"
#include "LocalStorage.h"

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool wifiConnected = false;
bool firebaseReady = false;

// LocalStorage instance (reuse MEGA's class!)
LocalStorage* localStorage;

// Timing
unsigned long lastStatusTime = 0;
unsigned long lastWiFiCheck = 0;

// Connect WiFi
void connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < ESP_RECONNECT_ATTEMPTS) {
        delay(250);
        attempts++;
    }

    wifiConnected = (WiFi.status() == WL_CONNECTED);

    if (wifiConnected && !firebaseReady) {
        config.database_url = DATABASE_URL;
        config.signer.tokens.legacy_token = FIREBASE_AUTH;
        Firebase.reconnectWiFi(true);
        Firebase.begin(&config, &auth);
        firebaseReady = true;
    }
}

// Upload all EEPROM data to Firebase
int uploadAllData() {
    if (!wifiConnected || !firebaseReady || !Firebase.ready()) {
        return 0;
    }

    int totalRecords = localStorage->getRecordCount();
    int uploaded = 0;

    for (int i = 0; i < totalRecords; i++) {
        SensorData data;
        if (!localStorage->retrieveData(data, i)) continue;

        // Use char buffer to avoid String memory leaks
        char path[80];
        snprintf(path, sizeof(path), "%s/%lu", FB_DATA_PATH, data.timestamp);

        char tempPath[90], weightPath[90], statusPath[90];
        snprintf(tempPath, sizeof(tempPath), "%s/temperature", path);
        snprintf(weightPath, sizeof(weightPath), "%s/weight", path);
        snprintf(statusPath, sizeof(statusPath), "%s/status", path);

        bool success =
            Firebase.RTDB.setFloat(&fbdo, tempPath, data.temperature()) &&
            Firebase.RTDB.setFloat(&fbdo, weightPath, data.weight()) &&
            Firebase.RTDB.setInt(&fbdo, statusPath, data.status);

        if (success) {
            uploaded++;
        } else {
            break;  // Stop on error
        }

        if (!wifiConnected) break;
    }

    // Clear EEPROM if all uploaded
    if (uploaded == totalRecords) {
        localStorage->clearStorage();
        Serial.println("CLEAR");  // Tell MEGA to clear its EEPROM too
    }

    return uploaded;
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Send boot message
    Serial.println("STATUS:ESP8266 booting...");

    // Initialize LocalStorage (same as MEGA!)
    localStorage = new LocalStorage(MAX_RECORDS, RECORD_SIZE);
    if (!localStorage->initialize()) {
        while (true) {
            Serial.println("STATUS:ERROR - Storage init failed");
            delay(5000);
        }
    }

    Serial.println("STATUS:Storage initialized");

    connectWiFi();

    if (wifiConnected) {
        Serial.println("STATUS:WiFi connected!");
    } else {
        Serial.println("STATUS:WiFi connection failed");
    }

    Serial.println("STATUS:ESP8266 ready");
}

void loop() {
    unsigned long currentTime = millis();

    // Check WiFi every 30 seconds
    if (currentTime - lastWiFiCheck >= 30000) {
        lastWiFiCheck = currentTime;
        if (WiFi.status() != WL_CONNECTED) {
            wifiConnected = false;
            firebaseReady = false;
            connectWiFi();
        } else {
            wifiConnected = true;
        }
    }

    // Receive JSON from MEGA
    if (Serial.available()) {
        String json = Serial.readStringUntil('\n');
        json.trim();

        // Bounds check to prevent memory overflow
        if (json.length() > 300) {
            Serial.println(F("STATUS:JSON too large, discarding"));
            Serial.flush();
            return;
        }

        if (json.length() > 0 && json.startsWith("{")) {
            // Echo back received JSON for debugging
            Serial.println(json);

            // Parse JSON
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, json);

            if (!error) {
                // Convert JSON to SensorData
                SensorData data;
                data.temperature() = doc["temp"] | 0.0;
                data.weight() = doc["weight"] | 0.0;
                data.timestamp = doc["ts"] | 0;
                data.status = STATUS_OK;

                // Save to EEPROM using LocalStorage
                if (localStorage->saveData(data)) {
                    // Saved successfully - show current count occasionally
                } else {
                    Serial.println("STATUS:Storage full!");
                }
            } else {
                Serial.println("STATUS:JSON parse error");
            }
        }
    }

    // Check if EEPROM ≥ 75% full
    int threshold = (MAX_RECORDS * 3) / 4;  // 75%
    int currentCount = localStorage->getRecordCount();

    if (currentCount >= threshold && wifiConnected && firebaseReady && Firebase.ready()) {
        Serial.print(F("STATUS:Uploading "));
        Serial.print(currentCount);
        Serial.println(F(" records..."));

        int uploaded = uploadAllData();

        if (uploaded > 0) {
            Serial.print(F("UPLOADED:"));
            Serial.print(uploaded);
            Serial.println(F(" records"));
        } else {
            Serial.println(F("STATUS:Upload failed"));
        }
    }

    // Status every 15 seconds
    if (currentTime - lastStatusTime >= 15000) {
        lastStatusTime = currentTime;

        Serial.print(F("STATUS:"));
        Serial.print(wifiConnected ? F("WiFi OK, ") : F("WiFi FAIL, "));
        Serial.print(currentCount);
        Serial.print(F("/"));
        Serial.print(MAX_RECORDS);
        Serial.println(F(" records"));
    }

    delay(10);
}

#endif
