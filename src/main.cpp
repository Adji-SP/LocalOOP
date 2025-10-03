#include <Arduino.h>
#include "Config.h"
#include "SensorData.h"
#include "LocalStorage.h"
// #include "FirebaseStorage.h"  // Firebase not supported on ATmega2560

// Storage instances
LocalStorage* localStorage;
// FirebaseStorage* firebaseStorage;  // Firebase not supported on ATmega2560

// Sensor simulation variables
float simulatedTemp = 25.0;
float simulatedWeight = 100.0;
unsigned long lastSampleTime = 0;
unsigned long lastDisplayTime = 0;

// Generate simulated sensor data
SensorData generateSimulatedData() {
    SensorData data;

    // Simulate RTD temperature sensor
    simulatedTemp += random(-100, 100) / 100.0;
    simulatedTemp = constrain(simulatedTemp, -50.0, 150.0);

    // Simulate load cell weight
    simulatedWeight += random(-500, 500) / 100.0;
    simulatedWeight = constrain(simulatedWeight, 0.0, 1000.0);

    data.temperature = simulatedTemp;
    data.weight = simulatedWeight;
    data.timestamp = millis();
    data.status = 1; // 1 = OK, 0 = Error

    return data;
}

void displayStatus() {
    Serial.println(F("\n========== System Status =========="));
    Serial.print(F("Uptime: "));
    Serial.print(millis() / 1000);
    Serial.println(F(" seconds"));

    Serial.print(F("Local Storage: "));
    if (localStorage && localStorage->isReady()) {
        Serial.print(F("OK ("));
        Serial.print(localStorage->getRecordCount());
        Serial.print(F("/"));
        Serial.print(MAX_RECORDS);
        Serial.println(F(" records)"));
    } else {
        Serial.println(F("ERROR"));
    }

    Serial.print(F("Firebase Storage: "));
    Serial.println(F("NOT AVAILABLE (ATmega2560)"));

    Serial.print(F("Last Temperature: "));
    Serial.print(simulatedTemp, 2);
    Serial.println(F(" °C"));

    Serial.print(F("Last Weight: "));
    Serial.print(simulatedWeight, 2);
    Serial.println(F(" kg"));

    Serial.println(F("===================================\n"));
}

void handleSerialCommands() {
    if (Serial.available()) {
        char cmd = Serial.read();

        switch (cmd) {
            case 'r':  // Read local storage
                {
                    Serial.println(F("\nLocal Storage Records:"));
                    SensorData data;
                    for (int i = 0; i < localStorage->getRecordCount(); i++) {
                        if (localStorage->retrieveData(data, i)) {
                            Serial.print(i);
                            Serial.print(F(": "));
                            Serial.println(data.toCSV());
                        }
                    }
                }
                break;

            case 'c':  // Clear local storage
                localStorage->clearStorage();
                Serial.println(F("Local storage cleared"));
                break;

            case 's':  // Force sync to Firebase
                Serial.println(F("Firebase sync not available on ATmega2560"));
                break;

            case 'e':  // Export to CSV
                {
                    String csvExport;
                    if (localStorage->exportToCSV(csvExport)) {
                        Serial.println(F("\nCSV Export:"));
                        Serial.println(csvExport);
                    }
                }
                break;

            case 'h':  // Help
                Serial.println(F("\nAvailable Commands:"));
                Serial.println(F("r - Read local storage records"));
                Serial.println(F("c - Clear local storage"));
                Serial.println(F("s - Force sync to Firebase"));
                Serial.println(F("e - Export to CSV"));
                Serial.println(F("h - Show this help"));
                break;
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println(F("\n================================"));
    Serial.println(F("Arduino MEGA2560 Data Logger"));
    Serial.println(F("Version 1.0.0"));
    Serial.println(F("================================\n"));

    // Generate unique device ID from chip serial
    randomSeed(analogRead(0));
    String deviceId = "MEGA_" + String(random(10000, 99999));

    // Initialize storage systems
    localStorage = new LocalStorage(MAX_RECORDS, RECORD_SIZE);

    // Initialize local storage
    if (!localStorage->initialize()) {
        Serial.println(F("WARNING: Local storage initialization failed"));
    }

    Serial.println(F("INFO: Firebase not available on ATmega2560 - using local storage only"));

    Serial.println(F("\nSystem ready. Type 'h' for help.\n"));
}

void loop() {
    unsigned long currentTime = millis();

    // Sample sensors at defined interval
    if (currentTime - lastSampleTime >= SAMPLE_INTERVAL) {
        lastSampleTime = currentTime;

        // Generate simulated sensor data
        SensorData data = generateSimulatedData();

        Serial.print(F("New data: "));
        Serial.println(data.toCSV());

        // Save to local storage
        if (!localStorage->saveData(data)) {
            Serial.println(F("ERROR: Failed to save to local storage"));
        }
    }

    // Display status every 30 seconds
    if (currentTime - lastDisplayTime >= 30000) {
        lastDisplayTime = currentTime;
        displayStatus();
    }

    // Handle serial commands
    handleSerialCommands();

    // Small delay to prevent overwhelming the system
    delay(10);
}