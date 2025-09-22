#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

/**
 * @file SensorData.h
 * @brief Data structure for sensor readings and serialization methods
 *
 * This file defines the core data structure used throughout the system
 * for storing, transmitting, and persisting sensor measurements.
 */

#include "Config.h"
#include <Arduino.h>

/**
 * @struct SensorData
 * @brief Container for all sensor measurements and metadata
 *
 * This structure holds a complete sensor reading including:
 * - Temperature from RTD sensor (°C)
 * - Weight from load cell (kg)
 * - Timestamp when measurement was taken (milliseconds since boot)
 * - Status code indicating measurement validity
 */
struct SensorData {
    float temperature;          // Temperature reading in Celsius
    float weight;              // Weight reading in kilograms
    unsigned long timestamp;    // Time of measurement (millis() value)
    uint8_t status;            // Status: 1=OK, 0=Error

    /**
     * @brief Convert sensor data to CSV format for local storage
     * @return String containing comma-separated values
     *
     * Format: timestamp,temperature,weight,status
     * Example: "12345,25.50,100.25,1"
     */
    String toCSV() const {
        return String(timestamp) + "," +
               String(temperature, 2) + "," +
               String(weight, 2) + "," +
               String(status);
    }

    /**
     * @brief Parse CSV string back into SensorData structure
     * @param csv Comma-separated string to parse
     * @return true if parsing successful, false on error
     *
     * Validates CSV format and converts string values back to appropriate types.
     * Returns false if the CSV format is invalid or missing required fields.
     */
    bool fromCSV(const String& csv) {
        int firstComma = csv.indexOf(',');
        int secondComma = csv.indexOf(',', firstComma + 1);
        int thirdComma = csv.indexOf(',', secondComma + 1);

        if (firstComma == -1 || secondComma == -1 || thirdComma == -1) {
            return false;
        }

        timestamp = strtoul(csv.substring(0, firstComma).c_str(), NULL, 10);
        temperature = csv.substring(firstComma + 1, secondComma).toFloat();
        weight = csv.substring(secondComma + 1, thirdComma).toFloat();
        status = csv.substring(thirdComma + 1).toInt();

        return true;
    }

    /**
     * @brief Convert sensor data to JSON format for Firebase transmission
     * @return String containing JSON object
     *
     * Creates a JSON object suitable for Firebase Realtime Database.
     * Format: {"timestamp":12345,"temperature":25.50,"weight":100.25,"status":1}
     */
    String toJSON() const {
        return "{\"timestamp\":" + String(timestamp) +
               ",\"temperature\":" + String(temperature, 2) +
               ",\"weight\":" + String(weight, 2) +
               ",\"status\":" + String(status) + "}";
    }
};

#endif