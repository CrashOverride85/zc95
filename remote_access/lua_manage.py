import argparse
import threading
import websocket
import lib.ZcMessages as zc
import queue
from lib.ZcWs import ZcWs
 
parser = argparse.ArgumentParser(description='Manage Lua scripts on ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')
parser.add_argument('--ip', action='store', required=True, help='IP address of ZC95')

parser.add_argument('--list', action='store_true', help='List scripts stored on ZC95')
parser.add_argument('--delete', action='store', type=int, choices=range(1, 6), help='Delete script at slot on ZC95')

args = parser.parse_args()


# Websocket setup / connect
rcv_queue = queue.Queue() 
zcws = ZcWs(args.ip, rcv_queue, args.debug)
ws_thread = threading.Thread(target=zcws.run_forever)
ws_thread.start()
zcws.wait_for_connection()

zc_messages = zc.ZcMessages(zcws, args.debug)

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

zcws.stop()

