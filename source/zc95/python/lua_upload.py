import websocket # pip3 install websocket-client
import argparse
import time
import json 
import threading
import queue
import lib.ZcMessages as zc
from lib.ZcWs import ZcWs
 
def ExitWithError(zcws, error):
  zcws.stop()
  quit(error)
 
parser = argparse.ArgumentParser(description='Upload Lua scripts on ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')
parser.add_argument('--ip', action='store', required=True, help='IP address of ZC95')
parser.add_argument('--index', action='store', required=True, type=int, choices=range(1, 6), help='Slot/index on ZC95 to upload script to')
parser.add_argument('--script', action='store', required=True, help='Lua script to upload')
args = parser.parse_args()

# Open script
try:
  luaFile = open(args.script, "r")
except OSError:
  quit("Failed to open [" + args.script + "]")

# Websocket setup / connect
rcv_queue = queue.Queue() 
zcws = ZcWs(args.ip, rcv_queue, args.debug)
ws_thread = threading.Thread(target=zcws.run_forever)
ws_thread.start()
zcws.wait_for_connection()

zc_messages = zc.ZcMessages(zcws, args.debug)

# Send start message with index/slot of where the new Lua script should go
if zc_messages.SendLuaStart(args.index) == None:
  ExitWithError(zcws, "Failed")
 
lineNumber = 0
print("Uploading...")
for luaLineString in luaFile:
  if zc_messages.SendLuaLine(lineNumber, luaLineString) == None:
      ExitWithError(zcws, "Failed")

  lineNumber += 1

# Finished sending message, send end message. This step may fail if the script is invalid
if zc_messages.SendLuaEnd() == None:
  ExitWithError(zcws, "Failed")

print("Done!")

zcws.stop()
