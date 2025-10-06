#ifndef LOCAL_STORAGE_H
#define LOCAL_STORAGE_H

/**
 * @file LocalStorage.h
 * @brief EEPROM-based local storage implementation
 *
 * This file implements persistent local storage using the Arduino MEGA2560's
 * internal EEPROM. It provides circular buffer functionality to store sensor
 * readings locally when cloud connectivity is unavailable.
 */

#include "Config.h"
#include "DataStorage.h"
#include <EEPROM.h>

/**
 * @class LocalStorage
 * @brief EEPROM-based implementation of DataStorage interface
 *
 * This class provides persistent local storage using the Arduino MEGA2560's
 * internal EEPROM (4KB). It implements a circular buffer system that can
 * store up to MAX_RECORDS sensor readings with automatic overflow handling.
 *
 * EEPROM Layout:
 * - Bytes 0-7: Header (magic number, version, record count, current index)
 * - Bytes 8+: Sensor data records (32 bytes each)
 */
class LocalStorage : public DataStorage {
private:
    static const int HEADER_SIZE = 10;     // Header: magic(2) + version(1) + index(2) + count(2) + checksum(1) + reserved(2)
    static const int RECORD_START = HEADER_SIZE;  // Start address for data records
    static const uint8_t STORAGE_VERSION = 1;     // Version for compatibility checks
    int maxRecords;        // Maximum number of records that can be stored
    int recordSize;        // Size of each record in bytes (includes 2-byte length prefix)
    int currentIndex;      // Current write position (circular buffer)
    int recordCount;       // Number of records currently stored

    // EEPROM structure management methods

    /**
     * @brief Write header information to EEPROM
     * Stores metadata like record count and current index at the beginning of EEPROM
     */
    void writeHeader();

    /**
     * @brief Read header information from EEPROM
     * @return true if valid header found, false if EEPROM needs initialization
     */
    bool readHeader();

    /**
     * @brief Calculate EEPROM address for a given record index
     * @param index Record index (0-based)
     * @return EEPROM byte address for the record
     */
    int calculateAddress(int index);

public:
    /**
     * @brief Constructor with configurable storage parameters
     * @param maxRec Maximum number of records to store (default: MAX_RECORDS)
     * @param recSize Size of each record in bytes (default: RECORD_SIZE)
     */
    LocalStorage(int maxRec = MAX_RECORDS, int recSize = RECORD_SIZE);

    // Override virtual methods from DataStorage base class

    /**
     * @brief Initialize EEPROM storage system
     * @return true if initialization successful, false on error
     */
    bool initialize() override;

    /**
     * @brief Save sensor data to EEPROM
     * @param data SensorData structure to save
     * @return true if save successful, false on error
     */
    bool saveData(const SensorData& data) override;

    /**
     * @brief Retrieve sensor data from EEPROM by index
     * @param data SensorData structure to fill with retrieved data
     * @param index Zero-based index of record to retrieve
     * @return true if retrieval successful, false on error
     */
    bool retrieveData(SensorData& data, int index) override;

    /**
     * @brief Get the number of records currently stored
     * @return Number of stored records
     */
    int getRecordCount() override;

    /**
     * @brief Clear all stored data from EEPROM
     * @return true if clear successful, false on error
     */
    bool clearStorage() override;

    /**
     * @brief Get storage type identifier
     * @return String "LocalStorage"
     */
    String getStorageType() override { return "LocalStorage"; }

    // Additional methods specific to local storage

    /**
     * @brief Check if storage is at maximum capacity
     * @return true if storage is full, false otherwise
     */
    bool isFull() const;

    /**
     * @brief Get number of available record slots
     * @return Number of free storage slots
     */
    int getFreeSpace() const;

    /**
     * @brief Export stored data to CSV format
     * @param output String reference to fill with CSV data
     * @param startIndex Starting record index for export (default: 0)
     * @param count Number of records to export, -1 for all (default: -1)
     * @return true if export successful, false on error
     */
    bool exportToCSV(String& output, int startIndex = 0, int count = -1);
};

#endif