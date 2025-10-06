#include "LocalStorage.h"

LocalStorage::LocalStorage(int maxRec, int recSize)
    : maxRecords(maxRec), recordSize(recSize), currentIndex(0), recordCount(0)
{
}

bool LocalStorage::initialize()
{
    Serial.println(F("Initializing Local Storage..."));

    // ESP8266/ESP32 need EEPROM.begin(), AVR doesn't
    #if defined(ESP8266) || defined(ESP32)
        // Calculate required EEPROM size based on configuration
        int requiredSize = HEADER_SIZE + (maxRecords * recordSize);
        if (requiredSize > 4096) {
            Serial.print(F("ERROR: EEPROM config exceeds ESP8266 limit! Required: "));
            Serial.print(requiredSize);
            Serial.println(F(" bytes, Max: 4096 bytes"));
            return false;
        }
        Serial.print(F("EEPROM allocating "));
        Serial.print(requiredSize);
        Serial.println(F(" bytes"));
        EEPROM.begin(requiredSize);
    #endif

    // Check if storage has been initialized before
    if (!readHeader())
    {
        // First time initialization
        Serial.println(F("First time EEPROM initialization"));
        clearStorage();
    }

    isInitialized = true;
    Serial.println(F("Local Storage initialized successfully"));
    return true;
}

bool LocalStorage::saveData(const SensorData &data)
{
    if (!isInitialized)
    {
        handleError("Storage not initialized");
        return false;
    }

    String csvData = data.toCSV();

    // Check if CSV data fits in record (accounting for 2-byte length prefix)
    if (csvData.length() > (unsigned int)(recordSize - 2))
    {
        handleError("Data too large for record size");
        return false;
    }

    int address = calculateAddress(currentIndex);

    // CRITICAL: Bounds check for EEPROM overflow
    if (address + recordSize > EEPROM.length())
    {
        handleError("EEPROM overflow: insufficient space");
        return false;
    }

    // Write 2-byte length header (supports up to 65535 bytes)
    uint16_t dataLen = csvData.length();
    EEPROM.write(address, (dataLen >> 8) & 0xFF);      // High byte
    EEPROM.write(address + 1, dataLen & 0xFF);         // Low byte

    // Write actual data
    for (unsigned int i = 0; i < csvData.length(); i++)
    {
        EEPROM.write(address + 2 + i, csvData[i]);
    }

    // Clear remaining bytes in record
    for (unsigned int i = csvData.length() + 2; i < (unsigned int)recordSize; i++)
    {
        EEPROM.write(address + i, 0);
    }

    // Circular buffer: rotate index properly
    currentIndex = (currentIndex + 1) % maxRecords;
    if (recordCount < maxRecords)
    {
        recordCount++;
    }

    // Reduce EEPROM wear: only write header every 10 writes or when buffer wraps
    static uint8_t writeCounter = 0;
    if (++writeCounter >= 10 || currentIndex == 0) {
        writeHeader();
        writeCounter = 0;
    }

    // ESP8266/ESP32 need commit() to persist changes
    #if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
    #endif

    return true;
}

bool LocalStorage::retrieveData(SensorData &data, int index)
{
    if (!isInitialized)
    {
        handleError("Storage not initialized");
        return false;
    }

    if (index >= recordCount)
    {
        handleError("Index out of range");
        return false;
    }

    int address = calculateAddress(index);

    // Read 2-byte length header
    uint16_t dataLength = (EEPROM.read(address) << 8) | EEPROM.read(address + 1);

    if (dataLength == 0 || dataLength > (unsigned int)(recordSize - 2))
    {
        handleError("Invalid data length");
        return false;
    }

    // Read data
    String csvData = "";
    csvData.reserve(dataLength);  // Pre-allocate for efficiency
    for (unsigned int i = 0; i < dataLength; i++)
    {
        csvData += (char)EEPROM.read(address + 2 + i);
    }

    return data.fromCSV(csvData);
}

int LocalStorage::getRecordCount()
{
    return recordCount;
}

bool LocalStorage::clearStorage()
{
    Serial.println(F("Clearing local storage..."));

    // Clear header
    for (int i = 0; i < HEADER_SIZE; i++)
    {
        EEPROM.write(i, 0);
    }

    currentIndex = 0;
    recordCount = 0;

    writeHeader();

    // ESP8266/ESP32 need commit() to persist changes
    #if defined(ESP8266) || defined(ESP32)
        EEPROM.commit();
    #endif

    Serial.println(F("Local storage cleared"));
    return true;
}

void LocalStorage::writeHeader()
{
    // Magic number for validation
    EEPROM.write(0, 0xAB);
    EEPROM.write(1, 0xCD);

    // Version byte
    EEPROM.write(2, STORAGE_VERSION);

    // Store current index (2 bytes)
    EEPROM.write(3, (currentIndex >> 8) & 0xFF);
    EEPROM.write(4, currentIndex & 0xFF);

    // Store record count (2 bytes)
    EEPROM.write(5, (recordCount >> 8) & 0xFF);
    EEPROM.write(6, recordCount & 0xFF);

    // Checksum (includes version)
    uint8_t checksum = 0xAB ^ 0xCD ^ STORAGE_VERSION ^
                       (currentIndex >> 8) ^ (currentIndex & 0xFF) ^
                       (recordCount >> 8) ^ (recordCount & 0xFF);
    EEPROM.write(7, checksum);

    // Reserved bytes (8-9) for future use
    EEPROM.write(8, 0x00);
    EEPROM.write(9, 0x00);
}

bool LocalStorage::readHeader()
{
    // Check magic number
    if (EEPROM.read(0) != 0xAB || EEPROM.read(1) != 0xCD)
    {
        return false;
    }

    // Check version compatibility
    uint8_t version = EEPROM.read(2);
    if (version != STORAGE_VERSION)
    {
        Serial.print(F("WARNING: EEPROM version mismatch. Expected: "));
        Serial.print(STORAGE_VERSION);
        Serial.print(F(", Found: "));
        Serial.println(version);
        return false;  // Incompatible version, reinitialize
    }

    // Read values
    currentIndex = (EEPROM.read(3) << 8) | EEPROM.read(4);
    recordCount = (EEPROM.read(5) << 8) | EEPROM.read(6);

    // Validate checksum
    uint8_t checksum = 0xAB ^ 0xCD ^ STORAGE_VERSION ^
                       (currentIndex >> 8) ^ (currentIndex & 0xFF) ^
                       (recordCount >> 8) ^ (recordCount & 0xFF);

    if (EEPROM.read(7) != checksum)
    {
        Serial.println(F("WARNING: EEPROM checksum failed"));
        return false;
    }

    // Bounds validation - prevent corruption
    if (recordCount > maxRecords)
    {
        Serial.println(F("WARNING: Record count exceeds max, resetting"));
        recordCount = 0;
    }

    if (currentIndex >= maxRecords)
    {
        Serial.println(F("WARNING: Current index out of bounds, resetting"));
        currentIndex = 0;
    }

    return true;
}

int LocalStorage::calculateAddress(int index)
{
    return RECORD_START + (index * recordSize);
}

bool LocalStorage::isFull() const
{
    return recordCount >= maxRecords;
}

int LocalStorage::getFreeSpace() const
{
    return maxRecords - recordCount;
}

bool LocalStorage::exportToCSV(String &output, int startIndex, int count)
{
    if (count == -1)
    {
        count = recordCount;
    }

    output = "Timestamp,Temperature,Weight,Status\n";
    SensorData data;

    for (int i = startIndex; i < min(startIndex + count, recordCount); i++)
    {
        if (retrieveData(data, i))
        {
            output += data.toCSV() + "\n";
        }
    }

    return true;
}