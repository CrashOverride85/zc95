import time
import queue
import asyncio
from lib.ZcBle import ZcBle

on = True
async def loop():
    global on
    global ble

    await ble.channel_power_level(1, 500)
    await ble.channel_power_level(2, 700)
    await ble.channel_power_level(3, 800)
    await ble.channel_power_level(4, 1000)

    if on == True:
        on = False
    else:
        on = True

    await ble.channel_power_enable(2, on)
    await ble.channel_power_enable(3, on)

async def main():
    global on
    global ble
    ble_tx_queue = asyncio.Queue()

    ble = ZcBle(ble_tx_queue)
    # ble = ZcBle(ble_tx_queue, "28:CD:C1:07:90:6C")

    ble_task = asyncio.create_task(ble.start())


    on = True
    while not ble_task.done():
        await loop()
        # await asyncio.sleep(1)
        await asyncio.sleep(0.5)
        # time.sleep(0.004)


if __name__ == "__main__":
    asyncio.run(main())


