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
        ColourPrinter.__init__(self, colour, name)


class UnlockChar(KrisCharacteristic):

    SECRET_KEY = 'abc123'

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

    def onWriteRequest(self, data, offset, withoutResponse, callback):
        """
        Handles the write request
        """
        self.print(dir(self))
        value = data.decode()
        self.print(f'Write request received, data: {data}, offset: {offset}')
        self.print(f'Current value is {self._value}')
        if data != self._value:
            self.print('The value has changed - Signal any listeners')
            for key, observer in self._changeObservers.items():
                self.print(f'Signalling observer {key}')
                observer(self._value[0])
        self._value = value

    def onNotify(self):
        self.print('onNotify called... apparently')



class StatusChar(KrisCharacteristic):
    """
    Provides the characteristic for an LED
    """
    def __init__(self, uuid: str) -> None:
        """
        Constructs the StatusChar
        """
        # self._value = wiringpi.digitalRead(self._led)
        KrisCharacteristic.__init__(self, {
            'uuid': uuid,
            'properties': ['notify'],
            'value': ''
            },
            'StatusChar',
            ColourPrinter.GOLD
        )
        self._value = 'Locked'
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

def onStateChange(state: str) -> None:
    """
    The state change handler function
    """
    global server
    printer = ColourPrinter(ColourPrinter.BG_BLUE, 'StateChange')
    printer.print(f'on -> State Change: {state}')

    if state == 'poweredOn':
        server.startAdvertising('Raspberry Pi Lock', ['FF10'])
    else:
        server.stopAdvertising()

def onAdvertisingStart(error: bool) -> None:
    """
    The advertising handler function
    """
    printer = ColourPrinter(ColourPrinter.BG_RED, 'AdvertisingStart')
    printer.print(f'on -> Advertising Start: {error}')
    if not error:
        global server
        status = StatusChar('FF12')
        switch = UnlockChar('FF11')
        # switch.addObserver('FF12', status.set)
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

cp = ColourPrinter(ColourPrinter.BG_SILVER + ColourPrinter.GOLD, 'Script')

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
cp.print('Exiting.')
