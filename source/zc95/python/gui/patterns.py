import websocket # pip3 install websocket-client
import time
import json 

class ZcPatterns:
  def __init__(self, websocket):
    self.ws = websocket
    self.msgCount = 1
  
  def Send(self, message):
    msgToSend = json.dumps(message);  
    #if args.debug:
    if True:
      print("> " + msgToSend)
    self.ws.send(msgToSend)


  def GetResponse(self, expectedMsgCount, expectedType):
    resultJson = self.ws.recv()
    #if args.debug:  
    
    if resultJson == None:
      print("Didn't get any message, expected " + expectedType)
      quit()      
    
    if True:
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
    expectedType = "Ack"
    response = self.GetResponse(self.msgCount, expectedType)

  def PatternMinMaxChange(self, menu_id, new_value):
    self.msgCount = self.msgCount + 1
    msgPatternMinMaxChange = {
      "Type": "PatternMinMaxChange",
      "MsgCount": self.msgCount,
      "MenuId": menu_id,
      "NewValue": new_value
    }
    self.Send(msgPatternMinMaxChange)

  def PatternMultiChoiceChange(self, menu_id, choice_id):
    self.msgCount = self.msgCount + 1
    msgPatternMultiChoiceChange = {
      "Type": "PatternMultiChoiceChange",
      "MsgCount": self.msgCount,
      "MenuId": menu_id,
      "ChoiceId": choice_id
    }
    self.Send(msgPatternMultiChoiceChange)
  

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
