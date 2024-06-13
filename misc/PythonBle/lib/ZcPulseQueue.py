import queue

SEND_WHEN_TIME_WITHIN_MS = 30 # Send message with pulse(s) when the queued pulse with the earlist due time is less than this number (ms)

# Handle outbound message queue, and sending of packets either when the queue is full (happens at higher
# frequenices), or when the first entry in the queue becomes old enough to have a timestamp only a few
# milliseconds in the future (happens at lower frequenices)
class PulseQueue:
    def __init__(self, bleak_client, pulse_char):
        self.first_pulse_in_queue_us = 0
        self.pulse_count_in_queue = 0
        self.packets_sent = 0
        self.pulse_queue = queue.Queue(14) # => (14 * 16 bytes) + 4 bytes = up to 228 bytes per packet (using DLE)
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
        packet += bytes([0])                         # two reserved bytes
        packet += bytes([0])
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
