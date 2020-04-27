/**
 * Lock GATT server
 *
 * This Arduino sketch requires either an Arduino with BLE capabilities
 * or a BLE adaptor board.
 *
 * @TODO - Fill in a more detailed description of this file.
 *
 *              Kris Dunning (ippie52@gmail.com) 2020
 */

#define USE_BLE (0)

#if USE_BLE
#include <ArduinoBLE.h>
#endif // #if USE_BLE
#include "NonVol.h"
#include "InputHelper.h"
#include "OutputHelper.h"

/**
 * Enumerations and constants
 */

// Pin mappings
enum Pins
{
    // LEDs
    Locked = 2,
    Unlocked = 3,
    Connected = 4,

    // Switches and buttons
    LogButton = 5,
    ManualOverrideSwitch = 6,
};

/**
 * Global variables
 */

#if USE_BLE
// The main service for the lock
BLEService lockService("F001");

// The characteristics of the lock service
// @TODO Provide some READ and Identify options
// @TODO Explain the UUIDs, read/write etc and the length
BLECharacteristic unlockChar("F002", BLEWrite, 20);
BLECharacteristic statusChar("F003", BLENotify, 20);

// The descriptors for the characteristics
// @TODO explain
BLEDescriptor unlockDesc("2901", "Accepts unlock codes.");
BLEDescriptor statusDesc("2901", "Provides the current status string.");

#endif // #if USE_BLE

// The output helper for writing to and reading from the locked LED pin.
OutputHelper lockedLed(Pins::Locked, HIGH);
// The output helper for writing to and reading from the unlocked LED pin.
OutputHelper unlockedLed(Pins::Unlocked);
// The output helper for writing to and reading from the connected LED pin.
OutputHelper connectedLed(Pins::Connected);

/**
 * @brief   Handler for when the log button is pressed.
 *
 * @param   pin             The input pin triggering the handler.
 * @param   state           The new state of the input.
 * @param   durationMs      The duration of the press/release in milliseconds.
 */
void logButtonHandler(const int pin, const int state, const long durationMs)
{
    if (pin == Pins::LogButton)
    {
        Serial.print("Log button has been ");
        Serial.println(state ? "pressed." : "released.");
        if (!state)
        {
            Serial.print("Button was pressed for ");
            Serial.print(durationMs);
            Serial.println(" milliseconds.");
        }
        connectedLed = state;
    }
    else
    {
        Serial.print("Incorrect button being handled - ");
        Serial.print(pin);
        Serial.print(" is not ");
        Serial.println(Pins::LogButton);
    }
}

/**
 * @brief   Handler for when the manual override switch is toggled.
 *
 * @param   pin             The input pin triggering the handler.
 * @param   state           The new state of the switch.
 * @param   durationMs      The duration of the press/release in milliseconds.
 */
void manOverrideHandler(const int pin, const int state, const long durationMs)
{
    if (pin == Pins::ManualOverrideSwitch)
    {
        Serial.print("Manual override switch has been toggled ");
        Serial.println(state ? "on" : "off.");
        Serial.print("Switch was ");
        Serial.print(state ? "off for " : "on for ");
        Serial.print(durationMs);
        Serial.println(" milliseconds.");

        lockedLed = !state;
        unlockedLed = state;
    }
    else
    {
        Serial.print("Incorrect button being handled - ");
        Serial.print(pin);
        Serial.print(" is not ");
        Serial.println(Pins::ManualOverrideSwitch);
    }
}


// The input helper for handling log button presses.
InputHelper logButton(Pins::LogButton, logButtonHandler);
// The input helper for handling manual override switch toggles.
InputHelper manOverrideSwitch(Pins::ManualOverrideSwitch, manOverrideHandler);

// The starting secret code to open the lock
// @TODO add means of updating this value to a personalised value
const char *secretCode = "BLE-Tutor";

/**
 * @brief   Sets up the Arduino, run once before carrying out the loop()
 *          function.
 */
void setup() {
    /**
     * BLE related set up code
     */

    // Start the serial communications, then wait for it to initialise
    Serial.begin(9600);
    while (!Serial)
    {
        delay(1);
    }

    Serial.println("Starting the GATT server...");
#if USE_BLE

#endif // #if USE_BLE
}

/**
 * @brief   Loop function carried out indefinitely after set up.
 */
void loop() {
  // put your main code here, to run repeatedly:
  logButton.poll();
  manOverrideSwitch.poll();
}
