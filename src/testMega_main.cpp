#include <Arduino.h>
#include "Config.h"
#include "SensorData.h"
#include "LocalStorage.h"
#include "DWIN.h"
// #include "FirebaseStorage.h"  // Firebase not supported on ATmega2560

// Storage instances
LocalStorage *localStorage;
// FirebaseStorage *firebaseStorage;
// LocalStorage* localStorage;
// FirebaseStorage* firebaseStorage;  // Firebase not supported on ATmega2560

// Sensor simulation variables
float simulatedTemp = 25.0;
float simulatedWeight = 100.0;
unsigned long lastSampleTime = 0;
unsigned long lastDisplayTime = 0;

// Generate simulated sensor data
SensorData generateSimulatedData()
{
    SensorData data;

    // Simulate RTD temperature sensor
    simulatedTemp += random(-100, 100) / 100.0;
    simulatedTemp = constrain(simulatedTemp, -50.0, 150.0);

    // Simulate load cell weight
    simulatedWeight += random(-500, 500) / 100.0;
    simulatedWeight = constrain(simulatedWeight, 0.0, 1000.0);

    // data.temperature = simulatedTemp;
    // data.weight = simulatedWeight;
    data.timestamp = millis();
    data.status = 1; // 1 = OK, 0 = Error

    return data;
}

// VP Address untuk 3 tombol
#define VP_POWER_SWITCH 5500
#define VP_BUTTON_2 6500
#define VP_BUTTON_3 7500

// VP address display di HMI
#define VP_TEMP_DISPLAY 5000
#define VP_WEIGHT_DISPLAY 5002
#define VP_HUMIDITY_DISPLAY 5004

DWIN hmi(19, 18, 115200); // RX, TX, baudrate (disesuaikan)

// Definisi pin relay
#define RELAY_1_PIN 7 // jgn lupa diganti
#define RELAY_2_PIN 8

// Status tombol
bool powerSwitchState = false;
bool button2State = false;
bool button3State = false;

void hmiCallback(String address, int lastByte, String message, String response)
{
    int vpAddress = address.toInt();
    int vpValue = message.toInt(); // 0 = OFF, 1 = ON

    Serial.print(F("📩 HMI Data -> VP: "));
    Serial.print(vpAddress);
    Serial.print(F(", Value: "));
    Serial.println(vpValue);

    switch (vpAddress)
    {
    case VP_POWER_SWITCH:
        powerSwitchState = (vpValue == 1);
        digitalWrite(LED_BUILTIN, powerSwitchState ? HIGH : LOW);
        Serial.println(powerSwitchState ? F("✅ Power Switch: ON") : F("❌ Power Switch: OFF"));
        break;

    case VP_BUTTON_2:
        button2State = (vpValue == 1);
        digitalWrite(RELAY_1_PIN, button2State ? HIGH : LOW);
        Serial.println(button2State ? F("⚡ Relay 1: ON") : F("🛑 Relay 1: OFF"));
        break;

    case VP_BUTTON_3:
        button3State = (vpValue == 1);
        digitalWrite(RELAY_2_PIN, button3State ? HIGH : LOW);
        Serial.println(button3State ? F("⚡ Relay 2: ON") : F("🛑 Relay 2: OFF"));
        break;

    default:
        Serial.println(F("⚠ VP Address tidak dikenali"));
        break;
    }
}

// kirim data ke HMI
void updateHmiDisplay(float temperature, float weight, float humidity, bool powerStatus)
{
    // kirim nilai temperatur ke HMI
    hmi.setText(VP_TEMP_DISPLAY, (String(temperature, 2))); // misal dikali 10 biar tampil 25.6°C jadi 256
    // kirim nilai berat
    hmi.setText(VP_WEIGHT_DISPLAY, (String(weight, 1)));
    // kirim status sistem
    hmi.setText(VP_HUMIDITY_DISPLAY, (String(humidity)));
}

void displayStatus()
{
    Serial.println(F("\n========== System Status =========="));
    Serial.print(F("Uptime: "));
    Serial.print(millis() / 1000);
    Serial.println(F(" seconds"));

    Serial.print(F("Local Storage: "));
    if (localStorage && localStorage->isReady())
    {
        Serial.print(F("OK ("));
        Serial.print(localStorage->getRecordCount());
        Serial.print(F("/"));
        Serial.print(MAX_RECORDS);
        Serial.println(F(" records)"));
    }
    else
    {
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

void handleSerialCommands()
{
    if (Serial.available())
    {
        char cmd = Serial.read();

        switch (cmd)
        {
        case 'r': // Read local storage
        {
            Serial.println(F("\nLocal Storage Records:"));
            SensorData data;
            for (int i = 0; i < localStorage->getRecordCount(); i++)
            {
                if (localStorage->retrieveData(data, i))
                {
                    Serial.print(i);
                    Serial.print(F(": "));
                    Serial.println(data.toCSV());
                }
            }
        }
        break;

        case 'c': // Clear local storage
            localStorage->clearStorage();
            Serial.println(F("Local storage cleared"));
            break;

        case 's': // Force sync to Firebase
            Serial.println(F("Firebase sync not available on ATmega2560"));
            break;

        case 'e': // Export to CSV
        {
            String csvExport;
            if (localStorage->exportToCSV(csvExport))
            {
                Serial.println(F("\nCSV Export:"));
                Serial.println(csvExport);
            }
        }
        break;

        case 'h': // Help
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

void setup()
{
    Serial.begin(115200);
    pinMode(RELAY_1_PIN, OUTPUT);
    pinMode(RELAY_2_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT); // buat indikator Power Switch

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
    if (!localStorage->initialize())
    {
        Serial.println(F("WARNING: Local storage initialization failed"));
    }

    hmi.hmiCallBack(hmiCallback);
    hmi.echoEnabled(true);

    Serial.println(F("INFO: Firebase not available on ATmega2560 - using local storage only"));

    Serial.println(F("\nSystem ready. Type 'h' for help.\n"));
}

void loop()
{
    unsigned long currentTime = millis();

    // Sample sensors at defined interval
    if (currentTime - lastSampleTime >= SAMPLE_INTERVAL)
    {
        lastSampleTime = currentTime;

        // Generate simulated sensor data
        SensorData data = generateSimulatedData();

        Serial.print(F("New data: "));
        Serial.println(data.toCSV());

        // Save to local storage
        if (!localStorage->saveData(data))
        {
            Serial.println(F("ERROR: Failed to save to local storage"));
        }
    }

    // Display status every 30 seconds
    if (currentTime - lastDisplayTime >= 30000)
    {
        lastDisplayTime = currentTime;
        displayStatus();
    }

    if (Serial.available())
    {
        String input = Serial.readStringUntil('\n');
        int commaIndex = input.indexOf(',');
        if (commaIndex > 0)
        {
            String addr = input.substring(0, commaIndex);
            String val = input.substring(commaIndex + 1);
            hmiCallback(addr, 0, val, input);
        }
    }

    // Handle serial commands
    handleSerialCommands();

    // Small delay to prevent overwhelming the system
    delay(10);
}