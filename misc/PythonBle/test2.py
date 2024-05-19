import asyncio
import time
import struct
import queue
from bleak import BleakClient

address = "28:CD:C1:07:90:6C" # 28:CD:C1:07:90:6C: ZC95

# export BLEAK_LOGGING=1
# export BLEAK_LOGGING=



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

class PulseQueue:
    def __init__(self, start_time_us, bleak_client, pulse_char):
        self.first_pulse_in_queue_us = 0
        self.pulse_count_in_queue = 0
        self.start_time_us = start_time_us
        self.pulse_queue = queue.Queue(18) # => 18 * 13 bytes + 1 byte = up to 235 bytes
        self.bleak_client = bleak_client
        self.pulse_char = pulse_char

    async def send_if_old(self, pattern_time_us):
        if self.pulse_count_in_queue > 0 :
            # print(str((self.first_pulse_in_queue_us - pattern_time_us)/1000 ) + "(" + str(self.first_pulse_in_queue_us) + " - " + str(pattern_time_us) + ")")
            if self.first_pulse_in_queue_us - pattern_time_us < (20 * 1000): # If the oldest pulse in the (unsent) queue is due in the next 20ms, send the queue now
                await self.send_pulses()

    async def send_pulses(self):
        print("Send: " + str(self.pulse_count_in_queue))
        pulses = bytes([self.pulse_count_in_queue])
        self.pulse_count_in_queue = 0

        while not self.pulse_queue.empty():
            pulses += self.pulse_queue.get().pack()

        await self.bleak_client.write_gatt_char(self.pulse_char, pulses, response=False)

    async def add_to_outbound_queue(self, pulse):
        if self.pulse_count_in_queue == 0:
            self.first_pulse_in_queue_us = pulse.time_us
            # print("Time set to: " + str(self.first_pulse_in_queue_us))

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
            # start message (clock sync)

            msg = bytes([1]) # first packet being sent only has one message - the start message
            msg += BTMessagePulse(0x01, 0, 0, 0, 0).pack()

            await client.write_gatt_char(pulse_char, msg, response=False)
            start_time_us = time_us()

            print("Start pulses")
            pulse_interval_ms = 5
            next_pulse_us = (1000 * pulse_interval_ms)
            pq = PulseQueue(start_time_us, client, pulse_char)
            packet_count = 0
            last_debug_msg = 0

            while True:
                pulse = BTMessagePulse(0x02, 130, 0, int(next_pulse_us)+(50 * 1000), 0)  # pulse x ms from now
               # await client.write_gatt_char(pulse_char, pulse.pack(), response=False)
                await pq.add_to_outbound_queue(pulse)
                packet_count += 1
                next_pulse_us = next_pulse_us + (1000 * pulse_interval_ms)
            #    print(".")

                if (time_us() - last_debug_msg > 1000 * 1000):
                    print('pck=' + str(packet_count) + ', ms=' + str((time_us() - last_debug_msg)/1000))
                    last_debug_msg = time_us()
                    packet_count = 0

                while (next_pulse_us > pattern_time_us(start_time_us)):
                    await pq.send_if_old(pattern_time_us(start_time_us))
                    await asyncio.sleep(0)

        nus = client.services.get_service("AC7744C0-0BAD-11EF-A9CD-0800200C9A00")
        pulse_char = nus.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9A01")

        # model_number = await client.read_gatt_char(MODEL_NBR_UUID)
        # print("Model Number: {0}".format("".join(map(chr, model_number))))
        await timer_func()


asyncio.run(main(address))
