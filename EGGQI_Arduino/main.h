#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <LedControl.h>
#include <SoftwareSerial.h>
#include "time.h"

// Pin Definitions
#define DIN_PIN 12
#define CS_PIN 7
#define CLK_PIN 6
#define PIN_RED 11
#define PIN_GREEN 10
#define PIN_BLUE 13
#define SWITCH_PIN 3
#define X_JOYSTICK_PIN A0
#define Y_JOYSTICK_PIN A1

// Display and Communication
LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, 1);
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial espSerial(8, 9); // RX, TX

// GlobalVariables
// Initial LCD cursor and Matrix dot position
int cursorX = 0;
int cursorY = 0;
int dotPosX = 1;
int dotPosY = 1;

// Previous LCD contents
char prevDisplayX[16] = "";
char prevDisplayY[16] = "";

// Millis
unsigned long currentTime = 0;
long int prevTime = millis();
long int blinkTimer = millis();
long int lastInputTime = millis() - prevTime;

// loading dots character
char *prevDots;

// LCD menu open boolean
bool menuIsOpen = false;

// Joystick directions
#define up 1
#define down 3
#define left 4
#define right 6

// Joystick button
bool isHeld = false;

// Egg Identification mode
int mode = 0; //
#define qualityMode 0
#define ageMode 1
bool getStatus = false;

// Egg quality identification and counter
#define goodEgg 2
#define poorEgg 1
#define badEgg 0

// String holder for egg counters (G:00 P:00 B:00)
char eggCounters[16];

// BT Connected
bool isConnected = 0;

// Blinking matrix on/off
bool matBlink = 0;
int stopDisp = 0;
int stopBlink = 0;
int newTray = 0;
int option = 0;

// Structures
struct EggValues
{
    char pos[7];
    int value;
};
EggValues eggValues[5][6];

// Function declaration
void initLCD();
void rstCursor();
void rstDot();
void printLCD(const char *s, int y = 0, bool middle = true, bool clr = true);
void loadDots(const char *s, int y, bool mid, bool clr);
// void printTime();
int jStick();
void moveCursor(int deltaX, int deltaY);
void Save();
void Mode();
void Pair();
void menuNav();
int openMenu();
void switchMenu(int n);
void titleScreen();
void handleChangeMode(bool sr = 0, int n = 0);
void setRGBLed(int r, int g, int b);
void eggMove(int x);
void moveDot(int deltaX, int deltaY);
void eggMatrix();
void handleEggAge();
void handleEggQuality();
bool handleEggValues();

// Initialize LCD and Serial communication
void initLCD()
{
    espSerial.begin(115200); // ESP32 baud rate
    Serial.begin(115200);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    titleScreen();
    // lcd.setCursor(1, 0);
    // lcd.print("Press Joystick");
    // lcd.setCursor(4, 1);
    // lcd.print("to Start");
    // while (digitalRead(SWITCH_PIN) == HIGH)
    //     ;
    // while (digitalRead(SWITCH_PIN) == LOW)
    //     ;
    // lcd.clear();
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(PIN_RED, OUTPUT);
    pinMode(PIN_GREEN, OUTPUT);
    pinMode(PIN_BLUE, OUTPUT);
    lc.shutdown(0, false);
    lc.setIntensity(0, 1);
    lc.clearDisplay(0);
}

// Display Egg Quality Identifier Title Screen
void titleScreen()
{
    lcd.clear();
    String title1 = "EGG  QUALITY";
    String title2 = "IDENTIFIER";
    for (int i = 0; i < title1.length(); i++)
    {
        lcd.setCursor(i + 2, 0);
        lcd.print(title1[i]);
        if (title1[i] != ' ')
            delay(200);
    }
    for (int i = 0; i < title2.length(); i++)
    {
        lcd.setCursor(i + 3, 1);
        lcd.print(title2[i]);
        delay(100);
    }
    delay(3000);
    lcd.clear();
}

// Reset menu cursor
void rstCursor()
{
    cursorX = 0;
    cursorY = 0;
}

// Reset Matrix dot cursor
void rstDot()
{
    dotPosX = 1;
    dotPosY = 1;
}

// Function for printing in LCD
void printLCD(const char *s, int y = 0, bool middle = true, bool clr = true)
{

    char *prevDisplay = (y == 0) ? prevDisplayX : prevDisplayY;
    if (strcmp(s, prevDisplay) != 0)
    {
        strcpy(prevDisplay, s);

        if (clr)
        {
            lcd.setCursor(0, y);
            lcd.print("                ");
        }

        int mid = 8 - (strlen(s) / 2);
        if (strlen(s) % 2 != 0)
        {
            mid--;
        }
        lcd.setCursor((middle) ? mid : 0, y);
        lcd.print(s);
    }
    // delete[] prevDisplay;
}

// Function for printing in LCD with loading dots
void loadDots(const char *s, int y, bool mid, bool clr)
{
    if (millis() - currentTime > 500)
    {
        currentTime = millis();
        if (prevDots == "   ")
            prevDots = ".  ";
        else if (prevDots == ".  ")
            prevDots = ".. ";
        else if (prevDots == ".. ")
            prevDots = "...";
        else
            prevDots = "   ";
    }

    // Calculate the x position for the text
    int x = (mid) ? 8 - (strlen(s) + strlen(prevDots)) / 2 : 0;

    // Print the text
    printLCD(s, y, true, clr);

    // Print the dots
    lcd.setCursor(x + strlen(s) + 1, y);
    lcd.print(prevDots);
}

// Read Joystick value/direction
int jStick()
{
    int xValue = analogRead(X_JOYSTICK_PIN); // Read X-axis analog value
    int yValue = analogRead(Y_JOYSTICK_PIN); // Read Y-axis analog value
    int arrow = 0;
    if (xValue < 340)
        arrow = left;
    else if (xValue > 680)
        arrow = right;
    else if (yValue < 340)
        arrow = up;
    else if (yValue > 680)
        arrow = down;
    return arrow;
}

// Move LCD menu cursor
void moveCursor(int deltaX, int deltaY)
{
    // Erase the cursor at the current position
    lcd.setCursor(cursorX, cursorY);
    lcd.print(" ");

    // Update cursor position
    cursorX += deltaX;
    cursorY += deltaY;

    // Keep cursor within menu bounds
    cursorX = constrain(cursorX, 0, 8);
    cursorY = constrain(cursorY, 0, 1);

    // Display cursor at the new position
    lcd.setCursor(cursorX, cursorY);
    lcd.print(">");
}

// Save to txt file and send date from RTC module to ESP32
void Save()
{
    prevDisplayX[0] = '\0';
    prevDisplayY[0] = '\0';
    Serial.println("Saving...");
    Serial.println(getTime());
    espSerial.print("SAVE:");
    espSerial.println(getTime());
    lcd.clear();
    printLCD("Saving...", 0, 1, 1);
    // Wait for ESP response
    int long startTime = millis();
    while (millis() - startTime < 2000)
    {
        loadDots("Saving", 0, 1, 1);
        if (espSerial.available())
        {
            char s = espSerial.read();
            if (s == '1')
            {
                Serial.println("Saved");
                printLCD("Saved", 0, 1, 1);
                delay(2000);
                return;
            }
            else if (s == '0')
            {
                Serial.println("Error saving!");
                printLCD("Error saving!", 0, 1, 1);
                delay(2000);
                return;
            }
        }
    }
    Serial.println("Error saving!");
    printLCD("Error saving!", 0, 1, 1);
    delay(2000);
    return;
}

// Menu for changing mode
void Mode()
{
    menuIsOpen = true;
    rstCursor();
    lcd.clear();
    lcd.setCursor(cursorX, cursorY);
    lcd.print(">");
    lcd.setCursor(1, 0);
    lcd.print("Quality Mode");
    lcd.setCursor(1, 1);
    lcd.print("Age Mode");
    while (digitalRead(SWITCH_PIN) == LOW)
        ;
    while (menuIsOpen)
    {
        delay(100);
        if (jStick() == up)
        {
            moveCursor(0, -1);
        }
        else if (jStick() == down)
        {
            moveCursor(0, 1);
        }
        while (jStick() != 0)
            ;
        if (digitalRead(SWITCH_PIN) == LOW)
        {
            handleChangeMode(1);
        }
    }
}

// Mode change handler
void handleChangeMode(bool sr = 0, int n = 0)
{
    if (sr)
    {
        mode = cursorY;
        if (mode == ageMode)
        {
            espSerial.println("AGEMODE");
        }
        else if (mode == qualityMode)
        {
            espSerial.println("QLTMODE");
        }
    }
    else
        mode = n;
    getStatus = false;
    Serial.print("Set to ");
    Serial.println((mode == ageMode) ? "Age Mode" : "Quality Mode");
    printLCD("Set Identifier", 0, 1, 1);
    mode == 0 ? printLCD("to Quality Mode", 1, 1, 1) : printLCD("to Age Mode", 1, 1, 1);
    delay(2000);
    option = 1;
    menuIsOpen = false;
    rstDot();
}

// Function for Menu navigation with Joystick
void menuNav()
{
    if (jStick() == right)
    {
        moveCursor(8, 0);
    }
    else if (jStick() == left)
    {
        moveCursor(-8, 0);
    }
    else if (jStick() == up)
    {
        moveCursor(0, -1);
    }
    else if (jStick() == down)
    {
        moveCursor(0, 1);
    }
    while (jStick() != 0)
        ;
}

// Display menu in LCD
int openMenu()
{
    prevDisplayX[0] = '\0';
    prevDisplayY[0] = '\0';
    menuIsOpen = true;
    rstCursor();
    int option = 0;
    lcd.clear();
    lcd.setCursor(cursorX, cursorY);
    lcd.print(">");
    lcd.setCursor(1, 0);
    lcd.print("Back");
    lcd.setCursor(1, 1);
    lcd.print("Save");
    lcd.setCursor(9, 0);
    lcd.print("Pair");
    lcd.setCursor(9, 1);
    lcd.print("Mode");
    while (menuIsOpen)
    {
        delay(100);
        menuNav();
        if (digitalRead(SWITCH_PIN) == LOW)
        {
            option = ((cursorX == 0) ? 0 : 1) + ((cursorY == 0) ? 0 : 2);
            menuIsOpen = false;
            delay(100);
            while (digitalRead(SWITCH_PIN) == LOW)
                ;
        }
    }
    return option;
}

// Process selected option in menu
void switchMenu(int n)
{
    switch (n)
    {
    case 1: // Back
        prevDisplayX[0] = '\0';
        prevDisplayY[0] = '\0';
        Serial.println("Back pressed");
        return;
    case 2: // Pair BT
        Serial.println("Pair pressed");
        Pair();
        break;
    case 3: // Save
        Serial.println("Save pressed");
        Save();
        break;
    case 4: // Mode
        Serial.println("Mode pressed");
        Mode();
        break;
    default:
        printLCD("Error!", 0, 1, 1);
        Serial.println("Error!");
        break;
    }
    // delay(3000);
    lcd.clear();
}

// Function to change RGB light
void setRGBLed(int r, int g, int b)
{
    analogWrite(PIN_RED, r);
    analogWrite(PIN_GREEN, g);
    analogWrite(PIN_BLUE, b);
}

// Move Matrix dot cursor with Joystick value
void eggMove(int x)
{
    switch (x)
    {
    case right:
        moveDot(1, 0);
        break;
    case left:
        moveDot(-1, 0);
        break;
    case up:
        moveDot(0, -1);
        break;
    case down:
        moveDot(0, 1);
        break;
        // Add more cases as needed
    }
}

// Function to move dot used in eggMove
void moveDot(int deltaX, int deltaY)
{
    lc.setLed(0, dotPosY, dotPosX, false);
    // Update egg position
    dotPosX += deltaX;
    dotPosY += deltaY;
    // Keep cursor within menu bounds
    dotPosX = (dotPosX < 1) ? 6 : (dotPosX > 6) ? 1
                                                : dotPosX;
    dotPosY = (dotPosY < 1) ? 5 : (dotPosY > 5) ? 1
                                                : dotPosY;
    // Display cursor at the new position
    lc.setLed(0, dotPosY, dotPosX, true);
}

// Function for Matrix LCD to display Egg quality
void eggMatrix()
{
    lc.clearDisplay(0);
    matBlink = !matBlink;
    for (int row = 0; row < 5; row++)
    {
        for (int col = 0; col < 6; col++)
        {
            int val = eggValues[row][col].value;
            if (val == badEgg)
            {
                // Turn on light if BAD
                lc.setLed(0, row + 1, col + 1, true);
            }
            else if (val == poorEgg)
            {
                // Blink light if POOR
                lc.setLed(0, row + 1, col + 1, matBlink);
            }
            else
            {
                // Turn off light if GOOD
                lc.setLed(0, row + 1, col + 1, false);
            }
        }
    }
}

// Display Egg Age in LCD
void handleEggAge()
{
    int eggAge = eggValues[dotPosY - 1][dotPosX - 1].value;
    // Age Identification
    char *ageDays[6] = {"Day 1-3", "Day 4-6", "Day 7-9", "Day 10-12", "Day 13-15", "Day 16-18"};
    printLCD(eggValues[dotPosY - 1][dotPosX - 1].pos, 0, true, true);
    printLCD(ageDays[eggAge - 1], 1, true, true);
}

// Display Egg Quality in LCD
void handleEggQuality()
{
    printLCD(eggValues[dotPosY - 1][dotPosX - 1].pos, 0, true, true);
    int eggvalue = eggValues[dotPosY - 1][dotPosX - 1].value;
    // Quality Identification
    if (eggvalue == badEgg)
    {
        printLCD("BAD", 1, true, true);
        setRGBLed(10, 0, 0);
    }
    else if (eggvalue == poorEgg)
    {
        printLCD("POOR", 1, true, true);
        setRGBLed(10, 5, 0);
    }
    else
    {
        printLCD("GOOD", 1, true, true);
        setRGBLed(0, 10, 0);
    }
}

// Send Get command to ESP32 and receive Egg Quality/Age values
bool handleEggValues()
{
    Serial.println("\nProcessing...");

    // String holder from serial
    char received[31];
    bool done = false;
    option = 1;
    int goodCounter = 0;
    int poorCounter = 0;
    int badCounter = 0;
    long int startTime = millis();
    printLCD("Processing", 0, 1, 1);
    espSerial.println("GET");
    while (millis() - startTime < 10000 && !done)
    {
        if (millis() - startTime < 2000)
        {
            loadDots("Egg Tray", 1, 1, 1);
        }
        else
        {
            while (espSerial.available())
            {
                char c = espSerial.read();
                if (c == '\n')
                {
                    received[30] = '\0';
                    Serial.print("\nReceived: ");
                    Serial.println(received);
                    Serial.print("in ");
                    Serial.print(millis() - startTime);
                    Serial.println("ms");
                    done = true;
                    break;
                }
                else
                {
                    if (c == '`')
                        c = '0';
                    else if (c == 'a')
                        c = '1';
                    else if (c == 'b')
                        c = '2';
                    else if (c == 'c')
                        c = '3';
                    else if (c == 'd')
                        c = '4';
                    else if (c == 'e')
                        c = '5';
                    else if (c == 'f')
                        c = '6';
                    int len = strlen(received);
                    sprintf(received + len, "%c", c);
                }
            }
        }
    }
    if (millis() - startTime > 10000 || strlen(received) != 30)
    {
        if (strlen(received) != 30)
        {
            Serial.println("Data length mismatch.");
            Serial.println(strlen(received));
        }
        else
        {
            Serial.println("Read serial timeout.");
        }
        printLCD("Error Reading", 0, 1, 1);
        printLCD("Values!", 1, 1, 1);
        delay(2000);
        return false;
    }
    else
    {
        lcd.clear();
        printLCD("Done!", 0, 1, 1);
        Serial.print("Length: ");
        Serial.println(strlen(received));
        delay(2000);
    }

    int count = 0;
    for (char row = 'A'; row <= 'E'; row++)
    {
        for (int col = 0; col < 6; col++)
        {
            snprintf(eggValues[row - 'A'][col].pos, sizeof(eggValues[row - 'A'][col].pos), "EGG %c%d", row, col + 1);
            eggValues[row - 'A'][col].value = received[count] - '0';
            if (eggValues[row - 'A'][col].value == goodEgg)
                goodCounter++;
            else if (eggValues[row - 'A'][col].value == poorEgg)
                poorCounter++;
            else
                badCounter++;
            count++;
        }
    }
    for (int row = 0; row < 5; row++)
    {
        for (int col = 0; col < 6; col++)
        {
            Serial.print(eggValues[row][col].pos[4]);
            Serial.print(eggValues[row][col].pos[5]);
            Serial.print(":");
            Serial.print(eggValues[row][col].value);
            Serial.print("  ");
        }
        Serial.print("\n");
    }
    if (mode == qualityMode)
    {
        Serial.println("Egg Counter:");
        sprintf(eggCounters, "G:%02d P:%02d B:%02d", goodCounter, poorCounter, badCounter);
        Serial.println(eggCounters);
    }
    rstDot();
    return true;
}

// Wait for esp32 to send all logs to android app
void handleSendLogs()
{
    lcd.clear();
    printLCD("Syncing");
    printLCD("logs...", 1);
    int long startTime = millis();
    while (millis() - startTime < 20000)
    {
        loadDots("logs", 1, 1, 1);
        if (espSerial.available())
        {
            char c = espSerial.read();
            if (c == '1')
            {
                lcd.clear();
                printLCD("Done!");
                delay(2000);
                return;
            }
            else if (c == '0')
            {
                break;
            }
        }
    }
    lcd.clear();
    printLCD("Error!");
    delay(1000);
}