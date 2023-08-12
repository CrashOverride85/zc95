import argparse
import threading
import websocket
import lib.ZcMessages as zc
import queue
from lib.ZcWs import ZcWs
from lib.ZcSerial import ZcSerial

parser = argparse.ArgumentParser(description='Manage Lua scripts on ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')

connection_group = parser.add_mutually_exclusive_group(required=True)
connection_group.add_argument('--ip', action='store', help='IP address of ZC95')
connection_group.add_argument('--serial', action='store', help='Serial port to use')

parser.add_argument('--list', action='store_true', help='List scripts stored on ZC95')
parser.add_argument('--delete', action='store', type=int, choices=range(1, 6), help='Delete script at slot on ZC95')

args = parser.parse_args()

rcv_queue = queue.Queue() 

# Connect either using serial or websocket
if args.serial:
  zc_connection = ZcSerial(args.serial, rcv_queue, args.debug)
else:
  zc_connection = ZcWs(args.ip, rcv_queue, args.debug)

conn_thread = threading.Thread(target=zc_connection.run_forever)
conn_thread.start()
zc_connection.wait_for_connection()

zc_messages = zc.ZcMessages(zc_connection, args.debug)

if args.list:
  scripts = zc_messages.SendGetLuaScripts()
  print("Script slots on ZC95:")
  
  for script in scripts:
    print(str(script["Index"]) + " - " + script["Name"])

elif args.delete:
  if zc_messages.SendDeleteLuaScript(args.delete) == None:
    print("Failed!")
  else:
    print("Done.")

zc_connection.stop()

