import time

# Basic toggle pattern. Alternates between channels 1+2 and 3+4 every 500ms.
# Doesn't change frequency or pulse width, so defaults will be used - currently
# 150 hz and 150 us

class toggle:
    def __init__(self, ble):
        self.toggle_time_ms = 500

        self.ble = ble
        self.c12on = False
        self.next_toggle_time_ms = 0

    async def setup(self):
        await self.ble.channel_power_level(1, 1000)
        await self.ble.channel_power_level(2, 1000)
        await self.ble.channel_power_level(3, 1000)
        await self.ble.channel_power_level(4, 1000)

    async def toggle(self):
        self.c12on = not self.c12on
        await self.ble.channel_power_enable(1, self.c12on)
        await self.ble.channel_power_enable(2, self.c12on)
        await self.ble.channel_power_enable(3, not self.c12on)
        await self.ble.channel_power_enable(4, not self.c12on)

    async def loop(self):
        time_ms = time.time()*1000.0
        if time_ms > self.next_toggle_time_ms:
            await self.toggle()
            self.next_toggle_time_ms = time_ms + self.toggle_time_ms
