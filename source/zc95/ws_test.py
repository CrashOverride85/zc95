import websocket # pip3 install websocket-client
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
  print("> " + msgToSend)
  ws.send(msgToSend)
  GetAndProcessAck(msgCount)


def GetAndProcessAck(expectedMsgCount):
  resultJson = ws.recv()
  print("< " + resultJson)
  
  result = json.loads(resultJson)
  if result["Type"] != "Ack":
    print("Didn't get expected Ack")
    quit()

  if result["MsgCount"] != expectedMsgCount:
    print("Unexpected MsgCount received")
    quit()

  if result["Result"] != "OK":
    print("Result not OK")
    quit()

ws = websocket.WebSocket()

ws.connect("ws://192.168.1.136/stream")

msgCount = 0
msgStart = {
    "Type": "LuaStart",
    "Index": 1,
    "MsgCount": msgCount
}
Send(msgStart)
msgCount += 1

luaFile = open("toggle2.lua", "r")
lineNumber = 0
for luaLineString in luaFile:
  luaLineToSend = GetLuaLine(msgCount, lineNumber, luaLineString)
  lineNumber += 1
  
  Send(luaLineToSend)
  msgCount += 1
  
  #time.sleep(0.05)


msgEnd = {
    "Type": "LuaEnd",
    "MsgCount": msgCount
}
Send(msgEnd)
msgCount += 1

