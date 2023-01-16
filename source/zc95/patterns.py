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

  if "Result" not in result or result["Result"] != "OK":
    if "Error" in result:
      print("Got error message: ")
      print("    " + result["Error"])
    else:
      print("Result not OK")

    quit()
    
  return result

def GetPatterns():
  msgCount = 1
  msgGetLuaScripts = {
    "Type": "GetPatterns",
    "MsgCount": msgCount
  }
  Send(msgGetLuaScripts)
  expectedType = "PatternList"
  response = GetResponse(msgCount, expectedType)
  
  patterns = response["Patterns"]
  print("Patterns on ZC95:")
  for pattern in patterns:
    print(str(pattern["Id"]) + "\t" + pattern["Name"])
  
def GetPatternDetails(pattern_id):
  msgCount = 1
  msgGetLuaScripts = {
    "Type": "GetPatternDetail",
    "MsgCount": msgCount,
    "Id": pattern_id
  }
  Send(msgGetLuaScripts)
  expectedType = "PatternDetail"
  response = GetResponse(msgCount, expectedType)
  
  print(json.dumps(response, indent=4))
  
parser = argparse.ArgumentParser(description='Get pattern info from ZC95')
parser.add_argument('--debug', action='store_true', help='Show debugging information')
parser.add_argument('--ip', action='store', required=True, help='IP address of ZC95')
parser.add_argument('--list', action='store_true', help='List patterns on ZC95')
parser.add_argument('--pattern', action='store', type=int, help='Get pattern details for specified id')

args = parser.parse_args()

ws = websocket.WebSocket()
print("Connecting")
ws.connect("ws://" + args.ip + "/stream")

if args.list:
  GetPatterns()
elif args.pattern:
  GetPatternDetails(args.pattern)





