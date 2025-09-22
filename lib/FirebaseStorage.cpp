#include "../include/FirebaseStorage.h"
#include "../include/Config.h"

FirebaseStorage::FirebaseStorage(const String& host, const String& auth, const String& devId)
    : firebaseHost(host), firebaseAuth(auth), deviceId(devId),
      wifiConnected(false), lastSyncTime(0), batchCount(0) {
}

bool FirebaseStorage::initialize() {
    Serial.println(F("Initializing Firebase Storage..."));

    // Initialize ESP8266 communication
    ESP8266_SERIAL.begin(ESP8266_BAUDRATE);
    delay(1000);

    // Reset ESP8266
    if (!sendATCommand("AT+RST", "ready", 10000)) {
        handleError("ESP8266 reset failed");
        return false;
    }

    // Set mode to Station + AP
    if (!sendATCommand("AT+CWMODE=3", "OK", 5000)) {
        handleError("Failed to set WiFi mode");
        return false;
    }

    // Connect to WiFi
    if (!connectWiFi(WIFI_SSID, WIFI_PASSWORD)) {
        handleError("WiFi connection failed");
        return false;
    }

    wifiConnected = true;
    isInitialized = true;

    Serial.println(F("Firebase Storage initialized successfully"));
    return true;
}

bool FirebaseStorage::saveData(const SensorData& data) {
    if (!isInitialized) {
        handleError("Storage not initialized");
        return false;
    }

    // Add batch buffer
    if (batchCount < BATCH_SIZE) {
        batchBuffer[batchCount++] = data;
    }

    unsigned long currentTime = millis();
    if (currentTime - lastSyncTime >= FIREBASE_SYNC_INTERVAL || batchCount >= BATCH_SIZE) {
        return syncBatch();
    }

    return true;
}

bool FirebaseStorage::syncBatch() {
    if (batchCount == 0) {
        return true;
    }

    if (!checkConnection()) {
        handleError("No WiFi connection");
        return false;
    }

    Serial.println(F("Syncing batch to Firebase..."));

    // Create JSON array of batch data
    DynamicJsonDocument doc(2048);
    JsonArray array = doc.to<JsonArray>();

    for (int i = 0; i < batchCount; i++) {
        JsonObject obj = array.createNestedObject();
        obj["timestamp"] = batchBuffer[i].timestamp;
        obj["temperature"] = batchBuffer[i].temperature;
        obj["weight"] = batchBuffer[i].weight;
        obj["status"] = batchBuffer[i].status;
        obj["deviceId"] = deviceId;
    }

    String jsonData;
    serializeJson(doc, jsonData);

    // Send to Firebase
    String path = "/sensors/" + deviceId + "/data";

    if (sendToFirebase(path, jsonData)) {
        Serial.print(F("Synced "));
        Serial.print(batchCount);
        Serial.println(F(" records to Firebase"));

        batchCount = 0;
        lastSyncTime = millis();
        return true;
    } else {
        handleError("Failed to sync to Firebase");
        return false;
    }
}

bool FirebaseStorage::sendATCommand(const String& command, const String& expectedResponse, unsigned long timeout) {
    ESP8266_SERIAL.println(command);

    unsigned long startTime = millis();
    String response = "";

    while (millis() - startTime < timeout) {
        if (ESP8266_SERIAL.available()) {
            char c = ESP8266_SERIAL.read();
            response += c;

            if (response.indexOf(expectedResponse) != -1) {
                return true;
            }
        }
    }

    Serial.print(F("AT Command failed: "));
    Serial.println(command);
    Serial.print(F("Response: "));
    Serial.println(response);

    return false;
}

bool FirebaseStorage::connectWiFi(const String& ssid, const String& password) {
    String cmd = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"";
    return sendATCommand(cmd, "WIFI CONNECTED", 20000);
}

bool FirebaseStorage::sendToFirebase(const String& path, const String& jsonData) {
    // Establish TCP connection
    String cmd = "AT+CIPSTART=\"TCP\",\"" + firebaseHost + "\",80";
    if (!sendATCommand(cmd, "CONNECT", 10000)) {
        return false;
    }

    // Prepare HTTP request
    String httpRequest = "PUT " + path + "?auth=" + firebaseAuth + " HTTP/1.1\r\n";
    httpRequest += "Host: " + firebaseHost + "\r\n";
    httpRequest += "Content-Type: application/json\r\n";
    httpRequest += "Content-Length: " + String(jsonData.length()) + "\r\n\r\n";
    httpRequest += jsonData;

    // Send data
    cmd = "AT+CIPSEND=" + String(httpRequest.length());
    if (!sendATCommand(cmd, ">", 5000)) {
        return false;
    }

    ESP8266_SERIAL.print(httpRequest);

    // Wait for response
    return sendATCommand("", "SEND OK", 10000);
}

bool FirebaseStorage::checkConnection() {
    return sendATCommand("AT+CWJAP?", "+CWJAP:", 5000);
}

bool FirebaseStorage::retrieveData(SensorData& data, int index) {
    // Firebase is write-only in this implementation
    // For reading, you would need to implement HTTP GET
    handleError("Read operation not implemented for Firebase");
    return false;
}

int FirebaseStorage::getRecordCount() {
    // Return batch buffer count
    return batchCount;
}

bool FirebaseStorage::clearStorage() {
    batchCount = 0;
    return true;
}