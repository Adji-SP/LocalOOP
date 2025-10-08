/**
 * @file TimeSync.h
 * @brief Time Synchronization Module using WorldTimeAPI
 *
 * This module fetches real Unix timestamps from worldtimeapi.org
 * and stores the time offset in EEPROM for offline use.
 */

#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <Arduino.h>

class TimeSync
{
public:
    TimeSync();

    // Initialize time sync (call in setup)
    bool begin();

    // Get current Unix timestamp (seconds since 1970-01-01)
    unsigned long getUnixTime();

    // Sync time from API (returns true if successful)
    bool syncTimeFromAPI();

    // Check if time is synced
    bool isSynced();

    // Get last sync time (millis)
    unsigned long getLastSyncTime();

    // Auto-sync if needed (call in loop)
    void update();

    // Set time manually (for AVR receiving time from ESP8266)
    void setUnixTime(unsigned long unixTime);

private:
    unsigned long _bootUnixTime;      // Unix time when system booted
    unsigned long _bootMillis;        // millis() when time was synced
    bool _timeSynced;
    unsigned long _lastSyncAttempt;

    // EEPROM addresses for time storage
    static const int EEPROM_TIME_ADDR = 0;
    static const int EEPROM_MILLIS_ADDR = 4;
    static const int EEPROM_VALID_FLAG_ADDR = 8;

    // Save time to EEPROM
    void saveTimeToEEPROM();

    // Load time from EEPROM
    bool loadTimeFromEEPROM();
};

#endif // TIME_SYNC_H
