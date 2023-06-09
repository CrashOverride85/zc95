import time
import argparse
import threading
import websocket
import lib.ZcMessages as zc
import queue
from lib.ZcWs import ZcWs
from lib.ZcSerial import ZcSerial
 
parser = argparse.ArgumentParser(description='List patterns on ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')

connection_group = parser.add_mutually_exclusive_group(required=True)
connection_group.add_argument('--ip', action='store', help='IP address of ZC95')
connection_group.add_argument('--serial', action='store', help='Serial port to use')

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

## get patterns
zc_messages = zc.ZcMessages(zc_connection, args.debug)
patterns = zc_messages.GetPatterns()
print("Patterns on ZC95:")
for pattern in patterns:
  print(str(pattern["Id"]) + "\t" + pattern["Name"])

zc_connection.stop()
