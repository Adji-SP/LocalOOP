#ifndef CONFIG_H
#define CONFIG_H

/**
 * @file Config.h
 * @brief System configuration constants and hardware pin definitions
 *
 * This file contains all configurable parameters for the DNA data logging system.
 * Modify these values to match your hardware setup and network configuration.
 */

// WiFi Configuration
// Replace with your actual WiFi network credentials
#define WIFI_SSID "YOUR_WIFI_SSID"        // WiFi network name
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD" // WiFi network password

// Firebase Configuration
// Replace with your Firebase project details
#define FIREBASE_HOST "your-project.firebaseio.com" // Firebase Realtime Database URL
#define FIREBASE_AUTH "YOUR_DATABASE_SECRET"        // Firebase database secret key

// EEPROM Configuration
// Local storage parameters for Arduino MEGA2560 internal EEPROM
#define EEPROM_SIZE 4096        // Total EEPROM size available (4KB on MEGA2560)
#define MAX_RECORDS 100         // Maximum number of sensor records to store locally
#define RECORD_SIZE 32          // Size in bytes for each sensor data record

// Sensor Configuration
// Hardware pin assignments for connected sensors
#define RTD_CS_PIN 10           // Chip Select pin for MAX31865 RTD sensor
#define LOADCELL_DOUT_PIN 3     // Data output pin for HX711 load cell amplifier
#define LOADCELL_SCK_PIN 2      // Serial clock pin for HX711 load cell amplifier

// Data Collection Timing (milliseconds)
#define SAMPLE_INTERVAL 5000           // How often to read sensors (5 seconds)
#define FIREBASE_SYNC_INTERVAL 60000   // How often to sync batched data to Firebase (1 minute)

// Serial Communication
// ESP8266 WiFi module communication settings
#define ESP8266_SERIAL Serial1      // Hardware serial port for ESP8266 (pins 18/19 on MEGA)
#define ESP8266_BAUDRATE 115200     // Communication speed with ESP8266 module

#endif