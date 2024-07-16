#include "main.h"

void setup()
{
    initLCD();
}

void loop()
{
    // Wait for ESP-Cam Serial commands
    while (espSerial.available())
    {
        lc.clearDisplay(0);
        char received = espSerial.read();
        espSerial.flush();
        Serial.print("\nReceived: ");
        Serial.println(received);
        if (received == 'G')
        {
            // Start reading values
            getStatus = handleEggValues();
        }
        if (received == 'A')
        {
            // Change mode to Age
            handleChangeMode(0, ageMode);
        }
        if (received == 'Q')
        {
            // Change mode to Quality
            handleChangeMode(0, qualityMode);
        }
        if (received == 'S')
        {
            // Save to log file
            Save();
            option = 1;
        }
        if (received == 'L')
        {
            // Sending logs to android app
            handleSendLogs();
            option = 1;
        }
        stopDisp = 0;
        stopBlink = 0;
    }

    lastInputTime = millis() - prevTime;
    if (getStatus)
    {
        if ((lastInputTime > 3000 || option == 1) && stopDisp == 0)
        {
            if (lastInputTime > 3000)
                stopBlink = 0;

            stopDisp = 1;
            if (option == 1)
            {
                prevTime = millis();
                option = 0;
            }
            if (mode == qualityMode)
            {
                setRGBLed(0, 0, 0);
                printLCD("Egg Quality", 0, 1, 1);
                printLCD(eggCounters, 1, 1, 1);
            }
            else
            {
                handleEggAge();
            }
        }

        if (millis() - blinkTimer > 500 && mode == qualityMode && stopBlink == 0)
        {
            blinkTimer = millis();
            eggMatrix();
        }
        else if (mode == ageMode)
        {
            lc.setLed(0, dotPosY, dotPosX, true);
        }
    }
    else
    {
        setRGBLed(0, 0, 10);
        printLCD("Press Joystick", 0, 1, 1);
        printLCD("to Start", 1, 1, 1);
        lc.clearDisplay(0);
    }

    if (digitalRead(SWITCH_PIN) == HIGH)
        isHeld = false;

    // Open Menu
    while (digitalRead(SWITCH_PIN) == LOW && !isHeld)
    {
        stopBlink = 1;
        stopDisp = 0;
        newTray = 0;
        prevTime = millis();
        lc.clearDisplay(0);
        while (digitalRead(SWITCH_PIN) == LOW)
        {
            if (millis() - prevTime > 1000 || !getStatus)
            {
                isHeld = true;
                newTray = 1;
                break;
            }
        }
        if (newTray == 1)
        {
            getStatus = handleEggValues();
            break;
        }
        if (mode == qualityMode)
        {
            eggMatrix();
        }
        option = openMenu() + 1;
        switchMenu(option);
        if (option == 4)
        {
            lc.clearDisplay(0);
            // lc.setLed(0, 1, 1, true);
        }
        else if (option == 1)
        {
            stopBlink = 0;
        }
    }

    while (jStick() != 0 && getStatus)
    {
        prevTime = millis();
        stopBlink = 1;
        stopDisp = 0;
        lc.clearDisplay(0);
        int joystickInput = jStick();
        if (lastInputTime > 3000 && mode == qualityMode)
        {
            lc.setLed(0, dotPosY, dotPosX, true);
        }
        else if (joystickInput == right || joystickInput == left || joystickInput == up || joystickInput == down)
        {
            eggMove(joystickInput);
        }
        (mode == qualityMode) ? handleEggQuality() : handleEggAge();
        while (jStick() != 0)
        {
            prevTime = millis();
        }
    }
}