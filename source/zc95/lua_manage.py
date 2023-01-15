import websocket # pip3 install websocket-client
import argparse
import time
import json 

#def GetLuaLine(msgCount, lineNumber, text):
#  msgLuaLine = {
#      "Type": "LuaLine",
#      "LineNumber": lineNumber,
#      "Text": text.rstrip(),
#      "MsgCount": msgCount
#  } 
#  return msgLuaLine

def Send(message):
  msgToSend = json.dumps(message);  
  if args.debug:
    print("> " + msgToSend)
  ws.send(msgToSend)
  # GetAndProcessAck(msgCount)


def GetResponse(expectedMsgCount, expectedType):
  resultJson = ws.recv()
  if args.debug:  
    print("< " + resultJson)
  
  result = json.loads(resultJson)
  if result["Type"] != expectedType:
    print("Didn't get expected " + expectedType + " message type")
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
    
  return result

def GetLuaScripts():
  msgCount = 1
  msgGetLuaScripts = {
    "Type": "GetLuaScripts",
    "MsgCount": msgCount
  }
  Send(msgGetLuaScripts)
  expectedType = "LuaScripts"
  response = GetResponse(msgCount, expectedType)
  
  scripts = response["Scripts"]
  print("Script slots on ZC95:")
  for script in scripts:
    print(str(script["Index"]) + " - " + script["Name"])
  

parser = argparse.ArgumentParser(description='Manage Lua scripts on ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')
parser.add_argument('--ip', action='store', required=True, help='IP address of ZC95')
parser.add_argument('--list', action='store_true', help='List scripts stored on ZC95')

args = parser.parse_args()

ws = websocket.WebSocket()
print("Connecting")
ws.connect("ws://" + args.ip + "/stream")

if args.list:
  GetLuaScripts()


