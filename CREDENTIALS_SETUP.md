# Credentials Setup Guide

## 📋 Overview
This project uses a centralized configuration file (`Credentials.h`) to manage all sensitive information including WiFi credentials, Firebase settings, and device configuration.

## 🔒 Security Notice
**IMPORTANT:** The `Credentials.h` file contains sensitive information and should **NEVER** be committed to version control. It is already listed in `.gitignore` for protection.

## 🚀 Setup Instructions

### Step 1: Create Your Credentials File
```bash
# Navigate to the include directory
cd include

# Copy the template file
cp Credentials.h.template Credentials.h
```

### Step 2: Configure WiFi Settings
Open `include/Credentials.h` and update:

```cpp
#define WIFI_SSID "YourWiFiName"           // Your WiFi network name
#define WIFI_PASSWORD "YourWiFiPassword"   // Your WiFi password
```

### Step 3: Configure Firebase Settings

#### Get Firebase Credentials:
1. Go to [Firebase Console](https://console.firebase.google.com)
2. Create a new project or select existing one
3. Go to **Realtime Database** → Create Database
4. Copy the **Database URL** (e.g., `https://your-project-12345.firebaseio.com`)

#### For Legacy Token (Recommended for simple setup):
1. Go to **Project Settings** → **Service Accounts** → **Database Secrets**
2. Copy or generate a new secret

#### Update Credentials.h:
```cpp
#define FIREBASE_HOST "your-project-12345.firebaseio.com"
#define FIREBASE_AUTH "your_database_secret_here"
#define API_KEY "your_web_api_key_here"
#define DATABASE_URL "https://your-project-12345.firebaseio.com"
```

### Step 4: Configure Device Settings
```cpp
#define DEVICE_NAME "MEGA_DNA_LOGGER_01"    // Unique name for this device
#define DEVICE_LOCATION "Laboratory_A"       // Physical location
```

## 📊 Firebase Database Structure

Your data will be stored in Firebase with this structure:

```
/devices
  /MEGA_DNA_LOGGER_01
    /data
      /1234567890
        temperature: 25.5
        weight: 123.4
        status: 1
        location: "Laboratory_A"
      /1234567891
        ...
    /status
      online: true
      last_sync: 1234567890
    /config
      sample_interval: 5000
      auto_sync: true
```

## ⚙️ Configuration Options

### Data Collection Settings
```cpp
#define DATA_SAMPLE_INTERVAL 5000      // Sample every 5 seconds
#define AUTO_SYNC_INTERVAL 300000      // Sync to Firebase every 5 minutes
#define DISPLAY_STATUS_INTERVAL 30000  // Display status every 30 seconds
```

### Local Storage Settings
```cpp
#define MAX_LOCAL_RECORDS 125          // Max records in EEPROM
#define RECORD_SIZE_BYTES 32           // Size per record
#define AUTO_CLEAR_AFTER_SYNC true     // Clear after successful sync
```

### Sensor Settings
```cpp
// RTD Temperature Sensor
#define RTD_ENABLED true
#define RTD_MIN_TEMP -50.0
#define RTD_MAX_TEMP 150.0
#define RTD_OFFSET 0.0                 // Calibration offset

// Load Cell
#define LOADCELL_ENABLED true
#define LOADCELL_MIN_WEIGHT 0.0
#define LOADCELL_MAX_WEIGHT 1000.0
#define LOADCELL_CALIBRATION 1.0       // Calibration factor
```

## 🔧 Troubleshooting

### Firebase Connection Issues
1. **Check WiFi:** Ensure WiFi credentials are correct
2. **Verify Database URL:** Must include `https://` for `DATABASE_URL`
3. **Check Auth Token:** Ensure legacy token or API key is valid
4. **Database Rules:** Set Firebase rules to allow write access:
```json
{
  "rules": {
    "devices": {
      ".read": true,
      ".write": true
    }
  }
}
```

### Local Storage Issues
- **EEPROM Full:** Reduce `MAX_LOCAL_RECORDS` or increase `AUTO_SYNC_INTERVAL`
- **Data Loss:** Enable `AUTO_CLEAR_AFTER_SYNC` to manage storage

### ESP8266 Communication Issues
- **No Response:** Check `ESP8266_SERIAL` is set to `Serial3` (pins 14/15 on MEGA)
- **Baud Rate:** Ensure `ESP_SERIAL_BAUD` matches (default: 115200)
- **DIP Switches:** Set to communication mode on RobotDyn board

## 📝 File Organization

```
DNA/
├── include/
│   ├── Credentials.h           ← Your actual credentials (NOT in git)
│   ├── Credentials.h.template  ← Template file (in git)
│   └── Config.h                ← Hardware config (uses Credentials.h)
├── src/
│   ├── main.cpp               ← ATmega2560 code
│   └── esp8266_main.cpp       ← ESP8266 code
└── .gitignore                 ← Protects Credentials.h
```

## 🔄 Updating Credentials

To update credentials after deployment:
1. Edit `include/Credentials.h`
2. Rebuild the project
3. Upload to ATmega2560 and/or ESP8266

## 🛡️ Security Best Practices

1. ✅ Never commit `Credentials.h` to public repositories
2. ✅ Use strong WiFi passwords
3. ✅ Rotate Firebase tokens periodically
4. ✅ Use Firebase security rules to restrict access
5. ✅ Keep backup of your credentials in a secure location

## 📞 Support

For issues or questions:
- Check Firebase Console for connection logs
- Enable `DEBUG_MODE true` in Credentials.h for verbose output
- Monitor Serial output at 115200 baud
