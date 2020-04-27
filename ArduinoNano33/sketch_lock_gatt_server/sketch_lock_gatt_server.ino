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
    LogBuggin = 5,
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

// The starting secret code to open the lock
// @TODO add means of updating this value to a personalised value
const char *secretCode = "abc123";

/**
 * Sets up the Arduino, run once before carrying out the loop() function.
 */
void setup() {
    /**
     * BLE related set up code
     */

    // Set up the serial comms port
    Serial.begin(9600);

    // Wait for the serial port to start up
    while (!Serial)
    {

    }
}

void loop() {
  // put your main code here, to run repeatedly:

}
