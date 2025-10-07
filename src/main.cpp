/**
 * @file main.cpp
 * @brief MEGA2560 - Sensor Broadcaster with HMI Display
 *
 * Job: Read sensors -> send JSON to ESP -> display on HMI
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include "SystemConfig.h"
#include <Adafruit_MAX31865.h>
#include "HX711.h"
#include "DWIN.h"

Adafruit_MAX31865 thermo = Adafruit_MAX31865(53, 51, 50, 52);
// Jika ingin hardware SPI cukup: Adafruit_MAX31865 thermo = Adafruit_MAX31865(10);

// ESP Communication
#define ESP_SERIAL ESP8266_SERIAL

// HMI Communication (using hardware Serial2 on pins 16=TX2, 17=RX2)
DWIN hmi(Serial2, 115200);

// VP address display di HMI
#define VP_TEMP_DISPLAY 5000
#define VP_WEIGHT_DISPLAY 5002
#define VP_KA_DISPLAY 5004

// VP Address untuk 3 tombol
#define VP_POWER_SWITCH 5500
#define VP_BUTTON_2 6500
#define VP_BUTTON_3 7500

// Timing
unsigned long lastSampleTime = 0;
unsigned long lastDisplayTime = 0;
unsigned long checkup = 0;
int statusSSR = 0;

HX711 scale;

// ================== Parameter Awal ==================
float calibration_factor = 208; // Sesuaikan hasil kalibrasi load cell
float M0 = 100.0;               // Massa tembakau awal (kg)
float w0 = 0.75;                // Kadar air awal (75%)
float batas_ka = 15.0;

// Sensor readings
float Temp = 0.0;
float Weight = 0.0;
float KadarAir = 0.0;

// Status tombol HMI
bool powerSwitchState = false;
bool button2State = false;
bool button3State = false;

// Read sensors and return JSON

// Read load cell and update global Weight and KadarAir variables
void loadCell()
{
    if (scale.is_ready())
    {
        float Mt = scale.get_units(10); // pembacaan berat rata-rata 10 kali (kg)

        // Rumus kadar air (%)
        float KA = ((Mt - M0 * (1 - w0)) / Mt) * 100.0;
        if (KA < 0)
            KA = 0; // Hindari nilai negatif

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
        Serial.println("HX711 tidak terdeteksi!");
        Weight = 0.0;
        KadarAir = 0.0;
    }
}

float rtdSensor()
{
    uint16_t rtd = thermo.readRTD();

    // Hitung ratio
    float ratio = rtd / 32768.0;

    // Hitung resistansi RTD
    float resistance = RREF * ratio;

    // Hitung tegangan pada RTD
    float vRTD = ratio * VBIAS;

    // Hitung temperatur (library method - available if needed)
    // float temperature = thermo.temperature(RNOMINAL, RREF);

    // === Persamaan regresi ===
    float suhu_reg = (vRTD - 0.2437) / 0.0007 - 1; // dari tegangan ke suhu
    // float vRTD_reg = 0.0007 * suhu + 0.2437; // jika mau dari suhu ke tegangan

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

    // Cek fault
    uint8_t fault = thermo.readFault();
    if (fault)
    {
        Serial.print("Fault 0x");
        Serial.println(fault, HEX);
        if (fault & MAX31865_FAULT_HIGHTHRESH)
            Serial.println("RTD High Threshold");
        if (fault & MAX31865_FAULT_LOWTHRESH)
            Serial.println("RTD Low Threshold");
        if (fault & MAX31865_FAULT_REFINLOW)
            Serial.println("REFIN- > 0.85 x Bias");
        if (fault & MAX31865_FAULT_REFINHIGH)
            Serial.println("REFIN- < 0.85 x Bias - FORCE- open");
        if (fault & MAX31865_FAULT_RTDINLOW)
            Serial.println("RTDIN- < 0.85 x Bias - FORCE- open");
        if (fault & MAX31865_FAULT_OVUV)
            Serial.println("Under/Over voltage");
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
    // Create JSON with global sensor values including relay states
    StaticJsonDocument<250> doc;
    doc["temp"] = Temp;
    doc["weight"] = Weight;
    doc["ka"] = KadarAir;
    doc["relay1"] = statusSSR;
    doc["relay2"] = digitalRead(RELAY_PIN2);
    doc["ts"] = millis();

    String json;
    serializeJson(doc, json);
    return json;
}

// HMI Callback untuk menangani input dari tombol HMI
void hmiCallback(String address, int lastByte, String message, String response)
{
    int vpAddress = address.toInt();
    int vpValue = message.toInt(); // 0 = OFF, 1 = ON

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

    case VP_BUTTON_2:
        button2State = (vpValue == 1);
        // Could control additional relay or function
        Serial.println(button2State ? F("Button 2: ON") : F("Button 2: OFF"));
        break;

    case VP_BUTTON_3:
        button3State = (vpValue == 1);
        // Could control additional relay or function
        Serial.println(button3State ? F("Button 3: ON") : F("Button 3: OFF"));
        break;

    default:
        Serial.println(F("VP Address tidak dikenali"));
        break;
    }
}

// Update HMI display with current sensor values
void updateHmiDisplay()
{
    // Send temperature to HMI
    hmi.setText(VP_TEMP_DISPLAY, String(Temp, 2));

    // Send weight to HMI
    hmi.setText(VP_WEIGHT_DISPLAY, String(Weight, 1));

    // Send kadar air (moisture content) to HMI
    hmi.setText(VP_KA_DISPLAY, String(KadarAir, 1));
}

// Display ALL ESP messages for debugging
void processESPMessages()
{
    while (ESP_SERIAL.available())
    {
        String msg = ESP_SERIAL.readStringUntil('\n');
        msg.trim();

        // Bounds check to prevent memory overflow
        if (msg.length() > 256)
        {
            ESP_SERIAL.flush(); // Clear corrupt data
            Serial.println(F("[ESP] Message too large, discarded"));
            continue;
        }

        if (msg.length() > 0)
        {
            // Show everything ESP sends
            Serial.println("[ESP] " + msg);
        }
    }
}

void setup()
{
    Serial.begin(115200);
    ESP_SERIAL.begin(ESP8266_BAUDRATE);
    delay(1000);

    Serial.println(F("\n================================"));
    Serial.println(F("MEGA2560 - Sensor Broadcaster"));
    Serial.println(F("================================"));
    Serial.println(F("Job: Read sensors -> Send JSON"));
    Serial.println(F("ESP handles all storage & upload"));
    Serial.println(F("================================\n"));

    // Initialize pins
    pinMode(RELAY_PIN1, OUTPUT);
    pinMode(RELAY_PIN2, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    // Initialize HX711 load cell
    scale.begin(HX711_DT, HX711_SCK);
    scale.set_scale(calibration_factor);
    scale.tare(); // Reset scale to 0

    Serial.println(F("HX711 initialized and tared"));

    // Initialize HMI
    hmi.hmiCallBack(hmiCallback);
    hmi.echoEnabled(true);
    Serial.println(F("HMI initialized"));

    // Test HMI communication
    Serial.println(F("Testing HMI on Serial2 (pins 16=RX2, 17=TX2)..."));
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
    Serial.println(F("If you see echo responses above, HMI is connected"));
    Serial.println(F("If screen shows nothing, check:"));
    Serial.println(F("  1. DWIN TX -> Mega pin 16 (RX2)"));
    Serial.println(F("  2. DWIN RX -> Mega pin 17 (TX2)"));
    Serial.println(F("  3. DWIN baud rate = 115200"));
    Serial.println(F("  4. VP addresses match your DWIN project"));

    // Wait for ESP to boot
    Serial.println(F("Waiting for ESP8266 to boot..."));
    delay(3000);

    // Clear boot garbage (74880 baud messages)
    while (ESP_SERIAL.available())
    {
        ESP_SERIAL.read();
    }

    // Initialize RTD sensor
    thermo.begin(MAX31865_3WIRE); // Sesuaikan: 2WIRE, 3WIRE, atau 4WIRE
    Serial.println(F("MAX31865 RTD sensor initialized"));

    Serial.println(F("System ready\n"));
}

void loop()
{
    unsigned long currentTime = millis();

    // Read sensors every SAMPLE_INTERVAL and send JSON
    if (currentTime - lastSampleTime >= SAMPLE_INTERVAL)
    {
        lastSampleTime = currentTime;

        // Read sensors - updates global Temp, Weight, KadarAir
        Temp = rtdSensor();
        loadCell();

        // Create JSON with updated sensor values
        String json = readSensors();

        // Send to ESP (no response expected)
        ESP_SERIAL.println(json);

        // Summary
        Serial.print(F("━━━ SUMMARY ━━━ Temp: "));
        Serial.print(Temp, 2);
        Serial.print(F("°C | Weight: "));
        Serial.print(Weight, 2);
        Serial.print(F("kg | Moisture: "));
        Serial.print(KadarAir, 1);
        Serial.println(F("% → ESP"));
    }

    // Update HMI display every 500ms (more responsive)
    if (currentTime - lastDisplayTime >= 500)
    {
        lastDisplayTime = currentTime;
        updateHmiDisplay();
    }

    // Process HMI input
    hmi.listen();

    // Process ESP messages
    processESPMessages();

    delay(10);
}