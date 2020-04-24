#!/usr/bin/python3
# ------------------------------------------------------------------------------
"""@package ble_lights.py

Sets the attached light values and notifies when they change.
"""
# ------------------------------------------------------------------------------
#                  Kris Dunning ippie52@gmail.com 2020.
# ------------------------------------------------------------------------------
import pybleno as ble
import wiringpi
from typing import Dict
from sys import exit
from time import sleep
from colour_printer import ColourPrinter

class KrisCharacteristic(ColourPrinter, ble.Characteristic):
    """
    Provides the base for debugging a characteristic
    """
    def __init__(self, settings: Dict[str, any], name: str, colour: str) -> None:
        """
        Initialises the object
        """
        ble.Characteristic.__init__(self, settings)
        ColourPrinter.__init__(colour, name)


class UnlockChar(KrisCharacteristic):
    """
    Provides the characteristic for the UnlockChar
    """
    def __init__(self, uuid: str) -> None:
        """
        Constructs ths UnlockChar
        """
        self._changeObservers = {}
        KrisCharacteristic.__init__(self, {
            'uuid': uuid,
            'properties': ['write'],
            'value': ''
            },
            'UnlockChar',
            ColourPrinter.GREEN
        )
        self._value = ''


    # def addObserver(self, name: str, observer) -> None:
    #     """
    #     Custom observer to turn on an LED
    #     """
    #     self.print(f'Adding observer for {name}.')
    #     self._changeObservers[name] = observer

    # def removeObserver(self, name: str) -> None:
    #     if name in self._changeObservers.keys():
    #         self.print(f'Removing observer {name}.')
    #         del self._changeObservers[name]
    #     else:
    #         self.print(f'Could not find observer {name} to remove.')

    # def onReadRequest(self, offset, callback):
    #     """
    #     Handles the read request for this characteristic
    #     """
    #     self.print(f'Read request received, offset {offset}')
    #     callback(ble.Characteristic.RESULT_SUCCESS, self._value)

    def onWriteRequest(self, data, offset, withoutResponse, callback):
        """
        Handles the write request
        """
        self.print(f'Write request received, data: {data}, offset: {offset}')
        if data != self._value[0]:
            self.print('The value has changed - Signal any listeners')
            for key, observer in self._changeObservers.items():
                self.print(f'Signalling observer {key}')
                observer(self._value[0])
        self._value = value

class StatusChar(KrisCharacteristic):
    """
    Provides the characteristic for an LED
    """
    def __init__(self, uuid: str, led: int) -> None:
        """
        Constructs the StatusChar
        """
        self._led = led
        self._value = wiringpi.digitalRead(self._led)
        KrisCharacteristic.__init__(self, {
            'uuid': uuid,
            'properties': ['notify'],
            'value': self._value
            },
            'StatusChar',
            ColourPrinter.GOLD
        )
        self._updateValueCallbacks = None

    def onSubscribe(self, maxValueSize: int, updateValueCallback) -> None:
        """
        Sets the update value callback
        """
        self.print('New subscriber added.')
        self._updateValueCallback = updateValueCallback

    def onUnsubscribe(self) -> None:
        """
        Removes the update value callback
        """
        self.print('Subscriber removed')
        self._updateValueCallback = None

    # def set(self, new_value: int):
    #     """
    #     Sets the value of the LED
    #     """
    #     new_value = 0 if new_value == 0 else 1
    #     wiringpi.digitalWrite(self._led, new_value)
    #     self._value = new_value


def onStateChange(state: str) -> None:
    """
    The state change handler function
    """
    global server
    print(f'on -> State Change: {state}')

    if state == 'poweredOn':
        server.startAdvertising('Kris Service?', ['FF10'])
    else:
        server.stopAdvertising()

def onAdvertisingStart(error: bool) -> None:
    """
    The advertising handler function
    """
    print(f'on -> Advertising Start: {error}')
    if not error:
        global server
        status = StatusChar('FF12')
        switch = UnlockChar('FF11')
        switch.addObserver('FF12', status.set)
        server.setServices([
            ble.BlenoPrimaryService({
                'uuid': 'FF10',
                'characteristics': [status, switch]
                })
            ]
        )

RED_GPIO = 0
GRN_GPIO = 2
BLU_GPIO = 3

LED_SEQUENCE = [RED_GPIO, GRN_GPIO, BLU_GPIO]

BTN_GPIO = 1

wiringpi.wiringPiSetup()  # For GPIO pin numbering
for led in LED_SEQUENCE:
    wiringpi.pinMode(led, 1)
    wiringpi.digitalWrite(led, 0)
wiringpi.pinMode(BTN_GPIO, 0)

cp = ColourPrinter(ColourPrinter.SILVER, 'Script')

cp.print('Creating the server...')
server = ble.Bleno()
cp.print('Binding the onStateChange handler')
server.on('stateChange', onStateChange)
cp.print('Binding the onAdvertisingStart handler')
server.on('advertisingStart', onAdvertisingStart)
cp.print('Starting the server...')
server.start()

running = True
while running:
    try:
        sleep(0.1)
    except KeyboardInterrupt:
        cp.print('Polite exit.')
        running = False

server.stopAdvertising()
server.disconnect()
