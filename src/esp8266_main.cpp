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
#include "SystemConfig.h"
#include "SensorData.h"
#include "LocalStorage.h"
#include "TimeSync.h"

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool wifiConnected = false;
bool firebaseReady = false;

// LocalStorage instance (reuse MEGA's class!)
LocalStorage *localStorage;

// TimeSync instance
TimeSync timeSync;

// Timing
unsigned long lastStatusTime = 0;
unsigned long lastWiFiCheck = 0;
unsigned long lastTimeBroadcast = 0;

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

// Upload all EEPROM data to Firestore
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

        // Create Firestore document path: sensor_data/{timestamp}
        char documentPath[128];
        snprintf(documentPath, sizeof(documentPath), "sensor_data/%lu", data.timestamp);

        // Create JSON document with all fields
        FirebaseJson json;
        json.set("fields/temp/doubleValue", data.temperature());
        json.set("fields/weight/doubleValue", data.weight());
        json.set("fields/ka/doubleValue", data.kadarAir);  // Added moisture content!
        json.set("fields/relay1/integerValue", String(data.relay1));
        json.set("fields/relay2/integerValue", String(data.relay2));
        json.set("fields/status/integerValue", String(data.status));
        json.set("fields/device/stringValue", DEVICE_NAME);
        json.set("fields/timestamp/integerValue", String(data.timestamp));

        // Upload to Firestore using patchDocument (creates or updates)
        // This prevents "Document already exists" errors
        if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath, json.raw(), ""))
        {
            uploaded++;
            Serial.print("STATUS:OK ");
            Serial.print(uploaded);
            Serial.print("/");
            Serial.println(totalRecords);
        }
        else
        {
            Serial.print("STATUS:Upload error: ");
            Serial.println(fbdo.errorReason());

            // If error is "Not Found", try createDocument as fallback
            if (String(fbdo.errorReason()).indexOf("NOT_FOUND") >= 0)
            {
                if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath, json.raw()))
                {
                    uploaded++;
                    Serial.println("STATUS:Created new document");
                }
            }
            else
            {
                break; // Stop on other errors
            }
        }

        if (!wifiConnected)
            break;

        delay(200); // Delay between Firestore writes (slower than RTDB)
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

        // Initialize time sync
        Serial.println("TIME:Initializing time sync...");
        if (timeSync.begin())
        {
            Serial.print("TIME:Synced! Unix time: ");
            Serial.println(timeSync.getUnixTime());

            // Send time to Mega immediately
            Serial.print("TIME:");
            Serial.println(timeSync.getUnixTime());
        }
        else
        {
            Serial.println("TIME:Failed to sync, will retry...");
        }
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

    // Update time sync (auto re-sync every 24h)
    timeSync.update();

    // Broadcast time to Mega every 60 seconds
    if (timeSync.isSynced() && (currentTime - lastTimeBroadcast >= 60000))
    {
        lastTimeBroadcast = currentTime;
        Serial.print("TIME:");
        Serial.println(timeSync.getUnixTime());
    }

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

            // Try to sync time if not synced yet
            if (!timeSync.isSynced())
            {
                timeSync.syncTimeFromAPI();
            }
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
                // Convert JSON to SensorData (stores temp, weight, ka, relay states)
                SensorData data;
                data.temperature() = doc["temp"] | 0.0;
                data.weight() = doc["weight"] | 0.0;
                data.kadarAir = doc["ka"] | 0.0;  // Now saving kadar air!
                data.relay1 = doc["relay1"] | 0;
                data.relay2 = doc["relay2"] | 0;

                // Use real Unix timestamp if time is synced, otherwise use millis
                if (timeSync.isSynced())
                {
                    data.timestamp = timeSync.getUnixTime();
                    Serial.print("DATA:Using Unix time: ");
                    Serial.println(data.timestamp);
                }
                else
                {
                    data.timestamp = millis() / 1000; // Fallback to millis
                    Serial.print("DATA:Using millis (time not synced): ");
                    Serial.println(data.timestamp);
                }

                data.status = STATUS_OK;

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
