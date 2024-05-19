import asyncio
import time
import struct
from bleak import BleakClient

address = "28:CD:C1:07:90:6C" # 28:CD:C1:07:90:6C: ZC95
# MODEL_NBR_UUID = "2A24"



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


async def main(address):
    async with BleakClient(address) as client:

        def time_us():
            return time.time_ns() / 1000

        def pattern_time_us(start_time_us):
            return int((time.time_ns() / 1000) - start_time_us)

        async def timer_func():
            print("Send start")
            # start message (clock sync)

            msg = BTMessagePulse(0x01, 0, 0, 0, 0)
            await client.write_gatt_char(pulse_char, msg.pack(), response=False)
            start_time_us = time_us()

            print("Start pulses")
            pulse_interval_ms = 10
            next_pulse_us = (1000 * pulse_interval_ms)

            packet_count = 0
            last_debug_msg = 0

            while True:
                msg = BTMessagePulse(0x02, 130, 0, int(next_pulse_us)+(20 * 1000), 0)  # pulse 40ms from now
                await client.write_gatt_char(pulse_char, msg.pack(), response=False)
                packet_count += 1
                next_pulse_us = next_pulse_us + (1000 * pulse_interval_ms)
            #    print(".")

                if (time_us() - last_debug_msg > 1000 * 1000):
                    print('pck=' + str(packet_count) + ', ms=' + str((time_us() - last_debug_msg)/1000))
                    last_debug_msg = time_us()
                    packet_count = 0


                while (next_pulse_us > pattern_time_us(start_time_us)):
                    await asyncio.sleep(0)

        nus = client.services.get_service("AC7744C0-0BAD-11EF-A9CD-0800200C9A00")
        pulse_char = nus.get_characteristic("AC7744C0-0BAD-11EF-A9CD-0800200C9A01")

        # model_number = await client.read_gatt_char(MODEL_NBR_UUID)
        # print("Model Number: {0}".format("".join(map(chr, model_number))))
        await timer_func()


asyncio.run(main(address))
