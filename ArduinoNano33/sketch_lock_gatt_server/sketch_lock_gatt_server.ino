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

#include <ArduinoBLE.h>
#include "InputHelper.h"
#include "OutputHelper.h"
#include "Lock.h"

/**
 * Enumerations and constants
 */

#define LOCKED_STRING       "Locked"
#define UNLOCKED_STRING     "Unlocked"

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

static void logButtonHandler(const int pin, const int state, const long durationMs);
static void lockStateChange(const bool state);

/**
 * Global variables
 */

// The main service for the lock
BLEService lockService("F001");

// The characteristics of the lock service
// @TODO Provide some READ and Identify options
// @TODO Explain the UUIDs, read/write etc and the length
BLECharacteristic unlockChar("F002", BLEWrite, 20);
BLECharacteristic statusChar("F003", BLENotify | BLERead, 20);

// The descriptors for the characteristics
// @TODO explain
BLEDescriptor unlockDesc("2901", "Accepts unlock codes.");
BLEDescriptor statusDesc("2901", "Provides the current status string.");


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

/**
 * @brief   Handler for data written to the unlock characteristic.
 *
 * @param   central         The central BLE device information
 * @param   characteristic  The characteristic attached to this handler
 */
static void unlockMessageWritten(BLEDevice central, BLECharacteristic characteristic)
{
    Serial.print("Message received from: ");
    Serial.println(central.address());

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

/**
 * @brief   Handles a new connection from a central device
 *
 * @param   central     The central device information
 */
static void connectedHandler(BLEDevice central)
{
  Serial.print("Client connected: ");
  Serial.println(central.address());
  connectedLed = HIGH;
}

/**
 * @brief   Handles disconnection from a central device
 *
 * @param   central     The central device information
 */
static void disconnectedHandler(BLEDevice central)
{
  Serial.print("Client disconnected: ");
  Serial.println(central.address());
  connectedLed = LOW;
}

/**
 * @brief   Lock state change handler function. Reports any changes to the
 *          locked state characteristic.
 *
 * @param   state   The current state of the lock
 */
static void lockStateChange(const bool state)
{
    statusChar.setValue(state ? LOCKED_STRING : UNLOCKED_STRING);
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

    Serial.println("Starting the GATT server.");

    if (!BLE.begin())
    {
        Serial.println("Failed to start BLE");
        while (true)
        {
            delay(100);
        }
    }

    // Configure the BLE server
    BLE.setDeviceName("BLE-Tutor Device");
    BLE.setLocalName("BLE-Tutor Arduino");

    // Add descriptors to the characteristics
    unlockChar.addDescriptor(unlockDesc);
    statusChar.addDescriptor(statusDesc);

    unlockChar.setEventHandler(BLEWritten, unlockMessageWritten);
    statusChar.setValue(LOCKED_STRING);

    lockService.addCharacteristic(unlockChar);
    lockService.addCharacteristic(statusChar);

    BLE.setEventHandler(BLEConnected, connectedHandler);
    BLE.setEventHandler(BLEDisconnected, disconnectedHandler);

    BLE.addService(lockService);
    BLE.advertise();
    Serial.print("Server stated on: ");
    Serial.println(BLE.address());
}

/**
 * @brief   Loop function carried out indefinitely after set up.
 */
void loop() {
    BLE.poll();
    logButton.poll();
    lock.poll();
}
