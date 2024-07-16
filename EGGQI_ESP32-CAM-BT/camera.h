// Camera related
#include "esp_camera.h"

// Used to disable brownout detection
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Default Threshold settings
int BAD = 40;
int POOR = 80;
int day1to3 = 90;
int day4to6 = 75;
int day7to9 = 60;
int day10to12 = 45;
int day13to15 = 30;
int day16to18 = 15;

// Camera adjustable settings
int camExposure = 1200;
int camGain = 10;
int camQuality = 10;
int camBrightness = -2;
int camContrast = 2;

// Serial Debug on/off
const bool serialDebug = 1;

// sd-card
#include "SD_MMC.h"
#include <SPI.h>
#include <FS.h>
#define SD_CS 5
bool sdcardPresent;
bool flashRequired = 0;
const int indicatorLED = 33;
const int brightLED = 4;
int brightLEDbrightness = 0;
const int ledFreq = 5000;
const int ledChannel = 15;
const int ledRresolution = 8;
const int serialSpeed = 115200;
framesize_t FRAME_SIZE_IMAGE = FRAMESIZE_UXGA;
pixformat_t PIXFORMAT = PIXFORMAT_GRAYSCALE;

// Camera pins
#define CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

bool cameraImageSettings();
bool initialiseCamera();
void setupCamera();
void setupFlashPWM();
void brightLed(byte);
void calculateEgg();
void resetCamera(bool);
void flashLED(int);
void writeConfigFile();
void processConfigLine(String line);
void readConfigFile();

// Camera setup
void setupCamera()
{
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    pinMode(indicatorLED, OUTPUT);
    digitalWrite(indicatorLED, HIGH);
    digitalWrite(indicatorLED, LOW);
    Serial.print(("\nInitialising camera: "));
    if (initialiseCamera())
    {
        if (serialDebug)
            Serial.println("OK");
    }
    else
    {
        if (serialDebug)
            Serial.println("failed");
    }
    if (!SD_MMC.begin("/sdcard", true))
    {
        if (serialDebug)
            Serial.println("No SD Card detected");
        sdcardPresent = 0;
    }
    else
    {
        uint8_t cardType = SD_MMC.cardType();
        if (cardType == CARD_NONE)
        {
            if (serialDebug)
                Serial.println("SD Card type detect failed");
            sdcardPresent = 0;
        }
        else
        {
            uint16_t SDfreeSpace = (uint64_t)(SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
            if (serialDebug)
                Serial.printf("SD Card found, free space = %dmB \n", SDfreeSpace);
            sdcardPresent = 1;
        }
    }
    fs::FS &fs = SD_MMC; // sd card file system

    pinMode(indicatorLED, OUTPUT);
    digitalWrite(indicatorLED, HIGH);
    setupFlashPWM();

    // startup complete
    if (serialDebug)
        Serial.println("\nStarted...");
    flashLED(2);
    brightLed(64);
    delay(200);
    brightLed(0);
}

// Initialize Camera module
bool initialiseCamera()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 10000000;       // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    config.pixel_format = PIXFORMAT;      // Options =  YUV422, GRAYSCALE, RGB565, JPEG, RGB888
    config.frame_size = FRAME_SIZE_IMAGE; // Image sizes
    config.jpeg_quality = 10;             // 0-63 lower number means higher quality (can cause failed image capture if set too low at higher resolutions)
    config.fb_count = 1;                  // if more than one, i2s runs in continuous mode. Use only with JPEG

    if (!psramFound())
    {
        if (serialDebug)
            Serial.println("Warning: No PSRam found so defaulting to image size 'CIF'");
        config.frame_size = FRAMESIZE_CIF;
    }

    esp_err_t camerr = esp_camera_init(&config); // initialise the camera
    if (camerr != ESP_OK)
    {
        if (serialDebug)
            Serial.printf("ERROR: Camera init failed with error 0x%x", camerr);
    }

    cameraImageSettings(); // apply custom camera settings

    return (camerr == ESP_OK); // return boolean result of camera initialisation
}

// Set camera settings
bool cameraImageSettings()
{

    if (serialDebug)
        Serial.println("Applying camera settings");

    sensor_t *s = esp_camera_sensor_get();
    if (s == NULL)
    {
        if (serialDebug)
        {
            Serial.println("Error: problem reading camera sensor settings");
            resetCamera(1);
        }
        return 0;
    }
    s->set_gain_ctrl(s, 0);                // auto gain off (1 or 0)
    s->set_exposure_ctrl(s, 0);            // auto exposure off (1 or 0)
    s->set_agc_gain(s, camGain);           // set gain manually (0 - 30)
    s->set_aec_value(s, camExposure);      // set exposure manually  (0-1200)
    s->set_vflip(s, 1);                    // Invert image (0 or 1)
    s->set_quality(s, camQuality);         // (0 - 63)
    s->set_gainceiling(s, GAINCEILING_2X); // Image gain (GAINCEILING_x2, x4, x8, x16, x32, x64 or x128)
    s->set_brightness(s, camBrightness);   // (-2 to 2) - set brightness
    s->set_lenc(s, 0);                     // lens correction? (1 or 0)
    s->set_saturation(s, 0);               // (-2 to 2)
    s->set_contrast(s, camContrast);       // (-2 to 2)
    s->set_sharpness(s, 0);                // (-2 to 2)
    s->set_hmirror(s, 1);                  // (0 or 1) flip horizontally
    s->set_colorbar(s, 0);                 // (0 or 1) - show a testcard
    s->set_special_effect(s, 0);           // (0 to 6?) apply special effect
    s->set_whitebal(s, 0);                 // white balance enable (0 or 1)
    s->set_awb_gain(s, 0);                 // Auto White Balance enable (0 or 1)
    s->set_wb_mode(s, 0);                  // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    s->set_dcw(s, 0);                      // downsize enable? (1 or 0)?
    s->set_raw_gma(s, 0);                  // (1 or 0)
    s->set_aec2(s, 0);                     // automatic exposure sensor?  (0 or 1)
    s->set_ae_level(s, 0);                 // auto exposure levels (-2 to 2)
    s->set_bpc(s, 0);                      // black pixel correction
    s->set_wpc(s, 1);                      // white pixel correction

    return 1;
} // cameraImageSettings

void setupFlashPWM()
{
    ledcSetup(ledChannel, ledFreq, ledRresolution);
    ledcAttachPin(brightLED, ledChannel);
    brightLed(brightLEDbrightness);
}

// change illumination LED brightness
void brightLed(byte ledBrightness)
{
    brightLEDbrightness = ledBrightness;  // store setting
    ledcWrite(ledChannel, ledBrightness); // change LED brightness (0 - 255)
    // if (serialDebug)
    //     Serial.println("Brightness changed to " + String(ledBrightness));
}

// Flash LED
void flashLED(int reps)
{
    for (int x = 0; x < reps; x++)
    {
        digitalWrite(indicatorLED, LOW);
        delay(1000);
        digitalWrite(indicatorLED, HIGH);
        delay(500);
    }
}

int values[30];

// Calculate intensity of eggs from tray
void calculateEgg()
{
    if (PIXFORMAT != PIXFORMAT_GRAYSCALE)
    {
        PIXFORMAT = PIXFORMAT_GRAYSCALE;
        resetCamera(0);
    }
    camera_fb_t *fb = esp_camera_fb_get(); // capture image frame from camera
    if (!fb)
    {
        if (serialDebug)
            Serial.println("Error: Camera capture failed");
        resetCamera(1);
        return;
    }
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();
    if (!fb)
    {
        if (serialDebug)
            Serial.println("Error: Camera capture failed");
        resetCamera(1);
        return;
    }
    esp_camera_fb_return(fb);
    unsigned int gridWidth = 6;
    unsigned int gridHeight = 5;
    unsigned int subImageWidth = fb->width / gridWidth;
    unsigned int subImageHeight = fb->height / gridHeight;
    unsigned int subImageSize = subImageWidth * subImageHeight;
    unsigned int numImages = gridWidth * gridHeight;

    int counter = 0;
    if (serialDebug)
    {
        Serial.print("Image Dimension: ");
        Serial.print(subImageWidth);
        Serial.print("x");
        Serial.println(subImageHeight);
    }

    // Calculate intensity for each eggs in 6 x 5 grid
    for (unsigned int y = 0; y < gridHeight; y++)
    {
        for (unsigned int x = 0; x < gridWidth; x++)
        {
            unsigned int startX = x * subImageWidth;
            unsigned int startY = y * subImageHeight;
            unsigned long intensitySum = 0;

            for (unsigned int row = startY; row < startY + subImageHeight; row++)
            {
                for (unsigned int col = startX; col < startX + subImageWidth; col++)
                {
                    unsigned int pixelIndex = row * fb->width + col;
                    uint8_t pixelValue = fb->buf[pixelIndex];
                    intensitySum += pixelValue;
                }
            }
            float averageIntensity = intensitySum / subImageSize;
            float normalizedIntensity = (averageIntensity / 255) * 100;
            values[counter] = normalizedIntensity;
            counter++;
        }
    }
    counter = 0;
    if (serialDebug)
    {
        for (char row = 'A'; row <= 'E'; row++)
        {
            for (int col = 1; col <= 6; col++)
            {
                Serial.print(row);
                Serial.print(col);
                Serial.print(":");
                Serial.printf("%d\t", values[counter]);
                // Serial.print(" ");
                counter++;
            }
            Serial.println("");
        }
    }
    esp_camera_fb_return(fb); // return frame so memory can be released
    return;
}

// Reset camera
void resetCamera(bool type = 0)
{
    if (type == 1)
    {
        // power cycle the camera module (handy if camera stops responding)
        digitalWrite(PWDN_GPIO_NUM, HIGH); // turn power off to camera module
        delay(300);
        digitalWrite(PWDN_GPIO_NUM, LOW);
        delay(300);
        initialiseCamera();
    }
    else
    {
        // reset via software (handy if you wish to change resolution or image type etc. - see test procedure)
        esp_camera_deinit();
        delay(50);
        initialiseCamera();
    }
}

// Read config file from Micro SD Card
void readConfigFile()
{
    File configFile = SD_MMC.open("/config.txt");
    if (configFile)
    {
        while (configFile.available())
        {
            String line = configFile.readStringUntil('\n');
            // Parse the line and set the values in the ESP32
            processConfigLine(line);
        }
        configFile.close();
        Serial.println("");
        Serial.println("-------Config-------");
        Serial.print("Bad Threshold:");
        Serial.println(BAD);
        Serial.print("Poor Threshold:");
        Serial.println(POOR);
        Serial.print("Quality:");
        Serial.println(camQuality);
        Serial.print("Exposure:");
        Serial.println(camExposure);
        Serial.print("Brightness:");
        Serial.println(camBrightness);
        Serial.print("Contrast:");
        Serial.println(camContrast);
        Serial.print("Gain:");
        Serial.println(camGain);
        Serial.print("day1to3:");
        Serial.println(day1to3);
        Serial.print("day4to6:");
        Serial.println(day4to6);
        Serial.print("day7to9:");
        Serial.println(day7to9);
        Serial.print("day10to12:");
        Serial.println(day10to12);
        Serial.print("day13to15:");
        Serial.println(day13to15);
        Serial.print("day16to18:");
        Serial.println(day16to18);
        Serial.println("--------------------");
    }
    else
    {
        Serial.println("Failed to open config file");
        writeConfigFile();
    }
}

// Read config line from config file
void processConfigLine(String line)
{
    // Parse and set the values from the config line in "key=value" format
    int separatorIndex = line.indexOf('=');
    if (separatorIndex != -1)
    {
        String key = line.substring(0, separatorIndex);
        String value = line.substring(separatorIndex + 1);
        // Set the values in the ESP32 based on the key
        if (key.equals("badThreshold"))
        {
            BAD = value.toInt();
        }
        else if (key.equals("poorThreshold"))
        {
            POOR = value.toInt();
        }
        else if (key.equals("quality"))
        {
            camQuality = value.toInt();
        }
        else if (key.equals("exposure"))
        {
            camExposure = value.toInt();
        }
        else if (key.equals("brightness"))
        {
            camBrightness = value.toInt();
        }
        else if (key.equals("contrast"))
        {
            camContrast = value.toInt();
        }
        else if (key.equals("gain"))
        {
            camGain = value.toInt();
        }
        else if (key.equals("day1to3"))
        {
            day1to3 = value.toInt();
        }
        else if (key.equals("day4to6"))
        {
            day4to6 = value.toInt();
        }
        else if (key.equals("day7to9"))
        {
            day7to9 = value.toInt();
        }
        else if (key.equals("day10to12"))
        {
            day10to12 = value.toInt();
        }
        else if (key.equals("day13to15"))
        {
            day13to15 = value.toInt();
        }
        else if (key.equals("day16to18"))
        {
            day16to18 = value.toInt();
        }
        // Add more cases for other parameters as needed
    }
}

// Write/Edit config file from Micro SD Card
void writeConfigFile()
{
    File configFile = SD_MMC.open("/config.txt", FILE_WRITE);
    if (configFile)
    {
        configFile.println("badThreshold=" + String(BAD));
        configFile.println("poorThreshold=" + String(POOR));
        configFile.println("quality=" + String(camQuality));
        configFile.println("exposure=" + String(camExposure));
        configFile.println("brightness=" + String(camBrightness));
        configFile.println("contrast=" + String(camContrast));
        configFile.println("gain=" + String(camGain));
        configFile.println("day1to3=" + String(day1to3));
        configFile.println("day4to6=" + String(day4to6));
        configFile.println("day7to9=" + String(day7to9));
        configFile.println("day10to12=" + String(day10to12));
        configFile.println("day13to15=" + String(day13to15));
        configFile.println("day16to18=" + String(day16to18));
        configFile.close();
        Serial.println("Config file updated");
    }
    else
    {
        Serial.println("Failed to open config file for writing");
    }
}

// Set settings sent by Android app
void handleSetSettings(String subString)
{
    int values[13];
    for (int i = 0; i < 13; i++)
    {
        int commaIndex = subString.indexOf(',');
        if (commaIndex != -1)
        {
            String valueStr = subString.substring(0, commaIndex);
            values[i] = valueStr.toInt();
            subString = subString.substring(commaIndex + 1);
        }
        else
        {
            values[i] = subString.toInt();
            break;
        }
    }
    BAD = values[0];
    POOR = values[1];
    camQuality = values[2];
    camExposure = values[3];
    camBrightness = values[4];
    camContrast = values[5];
    camGain = values[6];
    day1to3 = values[7];
    day4to6 = values[8];
    day7to9 = values[0];
    day10to12 = values[10];
    day13to15 = values[11];
    day16to18 = values[12];
    Serial.println("badThreshold=" + String(BAD));
    Serial.println("poorThreshold=" + String(POOR));
    Serial.println("quality=" + String(camQuality));
    Serial.println("exposure=" + String(camExposure));
    Serial.println("brightness=" + String(camBrightness));
    Serial.println("contrast=" + String(camContrast));
    Serial.println("gain=" + String(camGain));
    Serial.println("day1to3=" + String(day1to3));
    Serial.println("day4to6=" + String(day4to6));
    Serial.println("day7to9=" + String(day7to9));
    Serial.println("day10to12=" + String(day10to12));
    Serial.println("day13to15=" + String(day13to15));
    Serial.println("day16to18=" + String(day16to18));
}