#include "LocalStorage.h"

LocalStorage::LocalStorage(int maxRec, int recSize)
    : maxRecords(maxRec), recordSize(recSize), currentIndex(0), recordCount(0)
{
}

bool LocalStorage::initialize()
{
    Serial.println(F("Initializing Local Storage..."));

    // Initialize EEPROM (AVR boards don't need size parameter)
    EEPROM.begin();

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

    if (isFull())
    {
        // Implement circular buffer - overwrite oldest
        currentIndex = currentIndex % maxRecords;
    }

    String csvData = data.toCSV();
    if (csvData.length() > recordSize)
    {
        handleError("Data too large for record size");
        return false;
    }

    int address = calculateAddress(currentIndex);

    // Write data length first
    EEPROM.write(address, csvData.length());

    // Write actual data
    for (int i = 0; i < csvData.length(); i++)
    {
        EEPROM.write(address + 1 + i, csvData[i]);
    }

    // Clear remaining bytes in record
    for (int i = csvData.length() + 1; i < recordSize; i++)
    {
        EEPROM.write(address + i, 0);
    }

    currentIndex++;
    if (recordCount < maxRecords)
    {
        recordCount++;
    }

    writeHeader();
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

    // Read data length
    int dataLength = EEPROM.read(address);
    if (dataLength == 0 || dataLength > recordSize)
    {
        handleError("Invalid data length");
        return false;
    }

    // Read data
    String csvData = "";
    for (int i = 0; i < dataLength; i++)
    {
        csvData += (char)EEPROM.read(address + 1 + i);
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

    Serial.println(F("Local storage cleared"));
    return true;
}

void LocalStorage::writeHeader()
{
    // Magic number for validation
    EEPROM.write(0, 0xAB);
    EEPROM.write(1, 0xCD);

    // Store current index and record count
    EEPROM.write(2, (currentIndex >> 8) & 0xFF);
    EEPROM.write(3, currentIndex & 0xFF);
    EEPROM.write(4, (recordCount >> 8) & 0xFF);
    EEPROM.write(5, recordCount & 0xFF);

    // Checksum
    uint8_t checksum = 0xAB ^ 0xCD ^ (currentIndex >> 8) ^ (currentIndex & 0xFF) ^
                       (recordCount >> 8) ^ (recordCount & 0xFF);
    EEPROM.write(6, checksum);
}

bool LocalStorage::readHeader()
{
    // Check magic number
    if (EEPROM.read(0) != 0xAB || EEPROM.read(1) != 0xCD)
    {
        return false;
    }

    // Read values
    currentIndex = (EEPROM.read(2) << 8) | EEPROM.read(3);
    recordCount = (EEPROM.read(4) << 8) | EEPROM.read(5);

    // Validate checksum
    uint8_t checksum = 0xAB ^ 0xCD ^ (currentIndex >> 8) ^ (currentIndex & 0xFF) ^
                       (recordCount >> 8) ^ (recordCount & 0xFF);

    if (EEPROM.read(6) != checksum)
    {
        return false;
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