#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

/**
 * @file SensorData.h
 * @brief Dynamic data structure for sensor readings
 *
 * This file defines a flexible data structure that adapts to different
 * sensor configurations defined in SensorConfig.h
 */

#include "SystemConfig.h"
#include <Arduino.h>

/**
 * @struct SensorData
 * @brief Dynamic container for sensor measurements
 *
 * This structure adapts based on SENSOR_CONFIG selection:
 * - Stores up to 6 sensor values (flexible)
 * - Timestamp when measurement was taken
 * - Status code indicating measurement validity
 */
struct SensorData {
    float values[6];            // Sensor values (up to 6 sensors)
    unsigned long timestamp;    // Time of measurement (Unix timestamp or millis)
    uint8_t status;            // Status: 1=OK, 0=Error
    uint8_t relay1;            // Relay 1 state (SSR): 0=OFF, 1=ON
    uint8_t relay2;            // Relay 2 state: 0=OFF, 1=ON
    float kadarAir;            // Moisture content (%) - not stored in EEPROM, only sent to Firebase

    // Accessor methods for clearer code
    void setTemperature(float temp) { values[0] = temp; }
    void setWeight(float wt) { values[1] = wt; }
    void setKadarAir(float ka) { kadarAir = ka; }
    float getTemperature() const { return values[0]; }
    float getWeight() const { return values[1]; }
    float getKadarAir() const { return kadarAir; }

    // Generic accessor
    void setValue(int index, float value) {
        if (index >= 0 && index < SENSOR_COUNT) values[index] = value;
    }
    float getValue(int index) const {
        return (index >= 0 && index < SENSOR_COUNT) ? values[index] : 0.0;
    }

    // Legacy reference accessors (for backward compatibility)
    float& temperature() { return values[0]; }
    float& weight() { return values[1]; }
    const float& temperature() const { return values[0]; }
    const float& weight() const { return values[1]; }

    /**
     * @brief Convert sensor data to CSV format (dynamic based on sensor config)
     * @return String containing comma-separated values
     *
     * Format adapts to SENSOR_COUNT from SensorConfig.h
     * Example: "12345,25.50,100.25,1" for 2 sensors
     */
    String toCSV() const {
        String csv = String(timestamp);
        for (int i = 0; i < SENSOR_COUNT; i++) {
            csv += "," + String(values[i], 2);
        }
        csv += "," + String(status);
        return csv;
    }

    /**
     * @brief Parse CSV string back into SensorData structure (dynamic)
     * @param csv Comma-separated string to parse
     * @return true if parsing successful, false on error
     *
     * Automatically adapts to sensor count from SensorConfig.h
     */
    bool fromCSV(const String& csv) {
        int pos = 0;
        int nextComma = csv.indexOf(',');

        if (nextComma == -1) return false;

        // Parse timestamp
        timestamp = strtoul(csv.substring(0, nextComma).c_str(), NULL, 10);
        pos = nextComma + 1;

        // Parse sensor values dynamically
        for (int i = 0; i < SENSOR_COUNT; i++) {
            nextComma = csv.indexOf(',', pos);
            if (nextComma == -1 && i < SENSOR_COUNT - 1) return false;

            if (i == SENSOR_COUNT - 1) {
                // Last sensor value
                int statusComma = csv.indexOf(',', pos);
                if (statusComma == -1) return false;
                values[i] = csv.substring(pos, statusComma).toFloat();
                pos = statusComma + 1;
            } else {
                values[i] = csv.substring(pos, nextComma).toFloat();
                pos = nextComma + 1;
            }
        }

        // Parse status
        status = csv.substring(pos).toInt();

        return true;
    }

    /**
     * @brief Convert sensor data to JSON format for Firebase (dynamic)
     * @return String containing JSON object
     *
     * Creates a JSON object that adapts to sensor configuration.
     * Example: {"timestamp":12345,"accel_x":1.2,"accel_y":0.5,...}
     */
    String toJSON() const {
        String json = "{\"timestamp\":" + String(timestamp);

        // Add sensor fields dynamically based on configuration
        #ifdef SENSOR_CONFIG_TEMP_WEIGHT
            json += ",\"temperature\":" + String(values[0], 2);
            json += ",\"weight\":" + String(values[1], 2);
        #elif defined(SENSOR_CONFIG_MPU6050)
            json += ",\"accel_x\":" + String(values[0], 2);
            json += ",\"accel_y\":" + String(values[1], 2);
            json += ",\"accel_z\":" + String(values[2], 2);
            json += ",\"gyro_x\":" + String(values[3], 2);
            json += ",\"gyro_y\":" + String(values[4], 2);
            json += ",\"gyro_z\":" + String(values[5], 2);
        #elif defined(SENSOR_CONFIG_TEMP_HUMIDITY)
            json += ",\"temperature\":" + String(values[0], 2);
            json += ",\"humidity\":" + String(values[1], 2);
        #elif defined(SENSOR_CONFIG_DISTANCE_LIGHT)
            json += ",\"distance\":" + String(values[0], 2);
            json += ",\"light\":" + String(values[1], 0);
        #else
            // Generic fallback
            for (int i = 0; i < SENSOR_COUNT; i++) {
                json += ",\"value" + String(i) + "\":" + String(values[i], 2);
            }
        #endif

        json += ",\"status\":" + String(status) + "}";
        return json;
    }
};

#endif