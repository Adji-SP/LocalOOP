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
LocalStorage *localStorage;

// Timing
unsigned long lastStatusTime = 0;
unsigned long lastWiFiCheck = 0;

// Connect WiFi
void connectWiFi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < ESP_RECONNECT_ATTEMPTS)
    {
        delay(250);
        attempts++;
    }

    wifiConnected = (WiFi.status() == WL_CONNECTED);

    if (wifiConnected && !firebaseReady)
    {
        Serial.print("STATUS:Configuring Firebase... API: ");
        Serial.println(API_KEY);
        Serial.print("STATUS:Database URL: ");
        Serial.println(DATABASE_URL);

        config.api_key = API_KEY;
        config.database_url = DATABASE_URL;
        config.timeout.serverResponse = 10 * 1000;

        Serial.println("STATUS:Signing in anonymously...");

        Firebase.reconnectWiFi(true);

        // Use signUp for anonymous authentication
        if (Firebase.signUp(&config, &auth, "", ""))
        {
            Serial.println("STATUS:Anonymous signup success");
            firebaseReady = true;
        }
        else
        {
            Serial.print("STATUS:Signup failed: ");
            Serial.println(config.signer.signupError.message.c_str());
        }

        Firebase.begin(&config, &auth);
    }
}

// Upload all EEPROM data to Firebase
int uploadAllData()
{
    if (!wifiConnected)
    {
        Serial.println("STATUS:Upload failed - WiFi not connected");
        return 0;
    }
    if (!firebaseReady)
    {
        Serial.println("STATUS:Upload failed - Firebase not initialized");
        return 0;
    }
    if (!Firebase.ready())
    {
        Serial.print("STATUS:Upload failed - Firebase not ready. Error: ");
        Serial.println(fbdo.errorReason());
        return 0;
    }

    int totalRecords = localStorage->getRecordCount();
    int uploaded = 0;
    const int BATCH_SIZE = 10; // Upload 10 at a time to prevent memory issues

    for (int i = 0; i < totalRecords && i < BATCH_SIZE; i++)
    {
        SensorData data;
        if (!localStorage->retrieveData(data, i))
            continue;

        // Use char buffer to avoid String memory leaks
        char path[80];
        snprintf(path, sizeof(path), "%s/%lu", FB_DATA_PATH, data.timestamp);

        char tempPath[100], weightPath[100], statusPath[100], relay1Path[100], relay2Path[100];
        snprintf(tempPath, sizeof(tempPath), "%s/temp", path);
        snprintf(weightPath, sizeof(weightPath), "%s/weight", path);
        snprintf(statusPath, sizeof(statusPath), "%s/status", path);
        snprintf(relay1Path, sizeof(relay1Path), "%s/relay1", path);
        snprintf(relay2Path, sizeof(relay2Path), "%s/relay2", path);

        // Upload one field at a time to save memory
        bool success = true;

        if (Firebase.RTDB.setFloat(&fbdo, tempPath, data.temperature())) {
            if (Firebase.RTDB.setFloat(&fbdo, weightPath, data.weight())) {
                if (Firebase.RTDB.setInt(&fbdo, statusPath, data.status)) {
                    if (Firebase.RTDB.setInt(&fbdo, relay1Path, data.relay1)) {
                        if (Firebase.RTDB.setInt(&fbdo, relay2Path, data.relay2)) {
                            uploaded++;
                            Serial.print("STATUS:OK ");
                            Serial.print(uploaded);
                            Serial.print("/");
                            Serial.println(totalRecords);
                        } else {
                            success = false;
                        }
                    } else {
                        success = false;
                    }
                } else {
                    success = false;
                }
            } else {
                success = false;
            }
        } else {
            success = false;
        }

        if (!success)
        {
            Serial.print("STATUS:Upload error: ");
            Serial.println(fbdo.errorReason());
            break;
        }

        if (!wifiConnected)
            break;

        delay(100); // Small delay between uploads
        yield(); // Let ESP8266 handle WiFi
    }

    // Clear storage after successful batch upload
    if (uploaded == BATCH_SIZE || uploaded == totalRecords)
    {
        localStorage->clearStorage();
        Serial.println("CLEAR"); // Tell MEGA to clear its EEPROM too
        Serial.print("STATUS:Cleared ");
        Serial.print(uploaded);
        Serial.println(" records from storage");
    }

    return uploaded;
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // Send boot message
    Serial.println("STATUS:ESP8266 booting...");

    // Initialize LocalStorage (same as MEGA!)
    localStorage = new LocalStorage(MAX_RECORDS, RECORD_SIZE);
    if (!localStorage->initialize())
    {
        while (true)
        {
            Serial.println("STATUS:ERROR - Storage init failed");
            delay(5000);
        }
    }

    Serial.println("STATUS:Storage initialized");

    connectWiFi();

    if (wifiConnected)
    {
        Serial.println("STATUS:WiFi connected!");
    }
    else
    {
        Serial.println("STATUS:WiFi connection failed");
    }

    Serial.println("STATUS:ESP8266 ready");
}

void loop()
{
    unsigned long currentTime = millis();

    // Check WiFi every 30 seconds
    if (currentTime - lastWiFiCheck >= 30000)
    {
        lastWiFiCheck = currentTime;
        if (WiFi.status() != WL_CONNECTED)
        {
            wifiConnected = false;
            firebaseReady = false;
            connectWiFi();
        }
        else
        {
            connectWiFi();
            wifiConnected = true;
        }
    }

    // Receive JSON from MEGA
    if (Serial.available())
    {
        String json = Serial.readStringUntil('\n');
        json.trim();

        // Bounds check to prevent memory overflow
        if (json.length() > 300)
        {
            Serial.println(F("STATUS:JSON too large, discarding"));
            Serial.flush();
            return;
        }

        if (json.length() > 0 && json.startsWith("{"))
        {
            // Parse JSON from MEGA
            // Expected format: {"temp":25.5,"weight":100.2,"ka":15.3,"ts":12345}
            StaticJsonDocument<250> doc;
            DeserializationError error = deserializeJson(doc, json);

            if (!error)
            {
                // Convert JSON to SensorData (stores temp, weight, relay states)
                SensorData data;
                data.temperature() = doc["temp"] | 0.0;
                data.weight() = doc["weight"] | 0.0;
                data.relay1 = doc["relay1"] | 0;
                data.relay2 = doc["relay2"] | 0;
                data.timestamp = doc["ts"] | 0;
                data.status = STATUS_OK;

                // Note: Kadar Air (ka) is received but not stored in EEPROM
                // It's displayed on HMI and sent to Firebase in real-time

                // Save to EEPROM using LocalStorage
                if (localStorage->saveData(data))
                {
                    // Saved successfully - echo back for confirmation
                    Serial.print(F("SAVED:"));
                    Serial.print(localStorage->getRecordCount());
                    Serial.print(F("/"));
                    Serial.println(MAX_RECORDS);
                }
                else
                {
                    Serial.println("STATUS:Storage full!");
                }
            }
            else
            {
                Serial.println("STATUS:JSON parse error");
            }
        }
    }
    if (!wifiConnected || !firebaseReady || !Firebase.ready())
    {
        // Serial.println("STATUS:ERROR - Firebase error");
    }

    // Upload when ≥ 10 records (lower threshold to prevent memory issues)
    int currentCount = localStorage->getRecordCount();

    if (currentCount >= 10 && wifiConnected && firebaseReady && Firebase.ready())
    {
        Serial.print(F("STATUS:Uploading batch from "));
        Serial.print(currentCount);
        Serial.println(F(" records..."));

        int uploaded = uploadAllData();

        if (uploaded > 0)
        {
            Serial.print(F("UPLOADED:"));
            Serial.print(uploaded);
            Serial.print(F(" records, "));
            Serial.print(localStorage->getRecordCount());
            Serial.println(F(" remaining"));
        }
        else
        {
            Serial.println(F("STATUS:Upload failed"));
        }
    }

    // Status every 15 seconds
    if (currentTime - lastStatusTime >= 15000)
    {
        lastStatusTime = currentTime;

        Serial.print(F("STATUS:"));
        Serial.print(wifiConnected ? F("WiFi OK, ") : F("WiFi FAIL, "));
        Serial.print(F("Firebase "));
        Serial.print(Firebase.ready() ? F("READY, ") : F("NOT READY, "));
        Serial.print(currentCount);
        Serial.print(F("/"));
        Serial.print(MAX_RECORDS);
        Serial.println(F(" records"));
    }

    delay(10);
}

#endif
