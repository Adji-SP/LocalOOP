#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

/**
 * @file SystemConfig.h
 * @brief MASTER CONFIGURATION FILE - ALL SETTINGS IN ONE PLACE
 *
 * This is the ONLY config file you need to edit!
 * Everything is here:
 * - WiFi & Firebase credentials
 * - Sensor type selection
 * - Hardware pins
 * - Data collection settings
 * - Storage settings
 */

// ========================================
// SECTION 1: CREDENTIALS & NETWORK
// ========================================

// WiFi Settings
#define WIFI_SSID "bismillah"
#define WIFI_PASSWORD "akudeweyolali"

// Firebase Settings
#define API_KEY "AIzaSyAbU6hfeRRIUNVL3fmAOa8AtrQVpTcnuos"
#define DATABASE_URL "https://masgilang-304d2-default-rtdb.firebaseio.com"
#define FIREBASE_HOST "masgilang-304d2-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH ""

// Device Identity
#define DEVICE_NAME "MEGA_DNA_LOGGER"
#define DEVICE_LOCATION "Lab_A"
#define DEVICE_VERSION "1.0.0"

// ========================================
// SECTION 2: SENSOR CONFIGURATION
// ========================================

// 🎯 CHOOSE YOUR SENSOR TYPE (Uncomment ONE option):

#define SENSOR_CONFIG_TEMP_WEIGHT // ✓ Temperature + Weight (default)
// #define SENSOR_CONFIG_MPU6050          // Accelerometer + Gyroscope (6-axis)
// #define SENSOR_CONFIG_TEMP_HUMIDITY    // Temperature + Humidity (DHT22)
// #define SENSOR_CONFIG_DISTANCE_LIGHT   // Distance + Light sensor
// #define SENSOR_CONFIG_CUSTOM           // Define your own sensors

// --- Sensor Definitions (Auto-configured based on selection above) ---

#ifdef SENSOR_CONFIG_TEMP_WEIGHT
#define SENSOR_COUNT 2
#define SENSOR1_NAME "temperature"
#define SENSOR1_UNIT "°C"
#define SENSOR1_TYPE float
#define SENSOR1_MIN -50.0
#define SENSOR1_MAX 150.0
#define SENSOR2_NAME "weight"
#define SENSOR2_UNIT "kg"
#define SENSOR2_TYPE float
#define SENSOR2_MIN 0.0
#define SENSOR2_MAX 1000.0
#define CSV_HEADER "Timestamp,Temperature(°C),Weight(kg),Status"

#elif defined(SENSOR_CONFIG_MPU6050)
#define SENSOR_COUNT 6
#define SENSOR1_NAME "accel_x"
#define SENSOR1_UNIT "g"
#define SENSOR2_NAME "accel_y"
#define SENSOR2_UNIT "g"
#define SENSOR3_NAME "accel_z"
#define SENSOR3_UNIT "g"
#define SENSOR4_NAME "gyro_x"
#define SENSOR4_UNIT "°/s"
#define SENSOR5_NAME "gyro_y"
#define SENSOR5_UNIT "°/s"
#define SENSOR6_NAME "gyro_z"
#define SENSOR6_UNIT "°/s"
#define CSV_HEADER "Timestamp,AccelX(g),AccelY(g),AccelZ(g),GyroX(°/s),GyroY(°/s),GyroZ(°/s),Status"

#elif defined(SENSOR_CONFIG_TEMP_HUMIDITY)
#define SENSOR_COUNT 2
#define SENSOR1_NAME "temperature"
#define SENSOR1_UNIT "°C"
#define SENSOR2_NAME "humidity"
#define SENSOR2_UNIT "%"
#define CSV_HEADER "Timestamp,Temperature(°C),Humidity(%),Status"

#elif defined(SENSOR_CONFIG_DISTANCE_LIGHT)
#define SENSOR_COUNT 2
#define SENSOR1_NAME "distance"
#define SENSOR1_UNIT "cm"
#define SENSOR2_NAME "light"
#define SENSOR2_UNIT "lux"
#define CSV_HEADER "Timestamp,Distance(cm),Light(lux),Status"

#elif defined(SENSOR_CONFIG_CUSTOM)
// 🔧 CUSTOM SENSORS - Define your own here:
#define SENSOR_COUNT 2
#define SENSOR1_NAME "sensor1"
#define SENSOR1_UNIT "unit1"
#define SENSOR2_NAME "sensor2"
#define SENSOR2_UNIT "unit2"
#define CSV_HEADER "Timestamp,Sensor1,Sensor2,Status"
#else
#error "No sensor selected! Uncomment one SENSOR_CONFIG_* option above"
#endif

// ========================================
// SECTION 3: HARDWARE PINS
// ========================================

#ifdef SENSOR_CONFIG_TEMP_WEIGHT
#define RTD_CS_PIN 10       // MAX31865 Chip Select
#define LOADCELL_DOUT_PIN 3 // HX711 Data Out
#define LOADCELL_SCK_PIN 2  // HX711 Clock
#define RREF 430.0          // 430Ω untuk PT100
#define RNOMINAL 100.0      // 100Ω untuk PT100
#define VBIAS 1.03
#define HX711_DT 25  // Pin data HX711
#define HX711_SCK 23 // Pin clock HX711
#define RELAY_PIN1 3  // Relay dikontrol melalui pin digital 3
#define RELAY_PIN2 7  // Relay dikontrol melalui pin digital 3
#endif

#ifdef SENSOR_CONFIG_MPU6050
#define MPU6050_I2C_ADDRESS 0x68
#define MPU6050_SCL_PIN 21
#define MPU6050_SDA_PIN 20
#endif

#ifdef SENSOR_CONFIG_TEMP_HUMIDITY
#define DHT_PIN 7
#define DHT_TYPE DHT22
#endif

#ifdef SENSOR_CONFIG_DISTANCE_LIGHT
#define ULTRASONIC_TRIG_PIN 8
#define ULTRASONIC_ECHO_PIN 9
#define LDR_ANALOG_PIN A0
#endif

// ========================================
// SECTION 4: DATA COLLECTION SETTINGS
// ========================================

#define DATA_SAMPLE_INTERVAL 1000     // Read sensors every 5 seconds
#define AUTO_SYNC_INTERVAL 300000     // Sync to Firebase every 5 minutes
#define DISPLAY_STATUS_INTERVAL 30000 // Show status every 30 seconds

// ========================================
// SECTION 5: LOCAL STORAGE (EEPROM)
// ========================================

#define EEPROM_SIZE 4096           // ATmega2560 has 4KB EEPROM
#define MAX_LOCAL_RECORDS 125      // Max records to store
#define RECORD_SIZE_BYTES 32       // Size per record
#define AUTO_CLEAR_AFTER_SYNC true // Clear local storage after successful upload

// ========================================
// SECTION 6: ESP8266 COMMUNICATION
// ========================================

#define ESP_SERIAL_BAUD 115200        // Serial baud rate for ESP8266
#define ESP_WIFI_CHECK_INTERVAL 30000 // Check WiFi every 30 seconds
#define ESP_RECONNECT_ATTEMPTS 20     // Max WiFi reconnect attempts
#define ESP_RESPONSE_TIMEOUT 1000     // Response timeout (ms)

// ========================================
// SECTION 7: FIREBASE PATHS
// ========================================

#define FB_ROOT_PATH "/devices"
#define FB_DEVICE_PATH FB_ROOT_PATH "/" DEVICE_NAME
#define FB_DATA_PATH FB_DEVICE_PATH "/data"
#define FB_STATUS_PATH FB_DEVICE_PATH "/status"
#define FB_CONFIG_PATH FB_DEVICE_PATH "/config"

// ========================================
// SECTION 8: SERIAL & DEBUG
// ========================================

#define SERIAL_MONITOR_BAUD 115200
#define DEBUG_MODE true
#define VERBOSE_LOGGING false

// ========================================
// SECTION 9: STATUS CODES
// ========================================

#define STATUS_OK 1
#define STATUS_ERROR 0
#define STATUS_SENSOR_FAULT -1
#define STATUS_WIFI_OFFLINE -2

// ========================================
// LEGACY COMPATIBILITY (DO NOT EDIT)
// ========================================

// For backward compatibility with old code
#define MAX_RECORDS MAX_LOCAL_RECORDS
#define RECORD_SIZE RECORD_SIZE_BYTES
#define SAMPLE_INTERVAL DATA_SAMPLE_INTERVAL
#define FIREBASE_SYNC_INTERVAL AUTO_SYNC_INTERVAL
#define ESP8266_SERIAL Serial3
#define ESP8266_BAUDRATE ESP_SERIAL_BAUD

#endif