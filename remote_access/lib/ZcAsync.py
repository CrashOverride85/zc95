import asyncio
import json

import websockets  # pip3 install websockets


class ZcAsyncError(Exception):
  pass


class ZcAsync:
  """
  Asyncio websocket connection to a ZC95, plus a thin request/response
  helper for the JSON message protocol (see docs/JSON.md).

  Unlike ZcWs.py (which is thread based, for use with tkinter), this is
  meant to be used from a single asyncio event loop, e.g. within a FastAPI
  app - see webgui.py.
  """

  def __init__(self, ip, debug=False):
    self.ip = ip
    self.debug = debug
    self._ws = None
    self._msg_id = 0
    self._pending = {}  # MsgId -> asyncio.Future
    self._reader_task = None
    self._unsolicited_handler = None

  def set_unsolicited_handler(self, handler):
    """handler(message: dict) is called for messages from the ZC95 with MsgId == -1"""
    self._unsolicited_handler = handler

  async def connect(self):
    self._ws = await websockets.connect("ws://" + self.ip + "/stream", ping_interval=6)
    self._reader_task = asyncio.create_task(self._reader())

  async def close(self):
    if self._reader_task is not None:
      self._reader_task.cancel()
    if self._ws is not None:
      await self._ws.close()

  async def _reader(self):
    try:
      async for raw in self._ws:
        if self.debug:
          print("< " + raw)

        message = json.loads(raw)
        msg_id = message.get("MsgId")

        if msg_id in self._pending:
          self._pending.pop(msg_id).set_result(message)
        elif self._unsolicited_handler is not None:
          self._unsolicited_handler(message)
    except websockets.ConnectionClosed:
      pass
    finally:
      # Unblock anything still waiting on a response - the connection is gone
      for future in self._pending.values():
        if not future.done():
          future.set_exception(ZcAsyncError("Connection to ZC95 closed"))
      self._pending.clear()

  async def request(self, message, expected_type, timeout=6.0):
    """Send a message, and wait for the matching response. Raises ZcAsyncError on failure/timeout."""
    self._msg_id += 1
    message["MsgId"] = self._msg_id

    future = asyncio.get_running_loop().create_future()
    self._pending[self._msg_id] = future

    raw = json.dumps(message)
    if self.debug:
      print("> " + raw)

    try:
      await self._ws.send(raw)
    except websockets.ConnectionClosed as e:
      self._pending.pop(self._msg_id, None)
      raise ZcAsyncError("Connection to ZC95 closed") from e

    try:
      result = await asyncio.wait_for(future, timeout=timeout)
    except asyncio.TimeoutError:
      self._pending.pop(self._msg_id, None)
      raise ZcAsyncError("Timed out waiting for " + expected_type + " response")

    if result.get("Type") != expected_type:
      raise ZcAsyncError("Expected " + expected_type + " message, got " + str(result.get("Type")))

    if result.get("Result") != "OK":
      raise ZcAsyncError(result.get("Error") or "Result not OK")

    return result
