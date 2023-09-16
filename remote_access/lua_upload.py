import websocket # pip3 install websocket-client
import argparse
import time
import json 
import threading
import queue
import lib.ZcMessages as zc
from lib.ZcWs import ZcWs
from lib.ZcSerial import ZcSerial
 
def ExitWithError(zcws, error):
  zcws.stop()
  quit(error)

def OutputErrorMessagesReceived(rcv_queue):
  while not rcv_queue.empty():
      message = rcv_queue.get_nowait()
      if message["Result"] == "ERROR" and message["MsgId"] == -1 and message["Error"] != None:
          print("Got error: " + message["Error"])
 
parser = argparse.ArgumentParser(description='Upload Lua scripts to ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')

connection_group = parser.add_mutually_exclusive_group(required=True)
connection_group.add_argument('--ip', action='store', help='IP address of ZC95')
connection_group.add_argument('--serial', action='store', help='Serial port to use')

parser.add_argument('--index', action='store', required=True, type=int, choices=range(0, 5), help='Slot/index on ZC95 to upload script to')
parser.add_argument('--script', action='store', required=True, help='Lua script to upload')
args = parser.parse_args()

# Open script
try:
  luaFile = open(args.script, "r")
except OSError:
  quit("Failed to open [" + args.script + "]")

# Connect
rcv_queue = queue.Queue() 

if args.serial:
  zc_connection = ZcSerial(args.serial, rcv_queue, args.debug)
else:
  zc_connection = ZcWs(args.ip, rcv_queue, args.debug)

conn_thread = threading.Thread(target=zc_connection.run_forever)
conn_thread.start()
zc_connection.wait_for_connection()

zc_messages = zc.ZcMessages(zc_connection, args.debug)

# Send start message with index/slot of where the new Lua script should go
if zc_messages.SendLuaStart(args.index) == None:
  ExitWithError(zc_connection, "Failed")
 
lineNumber = 0
print("Uploading...")
for luaLineString in luaFile:
  if zc_messages.SendLuaLine(lineNumber, luaLineString) == None:
      OutputErrorMessagesReceived(rcv_queue)
      ExitWithError(zc_connection, "Failed on line " + str(lineNumber+1))

  lineNumber += 1

# Finished sending message, send end message. This step may fail if the script is invalid
if zc_messages.SendLuaEnd() == None:
    ExitWithError(zc_connection, "Failed")

print("Done!")

zc_connection.stop()
