



# TODO: chan polarity


import asyncio
import time
import struct
import queue
import csv
from bleak import BleakClient

address = "28:CD:C1:07:90:6C" # 28:CD:C1:07:90:6C: ZC95

PULSE_TIME_IN_FUTURE_MS  = 75 # When generating a pulse and adding it to the outbound queue, set its due time this number milliseconds in the future
SEND_WHEN_TIME_WITHIN_MS = 30 # Send message with pulse(s) when the queued pulse with the earlist due time is less than this number (ms)

class BTMessagePulse:
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

    @classmethod
    def unpack(cls, data):
        unpacked_data = struct.unpack('<BBBHQB', data)
        return cls(*unpacked_data)

# Handle outbound message queue, and sending of packets either when the queue is full (happens at higher
# frequenices), or when the first entry in the queue becomes old enough to have a timestamp only a few
# milliseconds in the future (happens at lower frequenices)
class PulseQueue:
    def __init__(self, bleak_client, pulse_char):
        self.first_pulse_in_queue_us = 0
        self.pulse_count_in_queue = 0
        self.packets_sent = 0
        self.pulse_queue = queue.Queue(18) # => (18 * 13 bytes) + 2 bytes = up to 236 bytes per packet (using DLE)
        self.bleak_client = bleak_client
        self.pulse_char = pulse_char
        self.packet_count = 1 # assume start has already been sent

    # If the first pulse in the queue is due in x ms or less, send the queue now - don't wait for it to be full
    async def send_if_old(self, pattern_time_us):
        if self.pulse_count_in_queue > 0 :
            if self.first_pulse_in_queue_us - pattern_time_us < (SEND_WHEN_TIME_WITHIN_MS * 1000):
                await self.send_pulses()

    # Send all queued pulses now
    async def send_pulses(self):
        # print("Send: " + str(self.pulse_count_in_queue))
        packet  = bytes([self.pulse_count_in_queue]) # first byte of packet is number of messages
        packet += bytes([(self.packet_count % 256)]) # second byte is an 8bit packet counter; warpping around to 0 after 255. Used by ZC95 to spot missed/dropped packets
        self.packet_count += 1
        self.pulse_count_in_queue = 0

        while not self.pulse_queue.empty():
            packet += self.pulse_queue.get().pack()

        await self.bleak_client.write_gatt_char(self.pulse_char, packet, response=False)
        self.packets_sent += 1

    # Add a pulse message to the outbound queue. If that makes the outbound queue full, send it
    async def add_to_outbound_queue(self, pulse):
        if self.pulse_count_in_queue == 0:
            self.first_pulse_in_queue_us = pulse.time_us

        self.pulse_count_in_queue += 1
        self.pulse_queue.put(pulse)

        if (self.pulse_queue.full()):
            await self.send_pulses()


async def main(address):
    pulse_list = []

    with open("Toggle.zc95.csv", mode='r', newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            pulse_list.append(row)

    async with BleakClient(address) as client:

        def time_us():
            return time.time_ns() / 1000

        def pattern_time_us(start_time_us):
            return ((time.time_ns() / 1000) - start_time_us)


        async def timer_func():
            print("Send start")
            # Start message (more of a clock sync message). Tells the ZC95 that all future messages will have a time_us relative to now.
            msg  = bytes([1]) # first packet being sent only has one message - the start message
            msg += bytes([0]) # second byte is the packet counter. This is the start message which resets it, so should be 0
            msg += BTMessagePulse(0x01, 0, 0, 0, 0, 0).pack()
            await client.write_gatt_char(pulse_char, msg, response=False)
            start_time_us = time_us()

            message_time_offset = 0
            pulse_index = 0
            next_pulse = pulse_list[pulse_index]

            print("Start pulses")
            pq = PulseQueue(client, pulse_char)

            # debug counters
            packet_count = 0
            message_count = 0
            last_debug_msg_us = time_us()

            while True:
                pulse = BTMessagePulse(0x02, int(pulse_list[pulse_index]['pos_width_us']),int(pulse_list[pulse_index]['pos_width_us']),  int(pulse_list[pulse_index]['power']), message_time_offset+int(pulse_list[pulse_index]['time_us'])+(PULSE_TIME_IN_FUTURE_MS * 1000), int(pulse_list[pulse_index]['channel']))  # pulse x ms from now
                await pq.add_to_outbound_queue(pulse)
                message_count += 1

                pulse_index += 1
                if pulse_index >= len(pulse_list):
                    message_time_offset += int(pulse_list[pulse_index-1]['time_us'])
                    pulse_index = 0
                    #print("Send start " + str(time_us()))
                    ## Start message (more of a clock sync message). Tells the ZC95 that all future messages will have a time_us relative to now.
                    #msg  = bytes([1]) # first packet being sent only has one message - the start message
                    #msg += bytes([0]) # second byte is the packet counter. This is the start message which resets it, so should be 0
                    #msg += BTMessagePulse(0x01, 0, 0, 0, 0).pack()
                    #await client.write_gatt_char(pulse_char, msg, response=False)
                    #start_time_us = time_us()

                # debug output
                if (time_us() - last_debug_msg_us > 1000 * 1000):
                    time_s = (time_us() - last_debug_msg_us)/(1000*1000)
                    msg_sec = message_count/time_s
                    packet_sec = (pq.packets_sent - packet_count) / time_s
                    print("{:.2f}".format(msg_sec) + ' messages/sec, ' + "{:.2f}".format(packet_sec) + ' packets/sec (average ' + "{:.0f}".format(msg_sec / packet_sec) + ' messages/packet)')
                    last_debug_msg_us = time_us()
                    message_count = 0
                    packet_count = pq.packets_sent

                while (int(pulse_list[pulse_index]['time_us'])+message_time_offset > pattern_time_us(start_time_us)):
                    await pq.send_if_old(pattern_time_us(start_time_us))
                    await asyncio.sleep(0)

        nus = client.services.get_service("AC7744C0-0BAD-11EF-A9CD-0800200C9A00")
        pulse_char = nus.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9A01")

        gen_control_service = client.services.get_service("AC7744C0-0BAD-11EF-A9CD-0800200C9B00")
        characteristic_channel_power_level = {}
        characteristic_channel_power_level[1] = gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B31")
        characteristic_channel_power_level[2] = gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B32")
        characteristic_channel_power_level[3] = gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B33")
        characteristic_channel_power_level[4] = gen_control_service.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9B34")

        await client.write_gatt_char(characteristic_channel_power_level[1], struct.pack('>H', 1000), response=False)
        await client.write_gatt_char(characteristic_channel_power_level[2], struct.pack('>H', 1000), response=False)
        await client.write_gatt_char(characteristic_channel_power_level[3], struct.pack('>H', 1000), response=False)
        await client.write_gatt_char(characteristic_channel_power_level[4], struct.pack('>H', 1000), response=False)


        await timer_func()


asyncio.run(main(address))
