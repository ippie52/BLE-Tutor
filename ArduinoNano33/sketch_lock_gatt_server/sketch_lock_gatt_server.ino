/**
 * @file sketch_lock_gatt_server.ino
 *
 * @brief Arduino sketch to control a BLE peripheral lock using a central device
 *
 * This sketch emulates a lock with BLE connectivity, with the aim of exploring
 * a BLE peripheral server capabilities, including services, characteristics and
 * descriptors, as well as their properties.
 *
 * The code is heavily documented to give a better understanding of as much of
 * the operation as possible. As much code unrelated to BLE has been pushed into
 * other files to make this file the focus of anyone wishing to learn more about
 * setting up a BLE peripheral on an Arduino board.
 *
 * This Arduino sketch requires either an Arduino with BLE capabilities
 * or a BLE adaptor board.
 *
 * @author  Kris Dunning (ippie52@gmail.com)
 * @data    2020
 */

// This uses the ArduinoBLE header, which needs to be installed as an additional
// library. Go to "Tools" > "Manage Libraries", then search for ArduinoBLE.
#include <ArduinoBLE.h>
// The Arduino Nano 33 IoT/BLE does not come with any EEPROM, so SRAM must be
// used. The FlashStorage library provides this. As with the ArduinoBLE library,
// go to "Tools" > "Manage Libraries" then search for FlashStorage.
#include <FlashStorage.h>
// These are helper classes written specifically for this sketch.
#include "Common.h"
#include "InputHelper.h"
#include "Lock.h"
#include "OutputHelper.h"

/**
 * Enumerations, structures and constants
 */

/// @brief  The string to display when locked
#define LOCKED_STRING                       "Locked"
/// @brief  The string to display when unlocked
#define UNLOCKED_STRING                     "Unlocked"

/// @brief  Pin mappings
enum Pins
{
    // LEDs
    LockedLed = 2,
    UnlockedLed = 3,
    ConnectedLed = 4,

    // Switches and buttons
    MultiPurpBtn = 5,
    ManualOverrideSwitch = 6,
};

/**
 * Forward declarations
 */
static void lockStateChange(const LockState state);
static void logMessageHandler(const bool full);

/**
 * Global variables
 */

/// @brief  The main service for the lock. This service will provide all the
///         characteristics to modify and monitor the lock. Here we have
///         provided the service with the 16-bit UUID, which will be expanded
///         to a 128-bit UUID.
BLEService lockService("F001");

// Characteristic information:
// The characteristics of the lock service listed below are given 16-bit UUIDs
// which will be expanded to 128-but UUIDs by the BLE library. Each one will
// have some properties set as follows:
// BLEWrite -   This means the peripheral will accept write commands from the
//              central device.
// BLERead -    This means the value of the characteristic can be read by the
//              central device
// BLENotify -  This means the value will be sent to the subscribed central
//              device when it changes.
// BLEIndicate - This is the same as the notify, however the write of the value
//              blocks until an acknowledge has been received from the connected
//              central device, so that continuous data can be sent without any
//              lost data.

/// @brief  The unlock characteristic only has the write property, as we do not
///         want others to see what unlock codes have been sent.
BLECharacteristic unlockChar("F002", BLEWrite, MAX_PACKET_LENGTH);

/// @brief  The status characteristic will update when the lock state changes.
///         In addition to this, newly connected central devices will want to
///         know the status when connecting, so it also has the read property.
BLECharacteristic statusChar("F003", BLENotify | BLERead, MAX_PACKET_LENGTH);

/// @brief  The log characteristic is triggered by a button press on the device,
///         where a short press will send a brief log message, and a prolonged
///         press will inform the user of when the last unlock events took place.
///         As log notes will be sent, we want to use the indicate property so
///         that multiple messages can be sent to the central device without
///         being lost. As with the status, a newly connected central device
///         may want to know a little more about what was last sent.
BLECharacteristic logChar("F004", BLEIndicate | BLERead, MAX_PACKET_LENGTH);

// The descriptors for the characteristics. These are all simply the user
// description, allowing the central devices to better understand what the
// characteristic is for. Note that they all use the UUID 0x2901, which is the
// UUID for the user description of a characteristic. Please see
// https://www.bluetooth.com/specifications/gatt/descriptors/ for a full list.
// The string that follows is what will be seen if the descriptor is read by
// the central device.

/// @brief  The unlock characteristic user description
BLEDescriptor unlockDesc("2901", "Accepts unlock codes.");
/// @brief  The status characteristic user description
BLEDescriptor statusDesc("2901", "Provides the current status string.");
/// @brief  The log characteristic user description
BLEDescriptor logDesc("2901", "Provides running log details.");

/// @brief  The output helper for writing to and reading from the connected
///         LED pin. This pin is used to indicate when there is a central
///         device connected to this peripheral.
OutputHelper connectedLed(Pins::ConnectedLed);

/// @brief  The lock object coordinates all of the lock logic, including the
///         unlock mechanism(s), signalling state changes and log triggers, and
///         checking/updating the secret code(s).
///         The constructor takes in the pins to control for the locked and
///         unlocked outputs, as well as the manual override switch and
///         multi-purpose button inputs. Two function pointers are also
///         provided, which are triggered when the lock state changes and when
///         the multi-purpose button has triggered short or long log message
///         reports.
Lock lock(
    Pins::LockedLed,
    Pins::UnlockedLed,
    Pins::ManualOverrideSwitch,
    Pins::MultiPurpBtn,
    lockStateChange,
    logMessageHandler
);

/**
 * Functions
 */

/*******************************************************************************
 * @brief   Sends a long message to a given characteristic in small, packet sized
 *          chunks. Sending anything above the 20 characters appears to be ignored.
 *
 * @param   c           The characteristic to write the message to
 * @param   message     The message to be written
 *
 *
 * @todo    It may be worth checking the characteristic properties as notify
 *          messages may be dropped, whilst indicate messages require an ack
 *          from the central device.
 */
static void sendCharacteristicMessage(BLECharacteristic c, const char *message)
{
    static const int MAX_MESSAGE_LENGTH = MAX_PACKET_LENGTH - 1;
    char messageBuffer[MAX_PACKET_LENGTH];
    const int fullMessageLen = strlen(message);
    for(int i = 0; i < fullMessageLen; i += MAX_MESSAGE_LENGTH)
    {
        const int messageLen = min(MAX_MESSAGE_LENGTH, fullMessageLen - i);
        memcpy(messageBuffer, message + i, messageLen);
        messageBuffer[messageLen] = '\0';
        c.writeValue(messageBuffer);
    }
}

/*******************************************************************************
 * @brief   Handler for when the multi-purpose button is pressed and a short
 *          or long report is to be sent to connected subscribers.
 *
 * @param   full     Whether to send the full log or brief
 */
static void logMessageHandler(const bool full)
{
    char buffer[128] = {'\0'};
    sprintf(
        buffer,
        "Lock is currently %s.",
        (lock ? LOCKED_STRING : UNLOCKED_STRING)
    );
    Serial.println(buffer);

    // NOTE:    Here, we're not sending a long string as by default,
    //          characteristic messages are set to have a maximum length of
    //          20 characters. The sendCharacteristicMessage() function handles
    //          this by breaking the message down into chunks and sending them
    //          across. This is again used when sending the full message.
    sendCharacteristicMessage(logChar, buffer);
    if (full)
    {
        // Send the full log
        for(int i = 0; i < MAX_UNLOCK_TIMES; i++)
        {
            const long lastUnlock = lock.getUnlockTime(i);
            if (lastUnlock != 0)
            {
                const long delta = millis() - lastUnlock;
                const float since = delta / 1000.0f;
                sprintf(buffer, "Unlocked %2.1f seconds ago.", since);
                sendCharacteristicMessage(logChar, buffer);
            }
        }
    }
    // On the central device, the messages will be received quite quickly, so
    // it sending across this standard end of message text will indicate that
    // all of the message has been received.
    logChar.writeValue("End of log.");
}

/*******************************************************************************
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

    if (lock.unlockWithMessage(message))
    {
        Serial.println("Valid code received - unlocking");
    }
    char blank[MAX_PACKET_LENGTH] = {'\0'};
    characteristic.writeValue(blank);
}

/*******************************************************************************
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

/*******************************************************************************
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

/*******************************************************************************
 * @brief   This handler *should* be called when the BLENotify characteristic is
 *          subscribed to. Sadly, this does not appear to be working, and some
 *          digging shows this pull request from 2017:
 *          https://github.com/arduino/ArduinoCore-arc32/pull/532/commits
 */
static void subscribedCallback(BLEDevice central, BLECharacteristic characteristic)
{
    Serial.println(String("*** Subscribed by ") + central.address());
}

/*******************************************************************************
 * @brief   This handler *should* be called when the BLENotify characteristic is
 *          unsubscribed. Sadly, this does not appear to be working, and some
 *          digging shows this pull request from 2017:
 *          https://github.com/arduino/ArduinoCore-arc32/pull/532/commits
 */
static void unsubscribedCallback(BLEDevice central, BLECharacteristic characteristic)
{
    Serial.println(String("*** Unsubscribed by ") + central.address());
}

/*******************************************************************************
 * @brief   Lock state change handler function. Reports any changes to the
 *          locked state characteristic.
 *
 * @param   state   The current state of the lock
 */
static void lockStateChange(const LockState state)
{
    if (state == (LockState::Locked + LockState::UpdateSecretCode))
    {
        statusChar.writeValue("Update (locked)");
    }
    else if (state == (LockState::Unlocked + LockState::UpdateSecretCode))
    {
        statusChar.writeValue("Update (unlocked)");
    }
    else if (state == LockState::Locked)
    {
        statusChar.writeValue(LOCKED_STRING);
    }
    else if (state == LockState::Unlocked)
    {
        statusChar.writeValue(UNLOCKED_STRING);
    }
    else if (state == LockState::ManuallyOverridden)
    {
        statusChar.writeValue("Manually Unlocked");
    }
    else
    {
        if (Serial)
        {
            Serial.println(String("Unknown state:") + state);
        }
    }
}

/*******************************************************************************
 * @brief   Prints the start-up information to the serial port.
 */
static void printStartInfo()
{
    Serial.print("Server stated on: ");
    Serial.println(BLE.address());
    Lock::printStartInfo();
}

/*******************************************************************************
 * @brief   Sets up the Arduino, run once before carrying out the loop()
 *          function.
 */
void setup() {

    // Start the serial communications, then wait for it to initialise
    Serial.begin(9600);
    while (!Serial)
    {
        delay(1);
    }

    Serial.println("Starting the peripheral server.");


    // BLE related set up code
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
    logChar.addDescriptor(logDesc);

    unlockChar.setEventHandler(BLEWritten, unlockMessageWritten);
    // Sadly these won't ever be called, but they're here for completeness.
    unlockChar.setEventHandler(BLESubscribed, subscribedCallback);
    unlockChar.setEventHandler(BLEUnsubscribed, unsubscribedCallback);

    // Add the characteristics to the service
    lockService.addCharacteristic(unlockChar);
    lockService.addCharacteristic(statusChar);
    lockService.addCharacteristic(logChar);

    // Set event handlers to toggle the connected LED
    BLE.setEventHandler(BLEConnected, connectedHandler);
    BLE.setEventHandler(BLEDisconnected, disconnectedHandler);

    // Add the service to the BLE peripheral and start advertising
    BLE.addService(lockService);
    BLE.advertise();

    // Initialise the lock
    Lock::initialise();

    // Set the current lock state and log state
    lockStateChange(lock.getLockState());
    logChar.writeValue("No log yet.");

    // Display any start up information
    printStartInfo();
}

/*******************************************************************************
 * @brief   Loop function carried out indefinitely after set up.
 */
void loop() {
    // Poll the BLE object to pick up on any new activity
    BLE.poll();
    // Poll the lock to check for any changes that are required
    lock.poll();
}
