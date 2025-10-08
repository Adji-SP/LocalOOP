/**
 * @file TimeSync.cpp
 * @brief Time Synchronization Implementation
 */

#include "TimeSync.h"

#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

TimeSync::TimeSync()
    : _bootUnixTime(0),
      _bootMillis(0),
      _timeSynced(false),
      _lastSyncAttempt(0)
{
}

bool TimeSync::begin()
{
    EEPROM.begin(512);

    // Try to load time from EEPROM first
    if (loadTimeFromEEPROM())
    {
        Serial.println("TIME:Loaded from EEPROM");
        return true;
    }

    // If EEPROM load failed, try to sync from API
    Serial.println("TIME:No valid EEPROM time, syncing from API...");
    return syncTimeFromAPI();
}

bool TimeSync::syncTimeFromAPI()
{
    // Don't spam the API - wait at least 10 seconds between attempts
    if (millis() - _lastSyncAttempt < 10000)
    {
        return false;
    }

    _lastSyncAttempt = millis();

    WiFiClient client;
    HTTPClient http;

    // Use worldtimeapi.org - free, no API key needed
    // Using Asia/Jakarta timezone (adjust for your location)
    const char *timeApiUrl = "http://worldtimeapi.org/api/timezone/Asia/Jakarta";

    Serial.print("TIME:Fetching time from ");
    Serial.println(timeApiUrl);

    http.begin(client, timeApiUrl);
    http.setTimeout(5000); // 5 second timeout

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String payload = http.getString();

        // Parse JSON response
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (!error)
        {
            // Get Unix timestamp from API
            unsigned long unixTime = doc["unixtime"];

            if (unixTime > 1609459200) // Sanity check: after 2021-01-01
            {
                _bootUnixTime = unixTime;
                _bootMillis = millis();
                _timeSynced = true;

                // Save to EEPROM
                saveTimeToEEPROM();

                Serial.print("TIME:Synced! Unix time: ");
                Serial.println(unixTime);

                http.end();
                return true;
            }
        }
        else
        {
            Serial.print("TIME:JSON parse error: ");
            Serial.println(error.c_str());
        }
    }
    else
    {
        Serial.print("TIME:HTTP error: ");
        Serial.println(httpCode);
    }

    http.end();
    return false;
}

unsigned long TimeSync::getUnixTime()
{
    if (!_timeSynced)
    {
        // Return 0 if not synced (caller should check with isSynced())
        return 0;
    }

    // Calculate current Unix time based on boot time and elapsed millis
    unsigned long elapsedSeconds = (millis() - _bootMillis) / 1000;
    return _bootUnixTime + elapsedSeconds;
}

bool TimeSync::isSynced()
{
    return _timeSynced;
}

unsigned long TimeSync::getLastSyncTime()
{
    return _lastSyncAttempt;
}

void TimeSync::update()
{
    // Auto re-sync every 24 hours
    const unsigned long RESYNC_INTERVAL = 24UL * 60 * 60 * 1000; // 24 hours

    if (_timeSynced && (millis() - _lastSyncAttempt > RESYNC_INTERVAL))
    {
        Serial.println("TIME:24h elapsed, re-syncing...");
        syncTimeFromAPI();
    }
}

void TimeSync::saveTimeToEEPROM()
{
    // Write Unix time (4 bytes)
    EEPROM.put(EEPROM_TIME_ADDR, _bootUnixTime);

    // Write boot millis (4 bytes)
    EEPROM.put(EEPROM_MILLIS_ADDR, _bootMillis);

    // Write valid flag (1 byte) - magic number 0xAB
    EEPROM.write(EEPROM_VALID_FLAG_ADDR, 0xAB);

    EEPROM.commit();

    Serial.println("TIME:Saved to EEPROM");
}

bool TimeSync::loadTimeFromEEPROM()
{
    // Check valid flag
    byte validFlag = EEPROM.read(EEPROM_VALID_FLAG_ADDR);
    if (validFlag != 0xAB)
    {
        Serial.println("TIME:EEPROM invalid flag");
        return false;
    }

    // Read Unix time
    unsigned long savedUnixTime;
    EEPROM.get(EEPROM_TIME_ADDR, savedUnixTime);

    // Read boot millis
    unsigned long savedBootMillis;
    EEPROM.get(EEPROM_MILLIS_ADDR, savedBootMillis);

    // Sanity check
    if (savedUnixTime < 1609459200) // Before 2021-01-01
    {
        Serial.println("TIME:EEPROM time too old");
        return false;
    }

    // Calculate elapsed time since EEPROM save
    unsigned long currentMillis = millis();
    unsigned long elapsedMillis = currentMillis - savedBootMillis;

    // If millis wrapped around or elapsed time is huge, reject it
    if (elapsedMillis > 604800000UL) // 7 days in milliseconds
    {
        Serial.println("TIME:EEPROM time too stale");
        return false;
    }

    // Restore time reference
    _bootUnixTime = savedUnixTime + (elapsedMillis / 1000);
    _bootMillis = currentMillis;
    _timeSynced = true;

    Serial.print("TIME:Restored from EEPROM: ");
    Serial.println(getUnixTime());

    return true;
}

#else
// AVR implementation (Mega2560) - receives time from ESP8266

TimeSync::TimeSync()
    : _bootUnixTime(0),
      _bootMillis(0),
      _timeSynced(false),
      _lastSyncAttempt(0)
{
}

bool TimeSync::begin()
{
    // On AVR, time will be set by ESP8266 via serial command
    _timeSynced = false;
    return true;
}

bool TimeSync::syncTimeFromAPI()
{
    // On AVR, this is a no-op
    // Time is received from ESP8266
    return false;
}

unsigned long TimeSync::getUnixTime()
{
    if (!_timeSynced)
    {
        // Use millis as fallback
        return millis() / 1000;
    }

    unsigned long elapsedSeconds = (millis() - _bootMillis) / 1000;
    return _bootUnixTime + elapsedSeconds;
}

bool TimeSync::isSynced()
{
    return _timeSynced;
}

unsigned long TimeSync::getLastSyncTime()
{
    return _lastSyncAttempt;
}

void TimeSync::update()
{
    // No-op on AVR
}

void TimeSync::saveTimeToEEPROM()
{
    // No EEPROM saving on AVR for time
}

bool TimeSync::loadTimeFromEEPROM()
{
    return false;
}

#endif

// Common implementation for both platforms
void TimeSync::setUnixTime(unsigned long unixTime)
{
    _bootUnixTime = unixTime;
    _bootMillis = millis();
    _timeSynced = true;

    Serial.print("TIME:Set to ");
    Serial.println(unixTime);
}
