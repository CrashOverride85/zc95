import websocket # pip3 install websocket-client
import argparse
import time
import json 

def Send(message):
  msgToSend = json.dumps(message);  
  if args.debug:
    print("> " + msgToSend)
  ws.send(msgToSend)


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
  
def DeleteLuaScript(scriptIndexToDelete):
  msgCount = 1
  msgGetLuaScripts = {
    "Type": "DeleteLuaScript",
    "MsgCount": msgCount,
    "Index": scriptIndexToDelete
  }
  Send(msgGetLuaScripts)
  expectedType = "Ack"
  response = GetResponse(msgCount, expectedType)
  print("Done.")
  

parser = argparse.ArgumentParser(description='Manage Lua scripts on ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')
parser.add_argument('--ip', action='store', required=True, help='IP address of ZC95')

parser.add_argument('--list', action='store_true', help='List scripts stored on ZC95')
parser.add_argument('--delete', action='store', type=int, choices=range(1, 6), help='Delete script at slot on ZC95')

args = parser.parse_args()

ws = websocket.WebSocket()
print("Connecting")
ws.connect("ws://" + args.ip + "/stream")

if args.list:
  GetLuaScripts()
elif args.delete:
  DeleteLuaScript(args.delete)



