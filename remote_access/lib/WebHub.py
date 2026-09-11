import asyncio
import json
import logging

from lib.ZcAsync import ZcAsync, ZcAsyncError

logger = logging.getLogger("webgui")


class WebHub:
  """
  Owns the single upstream websocket connection to the ZC95, and fans state
  out to any number of connected browser clients (tabs/devices), so they all
  see - and can control - the same running pattern.

  Browser clients talk a small JSON "cmd"/"type" protocol to this hub (see
  web_static/app.js), which is translated to/from the ZC95's own JSON
  protocol (docs/JSON.md).
  """

  def __init__(self, ip, debug=False):
    self.ip = ip
    self.debug = debug
    self.zc = ZcAsync(ip, debug)
    self.zc.set_unsolicited_handler(self._on_unsolicited)

    self._clients = set()

    # Last known state, sent to newly (re)connected browser clients so they sync up
    self._patterns = []
    self._pattern_detail = None
    self._last_power_status = None

    # SetPower is throttled to at most one message every 250ms, same as pattern_gui.py
    self._pending_power = None
    self._power_task = None

  async def connect(self):
    await self.zc.connect()
    self._power_task = asyncio.create_task(self._power_sender())
    self._patterns = (await self.zc.request({"Type": "GetPatterns"}, "PatternList"))["Patterns"]

  async def close(self):
    if self._power_task is not None:
      self._power_task.cancel()
    await self.zc.close()

  # ---- browser client management ----

  async def add_client(self, websocket):
    self._clients.add(websocket)
    await self._send_full_state(websocket)

  def remove_client(self, websocket):
    self._clients.discard(websocket)

  async def _send(self, websocket, message):
    await websocket.send_text(json.dumps(message))

  async def _broadcast(self, message):
    raw = json.dumps(message)
    dead = []
    for client in self._clients:
      try:
        await client.send_text(raw)
      except Exception:
        dead.append(client)
    for client in dead:
      self._clients.discard(client)

  async def _send_full_state(self, websocket):
    await self._send(websocket, {"type": "patterns", "patterns": self._patterns})
    if self._pattern_detail is not None:
      await self._send(websocket, {"type": "pattern_detail", "detail": self._pattern_detail})
    if self._last_power_status is not None:
      await self._send(websocket, {"type": "power_status", "status": self._last_power_status})

  # ---- browser -> zc95 ----

  async def handle_client_message(self, message):
    cmd = message.get("cmd")
    try:
      if cmd == "start_pattern":
        await self._start_pattern(message["id"])
      elif cmd == "stop_pattern":
        await self._stop_pattern()
      elif cmd == "set_power":
        self._pending_power = (message["chan1"], message["chan2"], message["chan3"], message["chan4"])
      elif cmd == "menu_min_max":
        await self.zc.request(
          {"Type": "PatternMinMaxChange", "MenuId": message["menu_id"], "NewValue": message["value"]}, "Ack")
      elif cmd == "menu_multi_choice":
        await self.zc.request(
          {"Type": "PatternMultiChoiceChange", "MenuId": message["menu_id"], "ChoiceId": message["choice_id"]}, "Ack")
      elif cmd == "soft_button":
        await self.zc.request(
          {"Type": "PatternSoftButton", "Pressed": 1 if message["pressed"] else 0}, "Ack")
      else:
        logger.warning("Unknown command from browser client: %s", cmd)
    except ZcAsyncError as e:
      await self._broadcast({"type": "error", "message": str(e)})

  async def _start_pattern(self, pattern_id):
    detail = await self.zc.request({"Type": "GetPatternDetail", "Id": pattern_id}, "PatternDetail")
    await self.zc.request({"Type": "PatternStart", "Index": pattern_id}, "Ack")
    self._pattern_detail = detail
    self._last_power_status = None
    await self._broadcast({"type": "pattern_detail", "detail": detail})

  async def _stop_pattern(self):
    await self.zc.request({"Type": "PatternStop"}, "Ack")
    self._pattern_detail = None
    self._last_power_status = None
    await self._broadcast({"type": "pattern_stopped"})

  async def _power_sender(self):
    while True:
      await asyncio.sleep(0.25)
      if self._pending_power is not None:
        chan1, chan2, chan3, chan4 = self._pending_power
        self._pending_power = None
        try:
          await self.zc.request(
            {"Type": "SetPower", "Chan1": chan1, "Chan2": chan2, "Chan3": chan3, "Chan4": chan4}, "Ack")
        except ZcAsyncError as e:
          await self._broadcast({"type": "error", "message": str(e)})

  # ---- zc95 -> browser (unsolicited, MsgId == -1) ----

  def _on_unsolicited(self, message):
    asyncio.create_task(self._handle_unsolicited(message))

  async def _handle_unsolicited(self, message):
    msg_type = message.get("Type")

    if msg_type == "PowerStatus":
      self._last_power_status = message
      await self._broadcast({"type": "power_status", "status": message})
    elif msg_type == "MenuOptionChanged":
      await self._broadcast({"type": "menu_option_changed", "menu_id": message["MenuId"], "value": message["Value"]})
    elif msg_type == "LuaScriptOutput":
      await self._broadcast({"type": "lua_output", "text_type": message["TextType"], "text": message["Text"]})
    elif msg_type == "LuaScriptError":
      await self._broadcast({"type": "lua_error"})
