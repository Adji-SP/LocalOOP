# DWIN HMI Wiring Guide

## Arduino Mega 2560 Wiring

### Hardware Connections

```
DWIN HMI          Arduino Mega 2560
---------         -----------------
TX      ------→   Pin 17 (RX2)
RX      ------→   Pin 16 (TX2)
GND     ------→   GND
5V      ------→   5V
```

### Code Configuration

```cpp
// In main.cpp (for Mega 2560)
#define HMI_RX_PIN 17  // RX2 - receives from DWIN TX
#define HMI_TX_PIN 16  // TX2 - transmits to DWIN RX

DWIN hmi(Serial2, HMI_RX_PIN, HMI_TX_PIN, 115200);
```

**Important Notes:**
- On Arduino Mega, Serial2 pins are **fixed in hardware**
- You cannot change RX2/TX2 to other pins
- The pin parameters in the constructor are for API compatibility with ESP32
- Serial2 always uses pins 16 (TX) and 17 (RX)

---

## Arduino Uno Wiring (SoftwareSerial)

### Hardware Connections

```
DWIN HMI          Arduino Uno
---------         -----------
TX      ------→   Pin 10 (Software RX)
RX      ------→   Pin 11 (Software TX)
GND     ------→   GND
5V      ------→   5V
```

### Code Configuration

```cpp
// In main.cpp (for Uno)
#define HMI_RX_PIN 10
#define HMI_TX_PIN 11

SoftwareSerial hmiSerial(HMI_RX_PIN, HMI_TX_PIN);
DWIN hmi(hmiSerial, 115200);
```

**Important Notes:**
- Arduino Uno only has one hardware serial (Serial) used for USB
- Must use SoftwareSerial for DWIN HMI
- Pins 10/11 can be changed to other digital pins if needed

---

## ESP32 Wiring (Configurable Pins)

### Hardware Connections

```
DWIN HMI          ESP32
---------         -----
TX      ------→   GPIO 16 (RX2)
RX      ------→   GPIO 17 (TX2)
GND     ------→   GND
5V      ------→   5V (or 3.3V depending on DWIN model)
```

### Code Configuration

```cpp
// For ESP32
#define DGUS_SERIAL Serial2
#define DGUS_RX_PIN 16
#define DGUS_TX_PIN 17
#define DGUS_BAUD 115200

DWIN hmi(DGUS_SERIAL, DGUS_RX_PIN, DGUS_TX_PIN, DGUS_BAUD);
```

**Important Notes:**
- ESP32 supports configurable UART pins
- You can change RX/TX pins to other GPIO pins
- The pins are passed to hardware serial configuration
- This is why ESP32 syntax is different from Arduino Mega!

---

## Key Differences

| Feature | ESP32 | Arduino Mega | Arduino Uno |
|---------|-------|--------------|-------------|
| **UART Pins** | Configurable | Fixed | N/A (uses SoftwareSerial) |
| **Constructor** | `DWIN(Serial2, 16, 17, 115200)` | `DWIN(Serial2, 17, 16, 115200)` | `DWIN(10, 11, 115200)` |
| **Pin Parameters** | Actually configure hardware | Ignored (pins are fixed) | Define SoftwareSerial pins |
| **Hardware Serial** | Serial1, Serial2 | Serial1, Serial2, Serial3 | Serial (USB only) |

---

## Arduino Mega Serial Port Pinout Reference

| Serial Port | RX Pin | TX Pin | Notes |
|-------------|--------|--------|-------|
| **Serial** | 0 | 1 | Used for USB communication |
| **Serial1** | 19 | 18 | Available for DWIN |
| **Serial2** | 17 | 16 | **Recommended for DWIN** |
| **Serial3** | 15 | 14 | Used for ESP8266 in this project |

**Why Serial2 for DWIN?**
- Serial1 could interfere with nearby pins
- Serial3 is used for ESP8266 communication
- Serial2 is the most isolated and reliable choice

---

## Troubleshooting

### DWIN not responding on Mega?

1. **Check wiring:**
   ```
   DWIN TX → Mega Pin 17 (RX2)
   DWIN RX → Mega Pin 16 (TX2)
   ```

2. **Check baud rate:**
   - DWIN must be configured to 115200 baud
   - Use DWIN configuration tool to verify

3. **Check DWIN power:**
   - DWIN needs 5V and GND connected
   - Some DWIN displays need separate power supply

4. **Verify Serial2 is working:**
   ```cpp
   Serial2.begin(115200);
   Serial2.println("TEST");
   ```

5. **Check VP addresses:**
   - Make sure VP addresses match your DWIN project
   - Default: 5000, 5002, 5004, 5500, 6500, 7500

### Compilation errors?

**Error: "no matching function for call to DWIN::DWIN"**
- Make sure you're using the updated DWIN library
- Check that DWIN.h and DWIN.cpp have the latest constructors

**Error: "Serial2 was not declared"**
- You're compiling for Arduino Uno (no Serial2)
- Use the Uno configuration with SoftwareSerial instead

---

## Code Examples

### Complete Mega 2560 Setup

```cpp
#include "DWIN.h"

#define HMI_RX_PIN 17  // RX2
#define HMI_TX_PIN 16  // TX2

DWIN hmi(Serial2, HMI_RX_PIN, HMI_TX_PIN, 115200);

void setup() {
    Serial.begin(115200);

    hmi.echoEnabled(true);

    // Test HMI
    hmi.setText(5000, "Hello");
    delay(1000);

    Serial.println("DWIN ready!");
}

void loop() {
    hmi.listen();
    delay(10);
}
```

### Complete Arduino Uno Setup

```cpp
#include "DWIN.h"
#include <SoftwareSerial.h>

#define HMI_RX_PIN 10
#define HMI_TX_PIN 11

SoftwareSerial hmiSerial(HMI_RX_PIN, HMI_TX_PIN);
DWIN hmi(hmiSerial, 115200);

void setup() {
    Serial.begin(115200);
    hmiSerial.begin(115200);

    hmi.echoEnabled(true);

    // Test HMI
    hmi.setText(5000, "Hello");
    delay(1000);

    Serial.println("DWIN ready!");
}

void loop() {
    hmi.listen();
    delay(10);
}
```

---

## Summary

✅ **Arduino Mega** uses `DWIN(Serial2, 17, 16, 115200)` - pins are fixed but specified for clarity
✅ **Arduino Uno** uses `DWIN(hmiSerial, 115200)` - with SoftwareSerial
✅ **ESP32** uses `DWIN(Serial2, 16, 17, 115200)` - pins are actually configurable

The syntax matches the DWIN documentation while working correctly on all platforms!
