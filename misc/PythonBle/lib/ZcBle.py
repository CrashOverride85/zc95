import asyncio
import time
import struct
import queue
from bleak import BleakClient
from bleak import BleakScanner

class ZcBle:
    def __init__(self, ble_tx_queue, ble_address = None):
        self.ble_address = ble_address
        self.ble_tx_queue = ble_tx_queue
        self.connected = False
        self.characteristic_channel_power_level = {}
        self.characteristic_channel_power_enable = {}
        self.characteristic_channel_pulse_width = {}
        self.characteristic_channel_frequency = {}

    def set_characteristics(self):
        self.gen_control_service = self.client.services.get_service("AC7744C0-0BAD-11EF-A9CD-0800200C9B00")
        self.characteristic_channel_power_level[1] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B31")
        self.characteristic_channel_power_level[2] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B32")
        self.characteristic_channel_power_level[3] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B33")
        self.characteristic_channel_power_level[4] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B34")

        self.characteristic_channel_power_enable[1] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B41")
        self.characteristic_channel_power_enable[2] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B42")
        self.characteristic_channel_power_enable[3] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B43")
        self.characteristic_channel_power_enable[4] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B44")

        self.characteristic_channel_pulse_width[1] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B11")
        self.characteristic_channel_pulse_width[2] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B12")
        self.characteristic_channel_pulse_width[3] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B13")
        self.characteristic_channel_pulse_width[4] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B14")

        self.characteristic_channel_frequency[1] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B21")
        self.characteristic_channel_frequency[2] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B22")
        self.characteristic_channel_frequency[3] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B23")
        self.characteristic_channel_frequency[4] = self.gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B24")

    async def ble_scan(self):
        print("Scanning...")
        devices = await BleakScanner.discover(
            return_adv=True,
            service_uuids=['AC7744C0-0BAD-11EF-A9CD-0800200C9B00']
        )

        for d, a in devices.values():
            print("Connecting to: " + str(d.name) + " (" + d.address + ")")
            return d.address

        print("Not found!")
        return None

    async def start(self):
        if self.ble_address is None:
            self.ble_address = await self.ble_scan()

        if self.ble_address == None:
            print("No address to connect to")
            return

        self.client = BleakClient(self.ble_address)
        await self.client.connect()

        self.connected = True
        print("Connected")

        self.set_characteristics()

        while True:
            message = await self.ble_tx_queue.get()
            print("GOT: " + message)

    async def channel_power_enable(self, channel, enable):
        if not self.connected:
            return

        if enable:
            enable_int = 1
        else:
            enable_int = 0

        await self.client.write_gatt_char(self.characteristic_channel_power_enable[channel], struct.pack('B', enable_int), response=False)

    async def channel_power_level(self, channel, power):
        if not self.connected:
            return
        await self.client.write_gatt_char(self.characteristic_channel_power_level[channel], struct.pack('>H', power), response=False)

    async def channel_pulse_width(self, channel, pos, neg):
        if not self.connected:
            return
        await self.client.write_gatt_char(self.characteristic_channel_pulse_width[channel], struct.pack('BB', pos, neg), response=False)

    async def channel_frequency(self, channel, freq):
        if not self.connected:
            return
        await self.client.write_gatt_char(self.characteristic_channel_frequency[channel], struct.pack('B', freq), response=False)

