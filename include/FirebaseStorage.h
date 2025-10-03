/**
 * @file FirebaseStorage.h
 * @brief Firebase cloud storage implementation with WiFi connectivity
 *
 * This file implements cloud-based storage using Firebase Realtime Database.
 * It manages WiFi connectivity via ESP8266 module and provides automatic
 * batching and retry mechanisms for reliable data transmission.
 */

#ifndef FIREBASE_STORAGE_H
#define FIREBASE_STORAGE_H

#include "Config.h"
#include "DataStorage.h"
#include <ArduinoJson.h>  // Keep this - it works fine on MEGA2560

/**
 * @class FirebaseStorage
 * @brief Firebase cloud storage implementation with ESP8266 WiFi connectivity
 *
 * This class provides cloud-based storage using Firebase Realtime Database.
 * It manages WiFi connectivity through an ESP8266 module connected via Serial1
 * and implements automatic batching to optimize network usage and reliability.
 *
 * Features:
 * - Automatic WiFi connection management
 * - Batch uploading to reduce network overhead
 * - Automatic retry on connection failures
 * - AT command interface for ESP8266 communication
 */
class FirebaseStorage : public DataStorage {
private:
    String firebaseHost;       // Firebase project URL
    String firebaseAuth;       // Firebase authentication token
    String deviceId;           // Unique identifier for this device
    bool wifiConnected;        // Current WiFi connection status
    unsigned long lastSyncTime; // Last time data was synced to Firebase

    // Buffer for batch uploads to optimize network usage
    static const int BATCH_SIZE = 10;  // Number of records to batch together
    SensorData batchBuffer[BATCH_SIZE]; // Array to hold batched sensor data
    int batchCount;                     // Current number of records in batch

    // ESP8266 communication methods

    /**
     * @brief Send AT command to ESP8266 and wait for expected response
     * @param command AT command string to send
     * @param expectedResponse Expected response from ESP8266
     * @param timeout Maximum time to wait for response (milliseconds)
     * @return true if expected response received, false on timeout/error
     */
    bool sendATCommand(const String& command, const String& expectedResponse, unsigned long timeout = 5000);

    /**
     * @brief Connect to WiFi network using ESP8266
     * @param ssid WiFi network name
     * @param password WiFi network password
     * @return true if connection successful, false on error
     */
    bool connectWiFi(const String& ssid, const String& password);

    /**
     * @brief Send JSON data to Firebase via HTTP POST
     * @param path Firebase database path for the data
     * @param jsonData JSON string containing sensor data
     * @return true if upload successful, false on error
     */
    bool sendToFirebase(const String& path, const String& jsonData);

public:
    /**
     * @brief Constructor with Firebase configuration
     * @param host Firebase Realtime Database URL
     * @param auth Firebase authentication token/secret
     * @param devId Unique device identifier for data organization
     */
    FirebaseStorage(const String& host, const String& auth, const String& devId);

    // Override virtual methods from DataStorage base class

    /**
     * @brief Initialize Firebase storage and WiFi connection
     * @return true if initialization successful, false on error
     */
    bool initialize() override;

    /**
     * @brief Add sensor data to batch buffer for upload
     * @param data SensorData structure to queue for upload
     * @return true if queued successfully, false on error
     *
     * Note: Data is batched locally and uploaded when batch is full
     * or when syncBatch() is called manually.
     */
    bool saveData(const SensorData& data) override;

    /**
     * @brief Retrieve data from batch buffer by index
     * @param data SensorData structure to fill
     * @param index Index within current batch buffer
     * @return true if retrieval successful, false on error
     *
     * Note: This only accesses the local batch buffer, not Firebase data.
     */
    bool retrieveData(SensorData& data, int index) override;

    /**
     * @brief Get number of records in current batch buffer
     * @return Number of records waiting to be uploaded
     */
    int getRecordCount() override;

    /**
     * @brief Clear the local batch buffer
     * @return true if clear successful, false on error
     *
     * Note: This does not delete data from Firebase, only clears local buffer.
     */
    bool clearStorage() override;

    /**
     * @brief Get storage type identifier
     * @return String "FirebaseStorage"
     */
    String getStorageType() override { return "FirebaseStorage"; }

    // Firebase-specific methods

    /**
     * @brief Upload all batched data to Firebase
     * @return true if sync successful, false on error
     *
     * Sends all data in the batch buffer to Firebase and clears the buffer
     * upon successful transmission.
     */
    bool syncBatch();

    /**
     * @brief Check current WiFi connection status
     * @return true if connected to WiFi, false otherwise
     */
    bool isConnected() const { return wifiConnected; }

    /**
     * @brief Update the device identifier
     * @param id New device ID string
     */
    void setDeviceId(const String& id) { deviceId = id; }

    /**
     * @brief Test WiFi and Firebase connectivity
     * @return true if connection is working, false on error
     */
    bool checkConnection();
};

#endif