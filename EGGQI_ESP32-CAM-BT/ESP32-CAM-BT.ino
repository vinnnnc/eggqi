#include <Arduino.h>
#include "camera.h"
#include <SPI.h>
#include "BluetoothSerial.h"

// HardwareSerial pins (ESP32 - Arduino serial communication)
#define RXD1 12
#define TXD1 13

#define BTname "EggQI"
BluetoothSerial SerialBT;
int mode = 0;
const int qualityMode = 0;
const int ageMode = 1;

String date = "";
int set = 1;

void setup()
{
    Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
    Serial.begin(115200);
    initBT();
    setupCamera();
    readConfigFile();
    cameraImageSettings();
}

void loop()
{
    // Commands from Arduino Uno hardwareserial
    while (Serial1.available())
    {
        flashLED(1);
        // If data is available from the Bluetooth connection, read and process it
        String receivedData = Serial1.readStringUntil('\n');
        receivedData.trim();
        if (serialDebug)
            Serial.println("Arduino Received data: " + receivedData);
        if (receivedData == "GET")
        {
            if (serialDebug)
                Serial.println("processing...");
            handleGetValues();
        }
        if (receivedData == "AGEMODE")
        {
            if (serialDebug)
                Serial.println("Set to Age Mode by Arduino");
            mode = ageMode;
        }
        if (receivedData == "QLTMODE")
        {
            if (serialDebug)
                Serial.println("Set to Quality Mode by Arduino");
            mode = qualityMode;
        }
        if (receivedData.startsWith("SAVE"))
        {
            if (serialDebug)
                Serial.println("Saving...");
            String getDate = receivedData.substring(5, 15);
            String getTime = receivedData.substring(17);
            if (getDate != date)
                date = getDate;
            char s = handleSave(getTime);
            Serial1.print(s);
        }
    }
}

// Initialize Bluetooth
void initBT()
{
    if (!SerialBT.begin(BTname))
    {
        if (serialDebug)
            Serial.println("An error occurred initializing Bluetooth");
        ESP.restart();
    }
    else
    {
        if (serialDebug)
            Serial.println("Bluetooth initialized");
    }
    SerialBT.register_callback(btCallback);
    if (serialDebug)
        Serial.println("The device started, now you can pair it with bluetooth");
}

// Delete all files in Micro SD card
void deleteFiles(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            char path1[200] = "/";
            strcat(path1, file.name());
            Serial.println(path1);
            deleteFiles(fs, path1, levels + 1);
        }
        else
        {
            char path[200] = "";
            strcat(path, dirname);
            strcat(path, "/");
            strcat(path, file.name());
            Serial.print("  FILE: ");
            Serial.print(path);
            Serial.print("  SIZE: ");
            Serial.println(file.size());
            fs.remove(path);
        }
        file = root.openNextFile();
    }
}

// Send save/log files through SerialBT (Bluetooth Serial)
void sendLogsBT(const char *directory)
{
    File root = SD_MMC.open(directory);
    if (!root)
    {
        Serial.println("Failed to open directory");
        Serial1.print('0');
        return;
    }

    while (File file = root.openNextFile())
    {
        if (file.isDirectory())
        {
            // Skip subdirectories
            continue;
        }

        const char *fileName = file.name();
        Serial.println(fileName);
        sendTxtBT(fileName);
    }

    root.close();
    Serial1.print('1');
    Serial.println("Done");
}

// Function used in sendLogsBT
void sendTxtBT(const char *fileName)
{
    char path[50];
    snprintf(path, sizeof(path), "/logs/%s", fileName);
    File file = SD_MMC.open(path, FILE_READ);
    if (!file)
    {
        Serial.println("Failed to open file for reading");
        return;
    }
    SerialBT.print("LOG:");
    SerialBT.print(fileName);
    SerialBT.print(":");
    while (file.available())
    {
        char c = file.read();
        SerialBT.write(c);
    }
    file.close();
    SerialBT.write('\0');
    SerialBT.flush();
    Serial.println("File sent over Bluetooth: " + String(fileName));
}

// Function for saving to txt file
char handleSave(String time)
{
    if (!SD_MMC.exists("/logs"))
        SD_MMC.mkdir("/logs");
    String fileName = "/logs/EggQI_" + date + "_log.txt";
    Serial.println(fileName);
    File file = SD_MMC.open(fileName, FILE_APPEND); // Use FILE_APPEND to append to the existing file
    if (!file)
    {
        file = SD_MMC.open(fileName, FILE_WRITE);
        if (!file)
        {
            Serial.println("Failed to open file for writing");
            return '0';
        }
    }

    file.println("*******************-SET " + String(set) + "-*******************");
    file.println((mode == 1) ? "Age Mode" : "Quality Mode");
    file.print(date);
    file.print(" ");
    file.println(time);
    int counter = 0;
    for (char row = 'A'; row <= 'E'; row++)
    {
        for (int col = 1; col <= 6; col++)
        {
            file.print(row);
            file.print(col);
            file.print(":");
            String value;
            if (mode == qualityMode)
            {
                if (values[counter] < BAD)
                    value = "B";
                else if (values[counter] < POOR)
                    value = "P";
                else
                    value = "G";
            }
            else if (mode == ageMode)
            {
                if (values[counter] < day16to18)
                    value = "16-18";
                else if (values[counter] < day13to15)
                    value = "13-15";
                else if (values[counter] < day10to12)
                    value = "10-12";
                else if (values[counter] < day7to9)
                    value = "17-9 ";
                else if (values[counter] < day4to6)
                    value = "4-6  ";
                else if (values[counter] < day1to3)
                    value = "1-3  ";
            }
            file.print(value);
            file.print("   ");
            counter++;
        }
        file.println();
    }
    // Close the file
    file.close();
    Serial.println("Done");
    set++;
    return '1';
}

// Get result from calculateEgg and send results to arduino and android
void handleGetValues()
{
    String s = "";
    calculateEgg();
    for (int i = 0; i < 30; i++)
    {
        int result;
        if (mode == qualityMode)
        {
            if (values[i] < BAD)
                result = 0;
            else if (values[i] < POOR)
                result = 1;
            else
                result = 2;
        }
        else if (mode == ageMode)
        {
            if (values[i] < day16to18)
                result = 6;
            else if (values[i] < day13to15)
                result = 5;
            else if (values[i] < day10to12)
                result = 4;
            else if (values[i] < day7to9)
                result = 3;
            else if (values[i] < day4to6)
                result = 2;
            else if (values[i] < day1to3)
                result = 1;
        }
        s += String(result);
    }
    // Serial1.flush();
    if (mode == qualityMode)
    {
        Serial1.println(s);
        Serial1.flush();
        Serial.println("QLT:" + s);
        SerialBT.println("QLT:" + s);
        handleGetRaw();
    }
    else if (mode == ageMode)
    {
        Serial1.println(s);
        Serial1.flush();
        Serial.println("AGE:" + s);
        SerialBT.println("AGE:" + s);
        handleGetRaw();
    }
    else
    {
        Serial.println("Error.");
    }
}

// Set-up bluetooth callback
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    if (event == ESP_SPP_SRV_OPEN_EVT)
    {
        if (serialDebug)
            Serial.println("Client Connected!");
        delay(500);
        if (serialDebug)
            Serial.println((mode == 1) ? "CMD:AGEMODE" : "CMD:QLTMODE");
        SerialBT.println((mode == 1) ? "CMD:AGEMODE" : "CMD:QLTMODE");
        handleGetSettings();
    }
    else if (event == ESP_SPP_DATA_IND_EVT)
    {
        if (serialDebug)
            Serial.printf("ESP_SPP_DATA_IND_EVT len=%d, handle=%d\n\n", param->data_ind.len, param->data_ind.handle);

        String command = "";
        for (int i = 0; i < param->data_ind.len; i++)
        {
            command += (char)param->data_ind.data[i];
        }
        command.trim();
        if (serialDebug)
            Serial.printf("BT Received data: %s\n", command);
        if (command.startsWith("GET"))
        {
            // char *get = "GET";
            Serial1.print('G');
        }
        if (command.startsWith("MODE:"))
        {
            if (serialDebug)
                Serial.print("MODE: ");
            if (command.substring(5) == "AGE")
            {
                mode = ageMode;
                Serial1.print('A');
                if (serialDebug)
                    Serial.println("Age Mode");
            }
            else if (command.substring(5) == "QLT")
            {
                mode = qualityMode;
                Serial1.print('Q');
                if (serialDebug)
                    Serial.println("Quality Mode");
            }
        }
        if (command.startsWith("LOGS"))
        {
            Serial.println("Sending logs...");
            Serial1.print('L');
            sendLogsBT("/logs");
        }
        if (command.startsWith("CONF:"))
        {
            if (command.substring(5) == "GET")
            {
                Serial.println("Sending config...");
                handleGetSettings();
            }
            else
            {
                Serial.println("Setting config...");
                handleSetSettings(command.substring(5));
            }
        }
        if (command.startsWith("RAW"))
        {
            handleGetRaw();
        }
        if (command.startsWith("SAVE"))
        {
            Serial1.print('S');
        }
        if (command.startsWith("DEL"))
        {
            Serial.println("Deleteing SD Card files...");
            deleteFiles(SD_MMC, "/", 0);
            Serial.println("Done");
        }
        if (command.startsWith("B"))
        {
            Serial.println("SerialBT Connection check: True");
        }
    }
    else if (event == ESP_SPP_CLOSE_EVT)
    {
        Serial.println("SerialBT Connection check: False");
    }
}

// Send settings to BTSerial
void handleGetSettings()
{
    char s[50];
    snprintf(s, sizeof(s), "CONF:%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",
             BAD,
             POOR,
             camQuality,
             camExposure,
             camBrightness,
             camContrast,
             camGain,
             day1to3,
             day4to6,
             day7to9,
             day10to12,
             day13to15,
             day16to18);

    Serial.println(s);
    SerialBT.println(s);
}

// Send raw intensity values to BTSerial for debugging/calibrating
void handleGetRaw()
{
    char s[150] = "RAW:";
    for (int i = 0; i < 30; i++)
    {
        char valueStr[10];
        itoa(values[i], valueStr, 10);
        strcat(s, valueStr);
        strcat(s, ",");
    }
    Serial.println(s);
    SerialBT.println(s);
}
