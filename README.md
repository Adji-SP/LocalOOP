# DNA - Data and Analytics System

## Project Overview

This is a PlatformIO project for Arduino MEGA2560 that implements a sensor data logging system with dual storage capabilities. The system collects temperature and weight sensor data and stores it both locally (EEPROM) and remotely (Firebase).

## Current Status

**Version:** 1.0.0
**Platform:** Arduino MEGA2560
**Framework:** Arduino
**Status:** Development Phase - Simulation Mode

### Implemented Features

 **Core Data Collection System**
- Simulated temperature sensor (RTD simulation)
- Simulated weight sensor (Load cell simulation)
- Configurable sampling interval (5 seconds default)

 **Dual Storage Architecture**
- Local EEPROM storage (100 records max, 32 bytes per record)
- Firebase cloud storage with batch uploading
- Abstract DataStorage base class for extensibility

 **Data Management**
- CSV export functionality
- Serial command interface for data manipulation
- Automatic storage synchronization
- Error handling and status monitoring

 **Communication Infrastructure**
- ESP8266 WiFi module integration (Serial1)
- AT command communication protocol
- Firebase REST API integration
- ArduinoJson library for data serialization

### Dependencies

- **bblanchon/ArduinoJson** v6.21.0 - JSON parsing and generation
- **mobizt/Firebase Arduino Client Library** v4.3.0 - Firebase integration
- **adafruit/Adafruit MAX31865 library** v1.6.2 - RTD temperature sensor
- **bogde/HX711** v0.7.5 - Load cell amplifier

## Architecture

### Core Components

1. **SensorData.h** - Data structure and serialization
2. **DataStorage.h** - Abstract storage interface
3. **LocalStorage.h** - EEPROM-based local storage
4. **FirebaseStorage.h** - Cloud storage implementation
5. **Config.h** - System configuration and constants

## Hardware Setup

### Target Hardware
- Arduino MEGA2560 (current)
- ESP8266 WiFi module (Serial1 communication)
- MAX31865 RTD temperature sensor (planned)
- HX711 load cell amplifier (planned)

### Pin Configuration
- RTD CS Pin: 10
- Load Cell DOUT: 3
- Load Cell SCK: 2
- ESP8266: Serial1 (115200 baud)

## Serial Commands

The system provides an interactive serial interface:

- `r` - Read all local storage records
- `c` - Clear local storage
- `s` - Force sync to Firebase
- `e` - Export data to CSV format
- `h` - Show help

## Configuration

Update `Config.h` with your settings:

```cpp
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define FIREBASE_HOST "your-project.firebaseio.com"
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET"
```

## Current Development Phase

The system is currently in **simulation mode** for development and testing:

- Temperature sensor simulated with random variations (�1�C)
- Weight sensor simulated with random variations (�5kg)
- Firebase integration implemented but requires WiFi configuration
- EEPROM storage fully functional

## Next Steps

1. **Hardware Integration**
   - Connect and integrate real RTD temperature sensor
   - Connect and integrate real load cell with HX711
   - Set up ESP8266 WiFi module

2. **Feature Enhancements**
   - Implement sensor calibration routines
   - Add data validation and filtering
   - Implement automatic retry mechanisms for failed uploads

3. **Testing & Validation**
   - Hardware-in-the-loop testing
   - Cloud connectivity testing
   - Long-term stability testing

## Building and Uploading

```bash
pio run                # Build the project
pio run --target upload # Upload to Arduino MEGA2560
pio device monitor     # Open serial monitor
```

## Monitoring

The system provides real-time status updates every 30 seconds showing:
- System uptime
- Storage status (local and cloud)
- Record counts
- Latest sensor readings

## License

This project is part of a data analytics system development.