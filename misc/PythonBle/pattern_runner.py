import time
import queue
import asyncio
import importlib
import sys
from lib.ZcBle import ZcBle

async def main():
    if len(sys.argv) != 2:
        print("Error - must specify pattern to run")
        exit()

    pattern_name = sys.argv[1]
    pattern_module = importlib.import_module("patterns." + pattern_name)
    pattern_class = getattr(pattern_module, pattern_name)

    ble = ZcBle()
    ble_task = asyncio.create_task(ble.start())

    # wait for connection or connection timeout
    while not ble.connected and not ble_task.done():
        await asyncio.sleep(0.1)

    if not ble.connected:
        return

    pattern = pattern_class(ble)
    await pattern.setup()

    while ble.connected:
        await pattern.loop()
        await asyncio.sleep(0.001)

if __name__ == "__main__":
    asyncio.run(main())
