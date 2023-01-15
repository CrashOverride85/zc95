import websocket # pip3 install websocket-client
import argparse
import time
import json 

def GetLuaLine(msgCount, lineNumber, text):
  msgLuaLine = {
      "Type": "LuaLine",
      "LineNumber": lineNumber,
      "Text": text.rstrip(),
      "MsgCount": msgCount
  } 
  return msgLuaLine

def Send(message):
  msgToSend = json.dumps(message);  
  if args.debug:
    print("> " + msgToSend)
  ws.send(msgToSend)
  GetAndProcessAck(msgCount)


def GetAndProcessAck(expectedMsgCount):
  resultJson = ws.recv()
  if args.debug:  
    print("< " + resultJson)
  
  result = json.loads(resultJson)
  if result["Type"] != "Ack":
    print("Didn't get expected Ack")
    quit()

  if result["MsgCount"] != expectedMsgCount:
    print("Unexpected MsgCount received")
    quit()

  if result["Result"] != "OK":
    if "Error" in result:
      print("Got error message: ")
      print("    " + result["Error"])
    else:
      print("Result not OK")

    quit()

parser = argparse.ArgumentParser(description='Upload Lua scripts on ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')
parser.add_argument('--ip', action='store', required=True, help='IP address of ZC95')
parser.add_argument('--index', action='store', required=True, type=int, choices=range(1, 6), help='Slot/index on ZC95 to upload script to')
parser.add_argument('--script', action='store', required=True, help='Lua script to upload')
args = parser.parse_args()

ws = websocket.WebSocket()
print("Connecting")
ws.connect("ws://" + args.ip + "/stream")

msgCount = 0
msgStart = {
    "Type": "LuaStart",
    "Index": args.index,
    "MsgCount": msgCount
}
Send(msgStart)
msgCount += 1

luaFile = open(args.script, "r")
lineNumber = 0
print("Connected, uploading...")
for luaLineString in luaFile:
  luaLineToSend = GetLuaLine(msgCount, lineNumber, luaLineString)
  lineNumber += 1
  
  Send(luaLineToSend)
  msgCount += 1

msgEnd = {
    "Type": "LuaEnd",
    "MsgCount": msgCount
}
Send(msgEnd)
msgCount += 1

print("Done!")