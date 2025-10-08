# Real-Time Timestamp System

## Overview

Your system now uses **real Unix timestamps** instead of `millis()` for all sensor data!

The ESP8266 fetches the current time from **WorldTimeAPI.org** and synchronizes it with the Mega2560.

## How It Works

```
┌─────────────┐      ┌──────────────┐      ┌─────────────────┐
│   Mega2560  │◄────►│   ESP8266    │◄────►│ WorldTimeAPI    │
│             │      │              │      │ (Internet)      │
└─────────────┘      └──────────────┘      └─────────────────┘
  Receives time      Fetches & syncs       Returns Unix time
  from ESP8266       every 24 hours         (Asia/Jakarta)
```

### Flow:

1. **ESP8266 boots** → Connects to WiFi
2. **ESP8266 fetches time** from `http://worldtimeapi.org/api/timezone/Asia/Jakarta`
3. **ESP8266 stores time** in EEPROM (survives reboots!)
4. **ESP8266 broadcasts** `TIME:1234567890` to Mega every 60 seconds
5. **Mega2560 receives** time and uses it for all sensor data
6. **All timestamps** are now real Unix timestamps (seconds since 1970-01-01)

---

## Features

### ✅ Real Unix Timestamps
- All sensor data uses **real-world timestamps**
- Proper date/time in Firebase (not just milliseconds!)
- Easy to convert to human-readable dates

### ✅ EEPROM Storage
- Time is saved to ESP8266 EEPROM
- Survives power cycles and reboots
- Auto-restores on boot (if less than 7 days old)

### ✅ Auto Re-sync
- ESP8266 re-syncs every **24 hours**
- Keeps time accurate long-term
- Handles time drift automatically

### ✅ Fallback Mode
- If time sync fails, falls back to `millis() / 1000`
- System still works without internet
- Automatically syncs when internet returns

---

## Configuration

### Change Timezone

Edit `TimeSync.cpp` line 47:

```cpp
// Current: Asia/Jakarta (UTC+7)
const char *timeApiUrl = "http://worldtimeapi.org/api/timezone/Asia/Jakarta";

// For other timezones, see: http://worldtimeapi.org/api/timezone
// Examples:
// - America/New_York
// - Europe/London
// - Asia/Tokyo
```

**Available timezones:** http://worldtimeapi.org/api/timezone

### Change Sync Interval

Edit `TimeSync.cpp` line 155:

```cpp
// Current: 24 hours
const unsigned long RESYNC_INTERVAL = 24UL * 60 * 60 * 1000;

// Change to 12 hours:
const unsigned long RESYNC_INTERVAL = 12UL * 60 * 60 * 1000;

// Change to 1 hour:
const unsigned long RESYNC_INTERVAL = 1UL * 60 * 60 * 1000;
```

### Change Broadcast Interval (ESP → Mega)

Edit `esp8266_main.cpp` line 236:

```cpp
// Current: 60 seconds
if (timeSync.isSynced() && (currentTime - lastTimeBroadcast >= 60000))

// Change to 30 seconds:
if (timeSync.isSynced() && (currentTime - lastTimeBroadcast >= 30000))
```

---

## Serial Monitor Output

### ESP8266 Boot:

```
STATUS:ESP8266 booting...
STATUS:Storage initialized
Connecting to WiFi: bismillah
WiFi connected!
IP: 192.168.1.100

TIME:Initializing time sync...
TIME:Fetching time from http://worldtimeapi.org/api/timezone/Asia/Jakarta
TIME:Synced! Unix time: 1704067200
TIME:Saved to EEPROM
TIME:1704067200

STATUS:ESP8266 ready
```

### Mega2560 Boot:

```
MEGA2560 - Sensor Broadcaster
================================
Job: Read sensors -> Send JSON
ESP handles storage & upload
================================

HX711 initialized and tared
TimeSync initialized (waiting for ESP8266)
HMI initialized on Serial2 (RX2=17, TX2=16)
Testing HMI communication...

[ESP] TIME:Synced! Unix time: 1704067200
System ready
```

### During Operation:

```
[ESP] TIME:1704067260    ← Time broadcast every 60s
[ESP] STATUS:WiFi OK, Firebase READY, 5/125 records
━━━ SUMMARY ━━━ Temp: 25.50°C | Weight: 100.20kg | Moisture: 15.3% → ESP
```

---

## Timestamp Format

### Before (millis):
```json
{
  "temp": 25.5,
  "weight": 100.2,
  "ka": 15.3,
  "ts": 123456  ← milliseconds since boot (meaningless!)
}
```

### After (Unix timestamp):
```json
{
  "temp": 25.5,
  "weight": 100.2,
  "ka": 15.3,
  "ts": 1704067200  ← seconds since 1970-01-01 (real date/time!)
}
```

### Convert to Human-Readable Date:

**JavaScript:**
```javascript
const timestamp = 1704067200;
const date = new Date(timestamp * 1000);
console.log(date.toISOString());
// Output: 2024-01-01T00:00:00.000Z
```

**Python:**
```python
import datetime
timestamp = 1704067200
date = datetime.datetime.fromtimestamp(timestamp)
print(date)
# Output: 2024-01-01 00:00:00
```

**Online:**
- https://www.unixtimestamp.com/

---

## Firebase Data Structure

Your Firestore data now has real timestamps:

```
Collection: sensor_data
├── Document: 1704067200
│   ├── temp: 25.5
│   ├── weight: 100.2
│   ├── relay1: 1
│   ├── relay2: 0
│   ├── status: 1
│   ├── device: "MEGA_DNA_LOGGER"
│   └── timestamp: 1704067200  ← Real Unix timestamp!
├── Document: 1704067260
│   └── ...
```

You can now:
- **Sort by date/time** accurately
- **Query by date range** (e.g., "last 24 hours")
- **Display in graphs** with real X-axis timestamps
- **Export to CSV** with proper dates

---

## Troubleshooting

### Time not syncing?

**Check WiFi:**
```
[ESP] STATUS:WiFi FAIL
```
→ Check WiFi credentials in `SystemConfig.h`

**Check API access:**
```
[ESP] TIME:HTTP error: -1
```
→ ESP8266 can't reach WorldTimeAPI
→ Check internet connection
→ Try alternative API (see below)

**Check serial connection:**
```
Mega: No [ESP] TIME: messages
```
→ Check Serial3 connection between Mega and ESP
→ Make sure ESP8266 is running and connected

### Timestamp is 0?

```cpp
unsigned long ts = timeSync.getUnixTime();
if (ts == 0) {
    Serial.println("Time not synced yet!");
}
```

**Possible causes:**
- ESP8266 hasn't booted yet
- WiFi not connected
- Time API call failed
- Serial communication issue

**Solution:**
- Wait for ESP8266 to sync (check serial monitor)
- The system will use `millis() / 1000` as fallback

### Time is wrong by hours?

**Timezone mismatch!**

Current: `Asia/Jakarta` (UTC+7)

Change in `TimeSync.cpp` line 47 to your timezone.

### EEPROM time is stale?

```
TIME:EEPROM time too stale
```

This happens if:
- ESP8266 was offline for > 7 days
- `millis()` wrapped around (49 days uptime)

**Solution:** ESP8266 will fetch fresh time from API automatically.

---

## Alternative Time APIs

If WorldTimeAPI is down, you can use these alternatives:

### 1. WorldClockAPI

```cpp
const char *timeApiUrl = "http://worldclockapi.com/api/json/utc/now";

// Parse response differently:
unsigned long unixTime = doc["currentFileTime"] / 10000000 - 11644473600;
```

### 2. TimeAPI.io

```cpp
const char *timeApiUrl = "https://timeapi.io/api/Time/current/zone?timeZone=Asia/Jakarta";

// Parse response:
unsigned long unixTime = doc["seconds"];
```

### 3. NTP (More complex, but most reliable)

Install NTPClient library:
```cpp
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200); // UTC+7 offset

timeClient.begin();
timeClient.update();
unsigned long unixTime = timeClient.getEpochTime();
```

---

## API Usage Limits

**WorldTimeAPI:**
- ✅ Free
- ✅ No API key required
- ✅ Unlimited requests
- ❌ May be slow or offline sometimes

**Best practice:**
- Sync every 24 hours (current setting)
- Store in EEPROM (already implemented)
- Use fallback to millis if API fails (already implemented)

---

## Testing

### Manual Time Sync Test (ESP8266):

```cpp
void setup() {
    // ... existing code ...

    // Force time sync
    if (timeSync.syncTimeFromAPI()) {
        Serial.println("✓ Time sync successful!");
        Serial.print("Unix time: ");
        Serial.println(timeSync.getUnixTime());
    } else {
        Serial.println("✗ Time sync failed!");
    }
}
```

### Check if Time is Synced (Mega2560):

```cpp
void loop() {
    if (timeSync.isSynced()) {
        Serial.print("Current time: ");
        Serial.println(timeSync.getUnixTime());
    } else {
        Serial.println("Time not synced yet");
    }
}
```

---

## Summary

✅ **Real timestamps** instead of millis()
✅ **Auto-sync** from WorldTimeAPI every 24 hours
✅ **EEPROM storage** survives reboots
✅ **Fallback** to millis() if sync fails
✅ **ESP → Mega** time broadcast every 60 seconds
✅ **Firestore** now has meaningful timestamps

Your sensor data now has **proper date/time information** for analysis, graphing, and reporting! 🎉
