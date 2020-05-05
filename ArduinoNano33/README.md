# Arduino Nano 33 (IoT/BLE)
This directory contains the Arduino sketch for creating a BLE peripheral device using an [Arduino Nano 33 IoT](https://store.arduino.cc/arduino-nano-33-iot) or [Arduino Nano 33 BLE](https://store.arduino.cc/arduino-nano-33-ble) device.

## Purpose
This project demonstrates the following a peripheral device to do the following:
1. Clearly demonstrate GAP advertising
1. Provide an easy to understand service
1. Provide characteristics that provide most properties; read, write, notify and indicate
1. Provide standard descriptors for the characteristics

## Description
The project simulates a lock with the following capabilities:
- Outputs over GPIO to indicate the following:

    - Central device connected
    - Lock state "locked"
    - Lock state "unlocked"
    - Lock secret code update allowed
- Multi-purpose GPIO input to carry out the following tasks:
    - Send a brief log message to connected central device (high for a short period)
    - Send a more detailed log message to connected central device (high for a few seconds), including a list of times since it was last unlocked.
    - Allow the secret code to be updated by the connected central device when next written (high for prolonged period of time)
- Manual override GPIO input for manually unlocking the lock
- BLE Connectivity providing the following:
    - Notifications of the lock status
    - Read the current status
    - Write a secret unlock code, or update the secret code (see multi-purpose button)
    - Indications of the log messages (see multi-purpose button)

In theory, the GPIO outputs could well be connected to a relay/solenoid or other means to actually control a lock, and the manual override and multi-purpose inputs could well be used with real switches.

Personally, I have created a small board for both Arduino Nano 33 boards and Raspberry Pis, with LEDs, a button and a switch. This was used for development and to visualise the lock states, as well as provide means to provide notifications and indications to the central device.

The sketch itself was tested with an IoT board, however the code should work just as well on a BLE device.

## Prerequisites
### Boards
The Nano 33 board must firstly be added to the list of known boards. In the Arduino IDE, go to "Tools" > "Boards xxx" > "Board Manager..." and search for:
- "Arduino SAMD Boards" for **Arduino Nano 33 IoT** or
- "Arduino nRF528x Boards" for **Arduino Nano 33 BLE**

### Libraries

Two additional libraries are required for the Arduino IDE:
- [ArduinoBLE](https://www.arduino.cc/en/Reference/ArduinoBLE)
- [FlashStorage](https://www.arduinolibraries.info/libraries/flash-storage)

Both can be installed by going to "Tools" > "Manage Libraries" and searching for them by name.

### Hardware
#### Parts required
- Arduino Nano 33 IoT or BLE
- Breadboard or other
- 3 LEDs and suitable resistors (e.g. 220R)
- 1 PTM switch and suitable pull-down resistor (e.g. 10k)
- 1 rocker switch and suitable pull-down resistor (e.g. 10k)
- USB cable for programming the Arduino

#### GPIO mapping
The project relies on some hardware attached to the Arduino board, and the default pin mappings are as follows:

| GPIO | Name | Purpose |
| ------------- | ------------- | ------------- |
| D2 | Locked LED | Is illuminated when the device is "locked" |
| D3 | Unlocked LED | Is illuminated when the device is "unlocked" |
| D4 | Connected LED | Is illuminated when a central device is connected |
| D5 | Multi-purpose button | A PTM button used to trigger log messages and update the secret code |
| D6 | Manual override switch | A simple rocker switch to manually unlock the device |


## Running
When placed on the Arduino, the device will wait for a serial connection, and until one is found, it will

## Disclaimer
Whilst this is not necessarily the most secure lock in the world, I have tried to make some of the features more realistic, such as only being able to update the secret code by having physical access to the device itself. Having said that, I do not in any way recommend using this software as the basis of your own lock!

If you choose to use this software as an actual locking mechanism, I accept no responsibility for the robustness, accuracy or security of your lock!
