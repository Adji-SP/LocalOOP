


/**
 * @file main.cpp
 * @brief Multi-Board Sensor Broadcaster with HMI Display
 *
 * Supports: ATmega328P (Uno) and ATmega2560 (Mega)
 * Job: Read sensors -> send JSON to ESP -> display on HMI
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SystemConfig.h"
#include <Adafruit_MAX31865.h>
#include "HX711.h"
#include "DWIN.h"
#include "TimeSync.h"

// ========================================
// BOARD-SPECIFIC CONFIGURATION
// ========================================

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
    // ATmega328P (Arduino Uno)
    #define BOARD_NAME "Arduino Uno (ATmega328P)"

    // HMI Communication via SoftwareSerial
    #include <SoftwareSerial.h>
    #define HMI_RX_PIN 10
    #define HMI_TX_PIN 11
    SoftwareSerial hmiSerial(HMI_RX_PIN, HMI_TX_PIN);
    DWIN hmi(hmiSerial, 115200);

    // MAX31865 RTD Sensor (Software SPI)
    #define RTD_CS_PIN 10
    #define RTD_MOSI_PIN 11
    #define RTD_MISO_PIN 12
    #define RTD_SCK_PIN 13
    Adafruit_MAX31865 thermo = Adafruit_MAX31865(RTD_CS_PIN, RTD_MOSI_PIN, RTD_MISO_PIN, RTD_SCK_PIN);

    // ESP Communication (Software Serial or disabled for Uno - not enough serial ports)
    // For testing on Uno, you might not have ESP connected
    #define ESP_AVAILABLE false

#else
    // ATmega2560 (Arduino Mega)
    #define BOARD_NAME "Arduino Mega 2560"

    // HMI Communication via Hardware Serial2
    #define HMI_RX_PIN 19  // RX2 (fixed on Mega - receives from DWIN TX)
    #define HMI_TX_PIN 18  // TX2 (fixed on Mega - transmits to DWIN RX)
    DWIN hmi(Serial1, HMI_RX_PIN, HMI_TX_PIN, 115200);  // Pins are fixed on Mega but specified for clarity

    // MAX31865 RTD Sensor (Software SPI for flexibility)
    Adafruit_MAX31865 thermo = Adafruit_MAX31865(53, 51, 50, 52);
    // For hardware SPI: Adafruit_MAX31865 thermo = Adafruit_MAX31865(10);

    // ESP Communication via Serial3
    #define ESP_AVAILABLE true

#endif

// ESP Serial definition
#define ESP_SERIAL ESP8266_SERIAL

// ========================================
// HMI VP ADDRESSES
// ========================================
#define VP_TEMP_DISPLAY 5000
#define VP_WEIGHT_DISPLAY 5002
#define VP_KA_DISPLAY 5004
#define VP_POWER_SWITCH 5500
#define VP_RELAY1_STATUS 6500
#define VP_RELAY2_STATUS 7500

// ========================================
// GLOBAL VARIABLES
// ========================================

// Timing
unsigned long lastSampleTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long checkup = 0;
int statusSSR = 0;

// HX711 Load Cell
HX711 scale;

// Calibration Parameters
float calibration_factor = 208;
float M0 = 100.0;    // Initial tobacco mass (kg)
float w0 = 0.75;     // Initial moisture content (75%)
float batas_ka = 15.0;

// Sensor Readings
float Temp = 0.0;
float Weight = 0.0;
float KadarAir = 0.0;

// HMI Button States
bool powerSwitchState = false;

// TimeSync instance
TimeSync timeSync;

// ========================================
// SENSOR FUNCTIONS
// ========================================

void loadCell()
{
    if (scale.is_ready())
    {
        float Mt = scale.get_units(10); // Average of 10 readings

        // Moisture content calculation
        float KA = ((Mt - M0 * (1 - w0)) / Mt) * 100.0;
        if (KA < 0) KA = 0;

        // Update global variables
        Weight = Mt;
        KadarAir = KA;

        Serial.print(F("Weight: "));
        Serial.print(Mt, 2);
        Serial.print(F("kg | Moisture: "));
        Serial.print(KA, 2);
        Serial.print(F("% | Relay2: "));

        // Moisture control
        if (KA <= batas_ka)
        {
            digitalWrite(RELAY_PIN2, HIGH);
            Serial.println(F("ON (Heater OFF)"));
        }
        else
        {
            digitalWrite(RELAY_PIN2, LOW);
            Serial.println(F("OFF (Heater ON)"));
        }
    }
    else
    {
        Serial.println(F("HX711 not detected!"));
        Weight = 0.0;
        KadarAir = 0.0;
    }
}

float rtdSensor()
{
    uint16_t rtd = thermo.readRTD();

    // Calculate ratio
    float ratio = rtd / 32768.0;

    // Calculate RTD resistance
    float resistance = RREF * ratio;

    // Calculate RTD voltage
    float vRTD = ratio * VBIAS;

    // Regression equation (voltage to temperature)
    float suhu_reg = (vRTD - 0.2437) / 0.0007 - 1;

    // Print sensor data
    Serial.print(F("RTD: "));
    Serial.print(rtd);
    Serial.print(F(" | Resistance: "));
    Serial.print(resistance, 3);
    Serial.print(F("Ω | Voltage: "));
    Serial.print(vRTD, 4);
    Serial.print(F("V | Temp: "));
    Serial.print(suhu_reg, 2);
    Serial.print(F("°C"));

    // Check fault
    uint8_t fault = thermo.readFault();
    if (fault)
    {
        Serial.print(F(" Fault 0x"));
        Serial.println(fault, HEX);
        if (fault & MAX31865_FAULT_HIGHTHRESH)
            Serial.println(F("  RTD High Threshold"));
        if (fault & MAX31865_FAULT_LOWTHRESH)
            Serial.println(F("  RTD Low Threshold"));
        if (fault & MAX31865_FAULT_REFINLOW)
            Serial.println(F("  REFIN- > 0.85 x Bias"));
        if (fault & MAX31865_FAULT_REFINHIGH)
            Serial.println(F("  REFIN- < 0.85 x Bias"));
        if (fault & MAX31865_FAULT_RTDINLOW)
            Serial.println(F("  RTDIN- < 0.85 x Bias"));
        if (fault & MAX31865_FAULT_OVUV)
            Serial.println(F("  Under/Over voltage"));
        thermo.clearFault();
    }

    // Temperature control
    if (suhu_reg >= 70)
    {
        digitalWrite(RELAY_PIN1, LOW);
        statusSSR = 0;
    }
    if (suhu_reg <= 40)
    {
        digitalWrite(RELAY_PIN1, HIGH);
        statusSSR = 1;
    }
    Serial.print(F(" | SSR: "));
    Serial.println(statusSSR ? F("ON") : F("OFF"));

    return suhu_reg;
}

String readSensors()
{
    StaticJsonDocument<250> doc;
    doc["temp"] = Temp;
    doc["weight"] = Weight;
    doc["ka"] = KadarAir;
    doc["relay1"] = statusSSR;
    doc["relay2"] = digitalRead(RELAY_PIN2);

    // Use real Unix timestamp if available, otherwise use millis
    if (timeSync.isSynced())
    {
        doc["ts"] = timeSync.getUnixTime();
    }
    else
    {
        doc["ts"] = millis() / 1000; // Fallback to millis in seconds
    }

    String json;
    serializeJson(doc, json);
    return json;
}

// ========================================
// HMI FUNCTIONS
// ========================================

void hmiCallback(String address, int lastByte, String message, String response)
{
    int vpAddress = address.toInt();
    int vpValue = message.toInt();

    Serial.print(F("HMI Data -> VP: "));
    Serial.print(vpAddress);
    Serial.print(F(", Value: "));
    Serial.println(vpValue);

    switch (vpAddress)
    {
    case VP_POWER_SWITCH:
        powerSwitchState = (vpValue == 1);
        digitalWrite(LED_BUILTIN, powerSwitchState ? HIGH : LOW);
        Serial.println(powerSwitchState ? F("Power Switch: ON") : F("Power Switch: OFF"));
        break;

    default:
        Serial.println(F("VP Address not recognized"));
        break;
    }
}

void updateHmiDisplay()
{
    hmi.setText(VP_TEMP_DISPLAY, String(Temp, 2));
    hmi.setText(VP_WEIGHT_DISPLAY, String(Weight, 1));
    hmi.setText(VP_KA_DISPLAY, String(KadarAir, 1));
    hmi.writeWord(VP_RELAY1_STATUS, statusSSR ? 1 : 0);
    hmi.writeWord(VP_RELAY2_STATUS, digitalRead(RELAY_PIN2) ? 1 : 0);
}

// ========================================
// ESP COMMUNICATION
// ========================================

void processESPMessages()
{
    #if ESP_AVAILABLE
    while (ESP_SERIAL.available())
    {
        String msg = ESP_SERIAL.readStringUntil('\n');
        msg.trim();

        if (msg.length() > 256)
        {
            ESP_SERIAL.flush();
            Serial.println(F("[ESP] Message too large, discarded"));
            continue;
        }

        if (msg.length() > 0)
        {
            // Check if it's a TIME sync message
            if (msg.startsWith("TIME:"))
            {
                // Extract Unix timestamp
                unsigned long unixTime = msg.substring(5).toInt();
                if (unixTime > 1609459200) // Sanity check: after 2021-01-01
                {
                    timeSync.setUnixTime(unixTime);
                    Serial.print(F("[ESP] Time synced: "));
                    Serial.println(unixTime);
                }
            }
            else
            {
                // Show everything else ESP sends
                Serial.println("[ESP] " + msg);
            }
        }
    }
    #endif
}

// ========================================
// SETUP
// ========================================

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println(F("\n================================"));
    Serial.print(F("Board: "));
    Serial.println(F(BOARD_NAME));
    Serial.println(F("Sensor Broadcaster with HMI"));
    Serial.println(F("================================"));
    Serial.println(F("Job: Read sensors -> Send JSON"));
    #if ESP_AVAILABLE
    Serial.println(F("ESP handles storage & upload"));
    #else
    Serial.println(F("ESP disabled (Uno testing mode)"));
    #endif
    Serial.println(F("================================\n"));

    // Initialize pins
    pinMode(RELAY_PIN1, OUTPUT);
    pinMode(RELAY_PIN2, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    // Initialize HX711 load cell
    scale.begin(HX711_DT, HX711_SCK);
    scale.set_scale(calibration_factor);
    scale.tare();
    Serial.println(F("HX711 initialized and tared"));

    // Initialize TimeSync
    timeSync.begin();
    Serial.println(F("TimeSync initialized (waiting for ESP8266)"));

    // Initialize HMI
    #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
        hmiSerial.begin(115200);
        Serial.print(F("HMI initialized on SoftwareSerial (RX="));
        Serial.print(HMI_RX_PIN);
        Serial.print(F(", TX="));
        Serial.print(HMI_TX_PIN);
        Serial.println(F(")"));
    #else
        Serial.print(F("HMI initialized on Serial2 (TX2=pin"));
        Serial.print(HMI_TX_PIN);
        Serial.print(F(", RX2=pin"));
        Serial.print(HMI_RX_PIN);
        Serial.println(F(")"));
    #endif

    hmi.hmiCallBack(hmiCallback);
    hmi.echoEnabled(true);

    // Test HMI communication
    Serial.println(F("Testing HMI communication..."));

    Serial.print(F("Sending to VP "));
    Serial.print(VP_TEMP_DISPLAY);
    Serial.println(F(": TEST"));
    hmi.setText(VP_TEMP_DISPLAY, "TEST");
    delay(1000);

    Serial.print(F("Sending to VP "));
    Serial.print(VP_WEIGHT_DISPLAY);
    Serial.println(F(": 123"));
    hmi.setText(VP_WEIGHT_DISPLAY, "123");
    delay(1000);

    Serial.print(F("Sending to VP "));
    Serial.print(VP_KA_DISPLAY);
    Serial.println(F(": 456"));
    hmi.setText(VP_KA_DISPLAY, "456");
    delay(1000);

    Serial.println(F("HMI test complete"));
    Serial.println(F("If you see echo responses, HMI is connected"));
    Serial.println(F("If screen shows nothing, check:"));
    #if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328__)
        Serial.print(F("  1. DWIN TX -> Arduino Uno pin "));
        Serial.println(HMI_RX_PIN);
        Serial.print(F("  2. DWIN RX -> Arduino Uno pin "));
        Serial.println(HMI_TX_PIN);
    #else
        Serial.print(F("  1. DWIN TX -> Mega RX2 (pin "));
        Serial.print(HMI_RX_PIN);
        Serial.println(F(")"));
        Serial.print(F("  2. DWIN RX -> Mega TX2 (pin "));
        Serial.print(HMI_TX_PIN);
        Serial.println(F(")"));
    #endif
    Serial.println(F("  3. DWIN baud rate = 115200"));
    Serial.println(F("  4. VP addresses match your DWIN project"));

    // Initialize ESP communication
    #if ESP_AVAILABLE
    ESP_SERIAL.begin(ESP8266_BAUDRATE);
    Serial.println(F("Waiting for ESP8266 to boot..."));
    delay(3000);

    // Clear boot garbage
    while (ESP_SERIAL.available())
    {
        ESP_SERIAL.read();
    }
    Serial.println(F("ESP8266 ready"));
    #endif

    // Initialize RTD sensor
    thermo.begin(MAX31865_3WIRE);
    Serial.println(F("MAX31865 RTD sensor initialized"));

    Serial.println(F("System ready\n"));
}

// ========================================
// MAIN LOOP
// ========================================

void loop()
{
    unsigned long currentTime = millis();

    // Read sensors every SAMPLE_INTERVAL
    if (currentTime - lastSampleTime >= SAMPLE_INTERVAL)
    {
        lastSampleTime = currentTime;

        // Read sensors
        Temp = rtdSensor();
        loadCell();

        // Create JSON
        String json = readSensors();

        // Send to ESP
        #if ESP_AVAILABLE
        ESP_SERIAL.println(json);
        #endif

        // Summary
        Serial.print(F("━━━ SUMMARY ━━━ Temp: "));
        Serial.print(Temp, 2);
        Serial.print(F("°C | Weight: "));
        Serial.print(Weight, 2);
        Serial.print(F("kg | Moisture: "));
        Serial.print(KadarAir, 1);
        #if ESP_AVAILABLE
        Serial.println(F("% → ESP"));
        #else
        Serial.println(F("%"));
        #endif
    }

    // Update HMI display every 500ms
    if (currentTime - lastDisplayTime >= 500)
    {
        lastDisplayTime = currentTime;
        updateHmiDisplay();
    }

    // Process HMI input
    hmi.listen();

    // Process ESP messages
    #if ESP_AVAILABLE
    processESPMessages();
    #endif

    delay(10);
}
