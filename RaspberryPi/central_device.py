#!/usr/bin/python3
# -*- mode: python; coding: utf-8 -*-
# ------------------------------------------------------------------------------
"""@package central_device.py

Provides the means to communicate with a BLE-Tutor lock peripheral, regardless
of the platform or language the peripheral is written in.
"""
# ------------------------------------------------------------------------------
#                  Kris Dunning ippie52@gmail.com 2020.
# ------------------------------------------------------------------------------

from bluetooth.ble import GATTRequester, DiscoveryService
from colour_printer import ColourPrinter
from argparse import ArgumentParser
from time import sleep
from threading import Event
from killable_thread import KillableThread
from sys import stdin
import json
import struct


class Descriptor:
    """
    Descriptor class to represent characteristic descriptors. Sadly, the Pybluez
    module does not seem to provide any descriptor information, aside from
    guessing at the handle and querying them directly. No means of identifying
    what type of descriptor is available either, it's just guesswork!
    """

    class Properties:
        """
        Class to store the subscription flags for notifications and indications.
        """
        NOTIFY_SUBSCRIBE = 0x1
        INDICATE_SUBSCRIBE = 0x2

    def __init__(self, requester, handle, properties):
        """
        Constructor - Takes the requester and queries the handle for the
        suspected descriptor. If it returns two bytes, and those two bytes read
        back as zero, assume this is a "Client Characteristic Configuration"
        descriptor (see
        0x2902 https://www.bluetooth.com/specifications/gatt/descriptors/),
        the characteristic properties will dictate whether or not to subscribe
        to the available notifications and/or indications as appropriate.
        """
        self.requester = requester
        self.handle = handle
        self.value = self.get_value()

        self.notify = properties & Characteristic.Properties.NOTIFY
        self.indicate = properties & Characteristic.Properties.INDICATE

        # If the characteristic has notify or indicate properties...
        # And if there are two bytes in the value...
        if (self.notify or self.indicate) and len(self.value) == 2:
            # Unpack the value as a short, and then...
            prop = struct.unpack('H', self.value)[0]
            # If the value read back is zero...
            if prop == 0:
                self.subscribe()

    def __del__(self):
        """
        Handles the destruction of the descriptor
        """
        self.unsubscribe()

    def get_value(self):
        """
        Requests the current value for this descriptor
        """
        return self.requester.read_by_handle(self.handle)[0]

    def __repr__(self):
        """
        """
        text  = f'\t\tDescriptor: {self.get_value()}\n'
        return text

    def subscribe(self):
        """
        Subscribes to indications and or notifications as required
        """
        prop  = Descriptor.Properties.NOTIFY_SUBSCRIBE if self.notify else 0
        prop += Descriptor.Properties.INDICATE_SUBSCRIBE if self.indicate else 0
        prop = struct.pack('H', prop)
        try:
            self.requester.write_by_handle(self.handle, prop)
            sleep(0.01)
            self.value = self.get_value()
        except Exception as e:
            print(
                ColourPrinter.RED,
                "Subscribing to the characteristic failed.",
                ColourPrinter.NORMAL
            )

    def unsubscribe(self):
        """
        Unsubscribes from and notifications and/or indications
        """
        if self.get_value() != 0:
            prop = struct.pack('H', 0)
            try:
                self.requester.write_by_handle(self.handle, prop)
                sleep(0.01)
                self.value = self.get_value()
            except Exception as e:
                print(
                    ColourPrinter.RED,
                    "Unsubscribing from the characteristic failed.",
                    ColourPrinter.NORMAL
                )

class Characteristic:
    """
    Class to represent a GATT characteristic.
    """
    class Properties:
        """
        Class to store the property values for characteristics
        """

        BROADCAST = 0x01
        READ = 0x02
        WRITE_WO_REPLY = 0x04
        WRITE = 0x08
        NOTIFY = 0x10
        INDICATE = 0x20

    class Info:
        """
        Provides a characteristic info object
        """

        def __init__(self, packed_data):
            """
            Constructor - Unpacks the packed info data
            @TODO - Fill this in!
            """
            pass

    def __init__(self, requester, data):
        """
        Constructor - Sets the parameters of the characteristic and reads its value
        """
        self.requester = requester
        self.uuid = data['uuid']
        self.properties = data['properties']
        self.handle = data['handle']
        self.value_handle = data['value_handle']
        self.value = self.get_value()
        self.info = self.get_info()
        self.descriptors = []

    def add_descriptor(self, handle):
        """
        Adds a descriptor to this characteristic.
        As descriptors are not provided in the pybluez library, this is done by
        assuming missing handles are descriptors.
        """
        desc = Descriptor(self.requester, handle, self.properties)
        self.descriptors.append(desc)
        return desc.value

    def can_read(self):
        """
        Returns true if this characteristic is readable
        """
        print(Characteristic.Properties.READ, self.properties)
        return (Characteristic.Properties.READ & self.properties)

    def can_write(self):
        """
        Returns true if this characteristic is writeable
        """
        return (Characteristic.Properties.WRITE & self.properties)

    def get_value(self):
        """
        Gets the current value of the characteristic if readable
        """
        if self.can_read():
            return self.requester.read_by_handle(self.value_handle)[0]
        else:
            return None

    def get_info(self):
        """
        Gets the info stored at the handle
        """
        return self.requester.read_by_handle(self.handle)[0]

    def __repr__(self):
        """
        Returns the dictionary of this object as a string
        """
        text  = f"\tCharacteristic {self.uuid}\n"
        text += f"\t\tProperties: "
        if self.properties & Characteristic.Properties.BROADCAST:
            text += f" BROADCAST "
        if self.properties & Characteristic.Properties.READ:
            text += f" READ "
        if self.properties & Characteristic.Properties.WRITE_WO_REPLY:
            text += f" WRITE_WO_REPLY "
        if self.properties & Characteristic.Properties.WRITE:
            text += f" WRITE "
        if self.properties & Characteristic.Properties.NOTIFY:
            text += f" NOTIFY "
        if self.properties & Characteristic.Properties.INDICATE:
            text += f" INDICATE "
        text += "\n"
        text += f"\t\tValue: {self.value}\n"
        for descriptor in self.descriptors:
            text += str(descriptor)

        text += "\n"
        return text

class Service:
    """
    Class representation of a BLE GATT service.
    """
    def __init__(self, requester, data, chars=None):
        """
        Constructs the service and all of its characteristics.
        """
        self.requester = requester
        self.uuid = data['uuid']
        self.start = data['start']
        self.end = data['end']
        if chars is None:
            chars = self.requester.discover_characteristics()

        self.chars = []
        handles = [self.start]
        next_expected_handle = self.start + 1
        for char in chars:
            if char['handle'] > self.start and char['handle'] < self.end:
                characteristic = Characteristic(self.requester, char)
                handles.append(characteristic.handle)
                handles.append(characteristic.value_handle)
                while next_expected_handle not in handles:
                    # print(next_expected_handle, "is not in", handles)
                    if len(self.chars):
                        self.chars[-1].add_descriptor(next_expected_handle)
                        handles.append(next_expected_handle)
                        next_expected_handle += 1
                    else:
                        next_expected_handle = max(handles)
                # else:
                    # print(next_expected_handle, "IS IN", handles)

                self.chars.append(characteristic)
                # print(next_expected_handle, "is now", end=' ')
                next_expected_handle = max(handles) + 1
                # print(next_expected_handle)
        while next_expected_handle < self.end:
            self.chars[-1].add_descriptor(next_expected_handle)
            handles.append(next_expected_handle)
            next_expected_handle += 1

        missing_handles = []
        for x in range(self.start, self.end):
            if x not in handles:
                missing_handles.append(x)
                print(ColourPrinter.RED, f'Handle {x} is missing!')
                print(self.requester.read_by_handle(x))
                print(ColourPrinter.NORMAL)


    def __repr__(self):
        """
        """
        text  = f'{ColourPrinter.YELLOW}Service {self.uuid}\n'
        text += f'{ColourPrinter.GOLD}Characteristics:\n'
        for char in self.chars:
            text += str(char)
        text += f"{ColourPrinter.NORMAL}\n"
        return text

class LockRequester(GATTRequester):
    """
    Requester class for requesting information from the lock peripheral
    """
    def __init__(self, event, *args):
        """
        Constructor - Sets up the GATTRequester
        """
        self.event = event
        self.notifications = []
        self.indications = []
        GATTRequester.__init__(self, *args)

    def on_notification(self, handle, data):
        """
        """
        for observer in self.notifications:
            observer(handle, data)
        self.event.set()

    def on_indication(self, handle, data):
        """
        """
        for observer in self.indications:
            observer(handle, data)
        self.event.set()

    def add_notification_observer(self, observer):
        """
        Adds an observer method for received notifications
        """
        self.notifications.append(observer)

    def add_indication_observer(self, observer):
        """
        Adds an observer method for received indications
        """
        self.indications.append(observer)



class CentralLockDevice(KillableThread):
    """
    Provides a central device class, used to connect and communicate with the
    lock peripheral
    """

    class KnownUUID:
        """
        Class providing known UUID values
        """
        LOCK_SERVICE_UUID = "F001"
        UNLOCK_CHAR_UUID = "F002"
        STATUS_CHAR_UUID = "F003"
        LOG_CHAR_UUID = "F004"

        @staticmethod
        def is_known(uuid):
            """
            Returns true if the UUID is known
            """
            known_uuids = [
                KnownUUID.LOCK_SERVICE_UUID,
                KnownUUID.UNLOCK_CHAR_UUID,
                KnownUUID.STATUS_CHAR_UUID,
                KnownUUID.LOG_CHAR_UUID,
            ]
            if len(uuid) == 36:
                uuid = CentralLockDevice.get_uuid16(uuid)
            return uuid in known_uuids



    def __init__(self, address, status_listener, log_listener):
        """
        Constructor - Connects to the peripheral device
        """
        self.event = Event()
        self.requester = LockRequester(self.event, address, False)
        self.requester.add_notification_observer(self.data_received)
        self.requester.add_indication_observer(self.data_received)
        KillableThread.__init__(self)
        self.connect()
        self.services = {}
        self.handle_map = {}
        self.uuid_map = {}
        self.status_listener = status_listener
        self.log_listener = log_listener
        self.log_builder = ""

    def data_received(self, handle, data):
        """
        Handles both incoming notifications and indications.
        """
        if handle in self.handle_map:
            uuid = self.handle_map[handle]
            if uuid == CentralLockDevice.KnownUUID.STATUS_CHAR_UUID:
                message_len = len(data) - 3
                fmt = f"BBB{message_len}s"
                q, h1, h2, message = struct.unpack(fmt, data)
                self.status_listener(message.decode())
            elif uuid == CentralLockDevice.KnownUUID.LOG_CHAR_UUID:
                message_len = len(data) - 3
                fmt = f"BBB{message_len}s"
                q, h1, h2, message = struct.unpack(fmt, data)
                self.handle_log_message(message.decode())
            else:
                print(f"{uuid} is not known?")


    def handle_log_message(self, message):
        """
        Handles an incoming log message, as they are split over several
        messages, they must be concatenated.
        """
        if isinstance(message, bytes):
            message = message.decode()
        if message == "End of log.":
            self.log_listener(self.log_builder)
            self.log_builder = ""
        else:
            self.log_builder += message



    def run(self):
        """
        """
        while self.is_running():
            self.event.wait()
            self.event.clear()

    def connect(self):
        """
        Connects to the peripheral device
        """
        print("Connecting...")
        self.requester.connect(True)
        self.start()
        print("Connected.")

    def read_status(self):
        """
        Reads the status value from the lock status characteristic
        """
        return self.requester.read_by_uuid(
            CentralLockDevice.KnownUUID.STATUS_CHAR_UUID
        )[0]

    def read_log(self):
        """
        Reads the value of the log characteristic
        """
        return self.requester.read_by_uuid(
            CentralLockDevice.KnownUUID.LOG_CHAR_UUID
        )[0]

    def disconnect(self):
        """
        """
        self.requester.disconnect()

    def discover_characteristics(self):
        """
        Discovers the service characteristics of the connected device
        """
        return self.requester.discover_characteristics()

    def discover_primary(self):
        """
        """
        return self.requester.discover_primary();

    def read_by_uuid(self, uuid):
        """
        """
        result = self.requester.read_by_uuid(uuid)
        sleep(0.01)
        return result

    def read_by_handle(self, handle):
        """
        """
        result = self.requester.read_by_handle(handle)
        sleep(0.01)
        return result

    def request_data(self):
        data = self.requester.read_by_handle(0x1)
        print(ColourPrinter.YELLOW, data, ColourPrinter.NORMAL)

        if isinstance(data, list):
            for d in data:
                d = d.decode()
                # print(ColourPrinter.GOLD, d, ColourPrinter.NORMAL)
                print(ColourPrinter.GOLD, "bytes received:", end=' ')

                for b in d:
                    print(hex(ord(b)), end=' ')
                print(ColourPrinter.NORMAL)

    def write(self, handle, message):
        """
        """
        return self.requester.write_by_handle(handle, message)

    def write_secret(self, message):
        """
        Writes the secret message to the BLE unlock characteristic
        """
        if isinstance(message, str):
            message = message.encode()
        print("Writing the secret message to the lock...")
        self.requester.write_by_handle(
            self.uuid_map[CentralLockDevice.KnownUUID.UNLOCK_CHAR_UUID],
            message
        )

    def print_service(self):
        """
        Prints the current services found
        """
        for service in self.services.values():
            print(service)

    @staticmethod
    def get_uuid16(uuid):
        """
        """
        if len(uuid) != 4:
            return uuid[4:8].upper()
        else:
            return uuid.upper();

    @staticmethod
    def get_uuid128(uuid):
        """
        """
        if len(uuid) == 4:
            return f"0000{uuid}-0000-1000-8000-00805f9b34fb"
        else:
            return uuid

    def query_peripheral(self):
        """
        """
        chars = self.requester.discover_characteristics()
        services = self.requester.discover_primary();
        self.services = {}
        self.handle_map = {}
        self.uuid_map = {}
        for service in services:
            uuid16 = CentralLockDevice.get_uuid16(service['uuid'])
            self.handle_map[service['start']] = uuid16
            self.uuid_map[uuid16] = service['start']
            self.services[uuid16] = Service(self.requester, service, chars)

        for char in chars:
            uuid16 = CentralLockDevice.get_uuid16(char['uuid'])
            self.handle_map[char['value_handle']] = uuid16
            self.uuid_map[uuid16] = char['value_handle']

        # print(ColourPrinter.BG_GREEN, self.services, ColourPrinter.NORMAL)


class LockActionHandler:
    """
    Class to interface with the user and run BLE commands
    """
    def __init__(self):
        """
        Constructor
        """
        self.scan_duration = 3
        self.available_devices = None
        self.lock = None

    def handle_next_action(self):
        """
        Handles the next action required by the user
        """
        info = f'{ColourPrinter.BG_BLUE}{ColourPrinter.SILVER}'
        normal = f'{ColourPrinter.NORMAL}'
        print(f'{info}Available actions:{normal}')
        parser = ArgumentParser()
        parser.add_argument(
            '-s',
            '--scan',
            help='Scan for available BLE peripherals',
            action='store_true'
        )
        parser.add_argument(
            '-d',
            '--duration',
            metavar='duration',
            help=f'Sets the scan duration in seconds (default {self.scan_duration})',
            type=int,
            default=self.scan_duration
        )
        parser.add_argument(
            '-c',
            '--connect',
            help='Connect to a given device by its address',
            action='store_true'
        )
        parser.print_help()
        actions = input('Please select from one of the available options above: ')
        actions = [x for x in actions.split(' ') if len(x)]
        args = parser.parse_args(actions)
        print(args)
        if args.scan:
            self.available_devices = self.scan()
            self.print_devices(self.available_devices)

        if args.connect:
            self.connect(self.available_devices)

    def print_devices(self, devices):
        """
        prints the devices along with an index
        """
        name_col = ColourPrinter.RED
        addr_col = ColourPrinter.BRIGHT_RED
        normal = ColourPrinter.NORMAL
        for index, entry in enumerate(devices):
            print(f"{name_col} {index + 1}) {entry['name']}: {addr_col}{entry['address']}{normal}")

    def maintain_connection(self, address):
        """
        Connects to the peripheral and maintains the connection until
        the user wishes to disconnect
        """
        OPT_DISCONNECT = 'D'
        OPT_WRITE_CODE = 'W'
        OPT_READ_STATUS = 'S'
        OPT_READ_LOG = 'L'
        OPT_SHOW_BLE_DATA = 'B'

        if address is not None:
            self.lock = CentralLockDevice(address, self.update_status, self.update_log)
            connected = True
            self.lock.query_peripheral();

            while connected:
                print(f'{ColourPrinter.GREEN}Please enter one of the following options:')
                print(f'{OPT_DISCONNECT} - Disconnect')
                print(f'{OPT_WRITE_CODE} - Write secret code')
                print(f'{OPT_READ_STATUS} - Read status value')
                print(f'{OPT_READ_LOG} - Read log value')
                print(f'{OPT_SHOW_BLE_DATA} - Show BLE data')
                print(ColourPrinter.NORMAL)
                try:
                    choice = input().upper()
                    if len(choice) == 0:
                        continue

                    if choice[0] == OPT_DISCONNECT:
                        print('Disconnecting...')
                        self.lock.disconnect()
                        connected = False
                        print('Disconnected.')
                    elif choice[0] == OPT_WRITE_CODE:
                        self.write_secret_code()
                    elif choice[0] == OPT_READ_STATUS:
                        print('Latest status value:', self.lock.read_status())
                    elif choice[0] == OPT_READ_LOG:
                        print('Latest log value:', self.lock.read_log())
                    elif choice[0] == OPT_SHOW_BLE_DATA:
                        self.lock.print_service()
                    else:
                        print(f'{ColourPrinter.RED}Unknown option. Please try again!{ColourPrinter.NORMAL}')
                except KeyboardInterrupt:
                    connected = False
                except Exception as e:
                    print('Exception occurred...')
                    raise e

    def write_secret_code(self):
        """
        Writes a secret code to the BLE lock
        """
        message = input('Please enter the secret code: ')
        self.lock.write_secret(message)

    def read_status(self):
        """
        Reads the current status value
        """
        self.lock.read

    def update_log(self, message):
        """
        Updates the current log message
        """
        print(f"{ColourPrinter.GOLD}LOG: {message}{ColourPrinter.NORMAL}")

    def update_status(self, status):
        """
        Updates the current state
        """
        print(f"{ColourPrinter.SILVER}STATUS: {status}{ColourPrinter.NORMAL}")



    def connect(self, devices):
        """
        Provides the interface to connect to a BLE Lock device
        """
        if devices is None:
            devices = self.scan()
        print(devices)
        self.print_devices(devices)
        device = input("Please select from one of the devices listed above (enter address or index): ")
        address = None
        try:
            index = int(device) - 1
            if index >= 0 and index < len(devices):
                print("A) Connecting to", devices[index])
                address = devices[index]['address']
        except:
            for entry in devices:
                if device == entry['address']:
                    print("B) CONNECTING TO", entry)
                    address = entry['address']
                elif device == entry['name']:
                    print("C) Connecting to", entry)
                    address = entry['address']
        finally:
            self.maintain_connection(address)


    def scan(self):
        """
        Scans for nearby BLE devices and returns their attributes
        """
        service = DiscoveryService()
        print("Scanning...")
        dev_dict = service.discover(self.scan_duration)
        devices = []
        for addr, name in dev_dict.items():
            devices.append({'name': name, 'address': addr})
        return devices


def print_action_option(letter, description):
    """
    Prints the action option in a standard format
    """
    print(f" {ColourPrinter.BRIGHT_GREEN}{letter}{ColourPrinter.NORMAL}\t{description}")


def main():
    """
    The main function of the script when run in isolation
    """
    running = True
    lah = LockActionHandler()

    while running:
        try:
            lah.handle_next_action()
        except KeyboardInterrupt:
            print("Polite exit.")
            running = False
        except Exception as e:
            print("Exception hit. Boo!")
            raise e

if __name__ == '__main__':
    main()
