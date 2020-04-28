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

#include "InputHelper.h"
#include "OutputHelper.h"
#include "Lock.h"

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
 * Forward declarations
 */

void logButtonHandler(const int pin, const int state, const long durationMs);
void lockStateChange(const bool state);

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

// The starting secret code to open the lock
// @TODO add means of updating this value to a personalised value
const char *secretCode = "BLE-Tutor";

// The output helper for writing to and reading from the locked LED pin.
OutputHelper lockedLed(Pins::Locked, HIGH);
// The output helper for writing to and reading from the unlocked LED pin.
OutputHelper unlockedLed(Pins::Unlocked);
// The output helper for writing to and reading from the connected LED pin.
OutputHelper connectedLed(Pins::Connected);

// The input helper for handling log button presses.
InputHelper logButton(Pins::LogButton, logButtonHandler);

// The lock item
Lock lock(Pins::Locked, Pins::Unlocked, Pins::ManualOverrideSwitch, lockStateChange);

/**
 * Functions
 */


/**
 * @brief   Handler for when the log button is pressed.
 *
 * @param   pin             The input pin triggering the handler.
 * @param   state           The new state of the input.
 * @param   durationMs      The duration of the press/release in milliseconds.
 */
static void logButtonHandler(const int pin, const int state, const long durationMs)
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

#if USE_BLE
/**
 * @brief   Handler for data written to the unlock characteristic.
 *
 * @param   central         The central BLE device information
 * @param   characteristic  The characteristic attached to this handler
 */
static void unlockMessageWritten(BLEDevice central, BLECharacteristic characteristic)
{
    Serial.print("Message received from: ");
    Serial.println(centra.address());

    char* message = (char *)characteristic.value();
    const int len = characteristic.valueLength();
    message[len] = '\0';

    if (strcmp(message, secretCode) == 0)
    {
        Serial.println("Valid code received - unlocking");
        lock.unlock();
    }
    characteristic.writeValue((byte)0);
}
#endif // #if USE_BLE

/**
 * @brief   Lock state change handler function. Reports any changes to the
 *          locked state characteristic.
 *
 * @param   state   The current state of the lock
 */
void lockStateChange(const bool state)
{
#if USE_BLE
    statusChar.setValue(state ? "Locked" : "Unlocked");
#endif // #if USE_BLE
}

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

    Serial.print("Starting the GATT server.");
#if USE_BLE

    // Configure the BLE server
    BLE.setDeviceName("BLE-Tutor Device");
    BLE.setLocalName("BLE-Tutor Arduino");

    // Add descriptors to the characteristics
    unlockChar.addDescriptor(unlockDesc);
    statusChar.addDescriptor(statusDesc);

    unlockChar.setEventHandler(BLEWritten, unlockMessageWritten);

#endif // #if USE_BLE
}

/**
 * @brief   Loop function carried out indefinitely after set up.
 */
void loop() {
  logButton.poll();
  lock.poll();
}
