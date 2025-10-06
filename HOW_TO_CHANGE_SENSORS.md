# 🔄 How to Change Sensors - Quick Guide

## ✨ **It's Now DYNAMIC!**

You can change from **Temperature+Weight** to **MPU6050** (or any other sensor) by editing **just ONE line** in the config file!

---

## 📝 **Step-by-Step Guide**

### **Step 1: Open the Sensor Config File**
```
include/SensorConfig.h
```

### **Step 2: Choose Your Sensor Configuration**

Find this section and **uncomment ONE option**:

```cpp
// ========================================
// Sensor Type Selection
// ========================================

// Option 1: Temperature + Weight (Current) ✓
#define SENSOR_CONFIG_TEMP_WEIGHT

// Option 2: MPU6050 (Accelerometer + Gyroscope)
// #define SENSOR_CONFIG_MPU6050

// Option 3: Temperature + Humidity (DHT22)
// #define SENSOR_CONFIG_TEMP_HUMIDITY

// Option 4: Distance + Light (HC-SR04 + LDR)
// #define SENSOR_CONFIG_DISTANCE_LIGHT

// Option 5: Custom sensors
// #define SENSOR_CONFIG_CUSTOM
```

### **Step 3: Change Sensor Type**

**Example: Switch to MPU6050**

**BEFORE:**
```cpp
#define SENSOR_CONFIG_TEMP_WEIGHT        ← Comment this out
// #define SENSOR_CONFIG_MPU6050          ← This is commented
```

**AFTER:**
```cpp
// #define SENSOR_CONFIG_TEMP_WEIGHT      ← Now commented
#define SENSOR_CONFIG_MPU6050              ← Now active!
```

### **Step 4: That's It! 🎉**

The system automatically adapts:

✅ **CSV Header** changes to: `Timestamp,AccelX(g),AccelY(g),AccelZ(g),GyroX(°/s),GyroY(°/s),GyroZ(°/s),Status`

✅ **Firebase Fields** update to: `accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z`

✅ **Data Structure** adapts to 6 sensors (instead of 2)

✅ **Pin Definitions** change automatically

---

## 🎯 **Available Sensor Configurations**

### **1. Temperature + Weight** (Default)
```cpp
#define SENSOR_CONFIG_TEMP_WEIGHT
```
- **Sensors:** MAX31865 RTD + HX711 Load Cell
- **Fields:** `temperature`, `weight`
- **CSV:** `Timestamp,Temperature(°C),Weight(kg),Status`

### **2. MPU6050 (Motion Sensor)**
```cpp
#define SENSOR_CONFIG_MPU6050
```
- **Sensors:** MPU6050 (Accel + Gyro)
- **Fields:** `accel_x`, `accel_y`, `accel_z`, `gyro_x`, `gyro_y`, `gyro_z`
- **CSV:** `Timestamp,AccelX(g),AccelY(g),AccelZ(g),GyroX(°/s),GyroY(°/s),GyroZ(°/s),Status`

### **3. Temperature + Humidity**
```cpp
#define SENSOR_CONFIG_TEMP_HUMIDITY
```
- **Sensors:** DHT22
- **Fields:** `temperature`, `humidity`
- **CSV:** `Timestamp,Temperature(°C),Humidity(%),Status`

### **4. Distance + Light**
```cpp
#define SENSOR_CONFIG_DISTANCE_LIGHT
```
- **Sensors:** HC-SR04 + LDR
- **Fields:** `distance`, `light`
- **CSV:** `Timestamp,Distance(cm),Light(lux),Status`

### **5. Custom Sensors**
```cpp
#define SENSOR_CONFIG_CUSTOM
```
Define your own sensors in `SensorConfig.h`:
```cpp
#define SENSOR_COUNT 3  // Number of sensors

#define SENSOR1_NAME "pressure"
#define SENSOR1_UNIT "Pa"
#define SENSOR1_MIN 0.0
#define SENSOR1_MAX 110000.0

#define SENSOR2_NAME "altitude"
#define SENSOR2_UNIT "m"
// ... etc
```

---

## 🔧 **What Changes Automatically?**

When you switch sensor configs:

| Component | What Changes |
|-----------|--------------|
| **SensorData** | Field names, data types, array size |
| **CSV Export** | Column headers, value count |
| **Firebase** | Field names in database |
| **Local Storage** | Record structure |
| **ESP8266** | Upload field names |
| **Pin Assignments** | Hardware pin definitions |

---

## 💡 **Example: Switch from Temp to MPU6050**

### **Before (Temperature + Weight):**
```
CSV: 12345,25.50,100.25,1
Firebase: {
  "timestamp": 12345,
  "temperature": 25.50,
  "weight": 100.25,
  "status": 1
}
```

### **After (MPU6050):**
```
CSV: 12345,1.2,0.5,-9.8,10.5,-5.2,3.1,1
Firebase: {
  "timestamp": 12345,
  "accel_x": 1.2,
  "accel_y": 0.5,
  "accel_z": -9.8,
  "gyro_x": 10.5,
  "gyro_y": -5.2,
  "gyro_z": 3.1,
  "status": 1
}
```

---

## ⚠️ **Important Notes**

1. **One Config at a Time**: Only one `SENSOR_CONFIG_*` should be uncommented
2. **Rebuild Required**: After changing config, rebuild and upload:
   - ATmega2560: `pio run -e megaatmega2560 -t upload`
   - ESP8266: `pio run -e esp8266 -t upload`
3. **Data Compatibility**: Old data stored with different sensor configs won't parse correctly
4. **Clear Local Storage**: After changing sensors, run command `c` to clear EEPROM

---

## 🚀 **Adding New Sensors**

Want to add a completely new sensor type? Edit `SensorConfig.h`:

```cpp
// Add after existing configs
#elif defined(SENSOR_CONFIG_MY_SENSOR)
    #define SENSOR_COUNT 2
    #define SENSOR1_NAME "my_sensor1"
    #define SENSOR1_UNIT "units"
    #define SENSOR1_TYPE float
    #define SENSOR1_MIN 0.0
    #define SENSOR1_MAX 100.0

    // ... define more sensors

    #define CSV_HEADER "Timestamp,MySensor1,MySensor2,Status"
    #define FIREBASE_FIELDS "my_sensor1,my_sensor2"
#endif
```

Then uncomment:
```cpp
// #define SENSOR_CONFIG_MY_SENSOR  ← Remove the //
```

---

## ✅ **Summary**

**To change sensors:**
1. Edit `include/SensorConfig.h`
2. Comment out current config
3. Uncomment new config
4. Rebuild & upload

**That's it!** The entire system adapts automatically! 🎯
