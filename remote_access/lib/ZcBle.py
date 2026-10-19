import asyncio
import threading
import json
import sys

from bleak import BleakClient, BleakScanner # pip3 install bleak

# Nordic UART service
NUS_RX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e" # we write to this
NUS_TX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e" # notifies us

# As ZcSerial, but for a ZC95 in "BLE serial access" mode. run_forever() runs the asyncio loop
# bleak needs, so should be started on its own thread.
class ZcBle:
  def __init__(self, name_or_address, rcv_queue, debug, adapter=None):
    self.__name_or_address = name_or_address
    self.__scanner_args = {"adapter": adapter} if adapter else {} # e.g. hci1 (Linux)
    self.__rcv_queue = rcv_queue
    self.debug = debug

    self.__state = "IDLE"
    self.__message = bytearray()

    self.__connected = False
    self.__recv_waiting = False
    self.__pending_recv_message = ""
    self.__recv_event = threading.Event()
    self.__waiting_for_msgId = 0
    self.__connection_wait_event = threading.Event()
    self.__early_replies = {} # replies that arrived before recv() was called, by MsgId

    self.__loop = None
    self.__client = None
    self.__stop_event = None

  def run_forever(self):
    try:
      asyncio.run(self.__main())
    except Exception as e:
      print("BLE connection failed: " + (str(e) or type(e).__name__))
    finally:
      self.__connected = False
      self.__connection_wait_event.set()
      self.__recv_event.set()

  async def __main(self):
    self.__loop = asyncio.get_running_loop()
    self.__stop_event = asyncio.Event()

    print("Looking for: " + self.__name_or_address)
    if ":" in self.__name_or_address or "-" in self.__name_or_address: # MAC address, or macOS device UUID
      device = await BleakScanner.find_device_by_address(self.__name_or_address, timeout=10, **self.__scanner_args)
    else:
      device = await BleakScanner.find_device_by_name(self.__name_or_address, timeout=10, **self.__scanner_args)
    if device is None:
      print("ZC95 not found - is it in BLE serial access mode?")
      return

    async with BleakClient(device, disconnected_callback=self.__on_close, timeout=20) as client:
      self.__client = client
      await client.start_notify(NUS_TX_UUID, self.__on_data)
      print("Connection opened")
      self.__connected = True
      self.__connection_wait_event.set()
      await self.__stop_event.wait()

      try:
        await self.__write(b'\x04') # EOT, causes the zc95 to stop a pattern if one is running, and reset the connection
      except Exception:
        pass

  def __on_close(self, client):
    print("Connection closed")
    self.__stop_event.set()

  def __on_data(self, sender, data):
    for byte in data:
      if self.__state == "IDLE":
        if byte == 0x02: # STX
          self.__state = "RECV"

      elif self.__state == "RECV":
        if byte == 0x03: # ETX
          self.__on_message(self.__message.decode("utf-8"))

          # reset for next message
          self.__message = bytearray()
          self.__state = "IDLE"

        else:
          self.__message.append(byte)

  def __on_message(self, message):
    if self.debug:
      print("< " + message)

    result = json.loads(message)
    if self.__recv_waiting and "MsgId" in result and result["MsgId"] == self.__waiting_for_msgId:
        self.__pending_recv_message = message
        self.__recv_event.set()
    else:
      # send() waits for the write to complete, so a reply can arrive before recv() is called
      if result.get("MsgId", -1) > 0:
        self.__early_replies[result["MsgId"]] = message
        while len(self.__early_replies) > 20:
          del self.__early_replies[next(iter(self.__early_replies))]
      self.__rcv_queue.put(result)

  # The ZC95 doesn't support long writes, so each write has to fit in one packet
  async def __write(self, data):
    chunk = self.__client.services.get_characteristic(NUS_RX_UUID).max_write_without_response_size
    for pos in range(0, len(data), chunk):
      await self.__client.write_gatt_char(NUS_RX_UUID, data[pos:pos+chunk], response=True)

  def wait_for_connection(self):
    self.__connection_wait_event.wait()
    if not self.__connected:
      sys.exit(1)

  def send(self, message):
    if self.debug:
      print("> " + message)

    if not self.__connected:
      return

    data = b'\x02' + message.encode('utf-8') + b'\x03' # STX message ETX
    asyncio.run_coroutine_threadsafe(self.__write(data), self.__loop).result()

  # note: not at all thread safe
  def recv(self, msgId):
    if msgId in self.__early_replies:
      return self.__early_replies.pop(msgId)

    self.__waiting_for_msgId = msgId
    self.__recv_waiting = True

    if self.__recv_event.wait(timeout=6): # timeout is seconds
      retval = self.__pending_recv_message
    else:
      retval = None

    self.__recv_waiting = False
    self.__recv_event.clear()
    self.__waiting_for_msgId = 0

    return retval

  def stop(self):
    if self.__stop_event is not None:
      self.__loop.call_soon_threadsafe(self.__stop_event.set)
