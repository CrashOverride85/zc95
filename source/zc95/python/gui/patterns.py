import websocket # pip3 install websocket-client
import time
import json 

class ZcPatterns:
  def __init__(self, websocket, debug):
    self.ws = websocket
    self.msgCount = 1
    self.debug = debug
  
  def Send(self, message):
    msgToSend = json.dumps(message);  
    self.ws.send(msgToSend)

  def GetResponse(self, expectedMsgCount, expectedType):
    resultJson = self.ws.recv(expectedMsgCount)
    
    if resultJson == None:
      print("Didn't get any message, expected " + expectedType + " (msgId = " + str(expectedMsgCount) + ")" )
      return      
    
    result = json.loads(resultJson)
    if result["Type"] != expectedType:
      print("Didn't get expected " + expectedType + " message type (msgId = " + str(expectedMsgCount) + ")" )
      return

    if result["MsgCount"] != expectedMsgCount:
      print("Unexpected MsgCount received (expected msgId = " + str(expectedMsgCount) + ")" )
      return

    if "Result" not in result or result["Result"] != "OK":
      if "Error" in result:
        print("Got error message: ")
        print("    " + result["Error"])
      else:
        print("Result not OK")

      return
      
    return result

  def GetPatterns(self):
    self.msgCount = self.msgCount + 1
    msgGetLuaScripts = {
      "Type": "GetPatterns",
      "MsgCount": msgCount
    }
    self.Send(msgGetLuaScripts)
    expectedType = "PatternList"
    response = self.GetResponse(self.msgCount, expectedType)
    
    patterns = response["Patterns"]
    print("Patterns on ZC95:")
    for pattern in patterns:
      print(str(pattern["Id"]) + "\t" + pattern["Name"])
    
  def GetPatternDetails(self, pattern_id):
    self.msgCount = self.msgCount + 1
    msgGetPatternDetail = {
      "Type": "GetPatternDetail",
      "MsgCount": self.msgCount,
      "Id": pattern_id
    }
    self.Send(msgGetPatternDetail)
    expectedType = "PatternDetail"
    response = self.GetResponse(self.msgCount, expectedType)
    
    if self.debug:
      print(json.dumps(response, indent=4))
    return response
  
  def PatternStart(self, pattern_id):
    self.msgCount = self.msgCount + 1
    msgPatternStart = {
      "Type": "PatternStart",
      "MsgCount": self.msgCount,
      "Index": pattern_id
    }
    self.Send(msgPatternStart)
    self.GetResponse(self.msgCount, "Ack")

  def PatternMinMaxChange(self, menu_id, new_value):
    self.msgCount = self.msgCount + 1
    msgPatternMinMaxChange = {
      "Type": "PatternMinMaxChange",
      "MsgCount": self.msgCount,
      "MenuId": menu_id,
      "NewValue": new_value
    }
    self.Send(msgPatternMinMaxChange)
    self.GetResponse(self.msgCount, "Ack")

  def PatternMultiChoiceChange(self, menu_id, choice_id):
    self.msgCount = self.msgCount + 1
    msgPatternMultiChoiceChange = {
      "Type": "PatternMultiChoiceChange",
      "MsgCount": self.msgCount,
      "MenuId": menu_id,
      "ChoiceId": choice_id
    }
    self.Send(msgPatternMultiChoiceChange)
    self.GetResponse(self.msgCount, "Ack")
  

  def PatternSoftButton(self, pressed):
    self.msgCount = self.msgCount + 1
    
    if pressed:
      pressed_val = 1 # pressed
    else:
      pressed_val = 0 # released
    
    msgPatternSoftButton = {
      "Type": "PatternSoftButton",
      "MsgCount": self.msgCount,
      "Pressed": pressed_val
    }
    self.Send(msgPatternSoftButton)
    self.GetResponse(self.msgCount, "Ack")
    
  def SendSetPowerMessage(self, chan1, chan2, chan3, chan4):
    self.msgCount = self.msgCount + 1

    msgSetPower = {
      "Type": "SetPower",
      "MsgCount": self.msgCount,
      "Chan1": chan1,
      "Chan2": chan2,
      "Chan3": chan3,
      "Chan4": chan4
    }
    self.Send(msgSetPower)
    self.GetResponse(self.msgCount, "Ack")
