#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

/**
 * @file DataStorage.h
 * @brief Abstract base class for data storage implementations
 *
 * This file defines the interface that all storage classes must implement.
 * It provides a common API for both local (EEPROM) and remote (Firebase) storage,
 * allowing the main application to work with different storage backends seamlessly.
 */

#include "Config.h"
#include "SensorData.h"

/**
 * @class DataStorage
 * @brief Abstract base class for sensor data storage systems
 *
 * This class defines the interface that all storage implementations must follow.
 * It provides common functionality like error tracking and status reporting,
 * while leaving the actual storage implementation to derived classes.
 *
 * Derived classes must implement:
 * - initialize(): Set up the storage system
 * - saveData(): Store a sensor reading
 * - retrieveData(): Read back stored data
 * - getRecordCount(): Report how many records are stored
 * - clearStorage(): Remove all stored data
 * - getStorageType(): Return a string identifying the storage type
 */
class DataStorage {
public:
    /**
     * @brief Constructor initializes common state variables
     */
    DataStorage() : isInitialized(false), errorCount(0) {}

    /**
     * @brief Virtual destructor for proper cleanup of derived classes
     */
    virtual ~DataStorage() {}

    // Pure virtual methods - must be implemented by derived classes

    /**
     * @brief Initialize the storage system
     * @return true if initialization successful, false on error
     */
    virtual bool initialize() = 0;

    /**
     * @brief Save sensor data to storage
     * @param data Reference to SensorData structure to save
     * @return true if save successful, false on error
     */
    virtual bool saveData(const SensorData& data) = 0;

    /**
     * @brief Retrieve stored sensor data by index
     * @param data Reference to SensorData structure to fill
     * @param index Zero-based index of record to retrieve
     * @return true if retrieval successful, false on error
     */
    virtual bool retrieveData(SensorData& data, int index) = 0;

    /**
     * @brief Get the number of records currently stored
     * @return Number of stored records, or -1 on error
     */
    virtual int getRecordCount() = 0;

    /**
     * @brief Clear all stored data
     * @return true if clear successful, false on error
     */
    virtual bool clearStorage() = 0;

    /**
     * @brief Get a string identifying this storage type
     * @return String name of storage type (e.g., "LocalStorage", "FirebaseStorage")
     */
    virtual String getStorageType() = 0;

    // Common methods available to all storage implementations

    /**
     * @brief Check if storage system is ready for use
     * @return true if initialized and ready, false otherwise
     */
    bool isReady() const { return isInitialized; }

    /**
     * @brief Get the current error count
     * @return Number of errors that have occurred
     */
    int getErrorCount() const { return errorCount; }

    /**
     * @brief Reset the error counter to zero
     */
    void resetErrorCount() { errorCount = 0; }

protected:
    bool isInitialized;    // Flag indicating successful initialization
    int errorCount;        // Counter for tracking storage errors

    /**
     * @brief Handle storage errors with consistent logging
     * @param errorMsg Descriptive error message to log
     *
     * This method increments the error counter and outputs a formatted
     * error message to the serial console for debugging purposes.
     */
    void handleError(const String& errorMsg) {
        errorCount++;
        Serial.print(F("[ERROR] "));
        Serial.print(getStorageType());
        Serial.print(F(": "));
        Serial.println(errorMsg);
    }
};

#endif