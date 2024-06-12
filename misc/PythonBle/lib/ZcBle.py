import asyncio
import time
import struct
import queue
from bleak import BleakClient
from bleak import BleakScanner
from lib.ZcPulseQueue import PulseQueue

PULSE_TIME_IN_FUTURE_MS  = 75 # When generating a pulse and adding it to the outbound queue, set its due time this number milliseconds in the future

class ZcBlePulseMessage:
    def __init__(self, cmd_type, pulse_width_pos, pulse_width_neg, amplitude, time_us, channel):
        self.cmd_type = cmd_type
        self.pulse_width_pos = pulse_width_pos
        self.pulse_width_neg = pulse_width_neg
        self.amplitude = amplitude
        self.time_us = time_us

        self.channel_mask = 0
        if channel == 1:
            self.channel_mask = 0x03
        elif channel == 2:
            self.channel_mask = 0x0C
        elif channel == 3:
            self.channel_mask = 0x30
        elif channel == 4:
            self.channel_mask = 0xC0

    def pack(self):
        return struct.pack('<BBBHQB', self.cmd_type, self.pulse_width_pos, self.pulse_width_neg, self.amplitude, self.time_us, self.channel_mask)

class ZcBle:
    def __init__(self, ble_address = None):
        self.ble_address = ble_address
        self.connected = False
        self.characteristic_channel_power_level = {}
        self.characteristic_channel_power_enable = {}
        self.characteristic_channel_pulse_width = {}
        self.characteristic_channel_frequency = {}

        self.pulse_start_time = None

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

        pulse_service = self.client.services.get_service("AC7744C0-0BAD-11EF-A9CD-0800200C9A00")
        self.characteristic_pulse = pulse_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9A01")

    def time_us(self):
        return time.time_ns() / 1000

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
        self.pq = PulseQueue(self.client, self.characteristic_pulse)

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

    # all_ versions that change all channels
    ###

    async def all_channel_power_enable(self, enable):
        for channel in range(1, 5):
            await self.channel_power_enable(channel, enable)

    async def all_channel_power_level(self, power):
        for channel in range(1, 5):
            await self.channel_power_level(channel, power)

    async def all_channel_pulse_width(self, pos, neg):
        for channel in range(1, 5):
            await self.channel_pulse_width(channel, pos, neg)

    async def all_channel_frequency(self, freq):
        for channel in range(1, 5):
            await self.channel_frequency(channel, freq)

######################################
###### BLE pulse based messages ######
######################################

    async def pulse_send_start(self):
        print("Send start")
        # Start message (more of a clock sync message). Tells the ZC95 that all future messages will have a time_us relative to now.
        msg  = bytes([1]) # first packet being sent only has one message - the start message
        msg += bytes([0]) # second byte is the packet counter. This is the start message which resets it, so should be 0
        msg += ZcBlePulseMessage(0x01, 0, 0, 0, 0, 0).pack()
        await self.client.write_gatt_char(self.characteristic_pulse, msg, response=False)
        self.pulse_start_time = self.time_us()

    async def pulse_queue(self, width_pos_us, width_neg_us, power, pulse_time_us, channel):
        pulse_time_us += (PULSE_TIME_IN_FUTURE_MS * 1000)
        pulse = ZcBlePulseMessage(0x02, width_pos_us, width_neg_us, power, pulse_time_us, channel)
        await self.pq.add_to_outbound_queue(pulse)

    def pattern_time_us(self):
        return ((time.time_ns() / 1000) - self.pulse_start_time)

    async def process_queue(self):
        await self.pq.send_if_old(self.pattern_time_us())

