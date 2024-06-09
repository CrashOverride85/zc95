import time

# Loosely based on the "intense" pattern of a 312 box, as documented by Onwrikbaar on
# Joanne's Estim discord (#neostim-chat, 04/06/2024 18:45).
# Channels 1+2 are on constant at 40hz, 130us pulse width. On the 312, the frequency
# could be changed by the MA knob - this is self.freq_hz below. Channels 3+4 use the
# same frequency & pulse width as 1+2, but toggles between the two - switching every 250ms

class intense:
    def __init__(self, ble):
        self.freq_hz = 40 # as would be changed by MA knob (approx 15 - 125)
        self.toggle_period_ms = 250

        self.ble = ble
        self.c3on = False
        self.next_toggle_time_ms = 0

    async def setup(self):
        await self.ble.all_channel_power_level(1000)
        await self.ble.all_channel_frequency(self.freq_hz)
        await self.ble.all_channel_pulse_width(130, 130)

        await self.ble.channel_power_enable(1, self.c3on)
        await self.ble.channel_power_enable(2, self.c3on)

    async def toggle(self):
        self.c3on = not self.c3on
        await self.ble.channel_power_enable(3, self.c3on)
        await self.ble.channel_power_enable(4, not self.c3on)

    async def loop(self):
        time_ms = time.time()*1000.0
        if time_ms > self.next_toggle_time_ms:
            await self.toggle()
            self.next_toggle_time_ms = time_ms + self.toggle_period_ms
