import asyncio
import time
import struct
import queue
from bleak import BleakClient

address = "28:CD:C1:07:90:6C" # 28:CD:C1:07:90:6C: ZC95

PULSE_TIME_IN_FUTURE_MS  = 75 # When generating a pulse and adding it to the outbound queue, set its due time this number milliseconds in the future
SEND_WHEN_TIME_WITHIN_MS = 40 # Send message with pulse(s) when the queued pulse with the earlist due time is less than this number (ms)

class BTMessagePulse:
    def __init__(self, cmd_type, pulse_width, amplitude, time_us, channel_polarity):
        self.cmd_type = cmd_type
        self.pulse_width = pulse_width
        self.amplitude = amplitude
        self.time_us = time_us
        self.channel_polarity = channel_polarity

    def pack(self):
        return struct.pack('<BBHQB', self.cmd_type, self.pulse_width, self.amplitude, self.time_us, self.channel_polarity)

    @classmethod
    def unpack(cls, data):
        unpacked_data = struct.unpack('<BBHQB', data)
        return cls(*unpacked_data)

# Handle outbound message queue, and sending of packets either when the queue is full (happens at higher
# frequenices), or when the first entry in the queue becomes old enough to have a timestamp only a few
# milliseconds in the future (happens at lower frequenices)
class PulseQueue:
    def __init__(self, start_time_us, bleak_client, pulse_char):
        self.first_pulse_in_queue_us = 0
        self.pulse_count_in_queue = 0
        self.packets_sent = 0
        self.start_time_us = start_time_us
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
            msg += BTMessagePulse(0x01, 0, 0, 0, 0).pack()
            await client.write_gatt_char(pulse_char, msg, response=False)
            start_time_us = time_us()

            print("Start pulses")
            pulse_interval_ms = 5
            next_pulse_us = (1000 * pulse_interval_ms)
            pq = PulseQueue(start_time_us, client, pulse_char)

            # debug counters
            packet_count = 0
            message_count = 0
            last_debug_msg_us = time_us()

            while True:
                pulse = BTMessagePulse(0x02, 130, 1000, int(next_pulse_us)+(PULSE_TIME_IN_FUTURE_MS * 1000), 0xFF)  # pulse x ms from now
                await pq.add_to_outbound_queue(pulse)
                message_count += 1
                next_pulse_us = next_pulse_us + (1000 * pulse_interval_ms)

                # debug output
                if (time_us() - last_debug_msg_us > 1000 * 1000):
                    time_s = (time_us() - last_debug_msg_us)/(1000*1000)
                    msg_sec = message_count/time_s
                    packet_sec = (pq.packets_sent - packet_count) / time_s
                    print("{:.2f}".format(msg_sec) + ' messages/sec, ' + "{:.2f}".format(packet_sec) + ' packets/sec (average ' + "{:.0f}".format(msg_sec / packet_sec) + ' messages/packet)')
                    last_debug_msg_us = time_us()
                    message_count = 0
                    packet_count = pq.packets_sent

                while (next_pulse_us > pattern_time_us(start_time_us)):
                    await pq.send_if_old(pattern_time_us(start_time_us))
                    await asyncio.sleep(0)

        nus = client.services.get_service("AC7744C0-0BAD-11EF-A9CD-0800200C9A00")
        pulse_char = nus.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9A01")

        await timer_func()


asyncio.run(main(address))
