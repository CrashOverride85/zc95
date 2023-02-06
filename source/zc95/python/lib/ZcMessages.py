import websocket # pip3 install websocket-client
import time
import json 

class ZcMessages:
  def __init__(self, websocket, debug):
    self.ws = websocket
    self.msgId = 1
    self.debug = debug
  
  def Send(self, message):
    msgToSend = json.dumps(message);  
    self.ws.send(msgToSend)

  def GetResponse(self, expectedMsgId, expectedType):
    resultJson = self.ws.recv(expectedMsgId)
    
    if resultJson == None:
      print("Didn't get any message, expected " + expectedType + " (msgId = " + str(expectedMsgId) + ")" )
      return      
    
    result = json.loads(resultJson)
    if result["Type"] != expectedType:
      print("Didn't get expected " + expectedType + " message type (msgId = " + str(expectedMsgId) + ")" )
      return

    if result["MsgId"] != expectedMsgId:
      print("Unexpected MsgId received (expected msgId = " + str(expectedMsgId) + ")" )
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
    self.msgId = self.msgId + 1
    msgGetLuaScripts = {
      "Type": "GetPatterns",
      "MsgId": self.msgId
    }
    self.Send(msgGetLuaScripts)
    expectedType = "PatternList"
    response = self.GetResponse(self.msgId, expectedType)
    
    return response["Patterns"]

  def GetPatternDetails(self, pattern_id):
    self.msgId = self.msgId + 1
    msgGetPatternDetail = {
      "Type": "GetPatternDetail",
      "MsgId": self.msgId,
      "Id": pattern_id
    }
    self.Send(msgGetPatternDetail)
    expectedType = "PatternDetail"
    response = self.GetResponse(self.msgId, expectedType)
    
    if self.debug:
      print(json.dumps(response, indent=4))
    return response
  
  def PatternStart(self, pattern_id):
    self.msgId = self.msgId + 1
    msgPatternStart = {
      "Type": "PatternStart",
      "MsgId": self.msgId,
      "Index": pattern_id
    }
    self.Send(msgPatternStart)
    self.GetResponse(self.msgId, "Ack")

  def PatternMinMaxChange(self, menu_id, new_value):
    self.msgId = self.msgId + 1
    msgPatternMinMaxChange = {
      "Type": "PatternMinMaxChange",
      "MsgId": self.msgId,
      "MenuId": menu_id,
      "NewValue": new_value
    }
    self.Send(msgPatternMinMaxChange)
    self.GetResponse(self.msgId, "Ack")

  def PatternMultiChoiceChange(self, menu_id, choice_id):
    self.msgId = self.msgId + 1
    msgPatternMultiChoiceChange = {
      "Type": "PatternMultiChoiceChange",
      "MsgId": self.msgId,
      "MenuId": menu_id,
      "ChoiceId": choice_id
    }
    self.Send(msgPatternMultiChoiceChange)
    self.GetResponse(self.msgId, "Ack")
  
  def PatternSoftButton(self, pressed):
    self.msgId = self.msgId + 1
    
    if pressed:
      pressed_val = 1 # pressed
    else:
      pressed_val = 0 # released
    
    msgPatternSoftButton = {
      "Type": "PatternSoftButton",
      "MsgId": self.msgId,
      "Pressed": pressed_val
    }
    self.Send(msgPatternSoftButton)
    self.GetResponse(self.msgId, "Ack")
    
  def SendSetPowerMessage(self, chan1, chan2, chan3, chan4):
    self.msgId = self.msgId + 1

    msgSetPower = {
      "Type": "SetPower",
      "MsgId": self.msgId,
      "Chan1": chan1,
      "Chan2": chan2,
      "Chan3": chan3,
      "Chan4": chan4
    }
    self.Send(msgSetPower)
    self.GetResponse(self.msgId, "Ack")
    
  def SendPatternStopMessage(self):
    self.msgId = self.msgId + 1

    msgStopPattern = {
      "Type": "PatternStop",
      "MsgId": self.msgId
    }
    self.Send(msgStopPattern)
    self.GetResponse(self.msgId, "Ack")

  def GetVersionDetails(self):
    self.msgId = self.msgId + 1
    msgVersionDetails = {
      "Type": "GetVersion",
      "MsgId": self.msgId
    }
    self.Send(msgVersionDetails)
    expectedType = "VersionDetails"
    response = self.GetResponse(self.msgId, expectedType)
    return response
  
  def SendLuaStart(self, index):
    self.msgId = self.msgId + 1
    msgStart = {
        "Type": "LuaStart",
        "Index": index,
        "MsgId": self.msgId
    }
    self.Send(msgStart)
    return self.GetResponse(self.msgId, "Ack")
  
  def SendLuaLine(self, lineNumber, text):
    self.msgId = self.msgId + 1
    msgLuaLine = {
        "Type": "LuaLine",
        "LineNumber": lineNumber,
        "Text": text.rstrip(),
        "MsgId": self.msgId
    }
    self.Send(msgLuaLine)
    return self.GetResponse(self.msgId, "Ack")
    
  def SendLuaEnd(self):
    self.msgId = self.msgId + 1
    msgEnd = {
        "Type": "LuaEnd",
        "MsgId": self.msgId
    }
    self.Send(msgEnd)
    return self.GetResponse(self.msgId, "Ack")
  
  def SendGetLuaScripts(self):
    self.msgId = self.msgId + 1
    msgGetLuaScripts = {
      "Type": "GetLuaScripts",
      "MsgId": self.msgId
    }
    self.Send(msgGetLuaScripts)
    return self.GetResponse(self.msgId, "LuaScripts")["Scripts"]
    
  def SendDeleteLuaScript(self, scriptIndexToDelete):
    self.msgId = self.msgId + 1
    msgDeleteLuaScript = {
      "Type": "DeleteLuaScript",
      "MsgId": self.msgId,
      "Index": scriptIndexToDelete
    }
    self.Send(msgDeleteLuaScript)
    return self.GetResponse(self.msgId, "Ack")
