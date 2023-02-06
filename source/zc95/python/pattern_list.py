import argparse
import threading
import websocket
import lib.ZcMessages as zc
import queue
from lib.ZcWs import ZcWs
 
parser = argparse.ArgumentParser(description='List patterns on ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')
parser.add_argument('--ip', action='store', required=True, help='IP address of ZC95')
args = parser.parse_args()

# Websocket setup / connect
rcv_queue = queue.Queue() 
zcws = ZcWs(args.ip, rcv_queue, args.debug)
ws_thread = threading.Thread(target=zcws.run_forever)
ws_thread.start()
zcws.wait_for_connection()

## get patterns
zc_messages = zc.ZcMessages(zcws, args.debug)
patterns = zc_messages.GetPatterns()
print("Patterns on ZC95:")
for pattern in patterns:
  print(str(pattern["Id"]) + "\t" + pattern["Name"])

zcws.stop()
