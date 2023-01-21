import websocket # pip3 install websocket-client
import time
import json 

class ZcPatterns:
  def __init__(self, websocket):
    self.ws = websocket
  
  def Send(self, message):
    msgToSend = json.dumps(message);  
    #if args.debug:
    if True:
      print("> " + msgToSend)
    self.ws.send(msgToSend)


  def GetResponse(self, expectedMsgCount, expectedType):
    resultJson = self.ws.recv()
    #if args.debug:  
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
    msgCount = 1
    msgGetLuaScripts = {
      "Type": "GetPatterns",
      "MsgCount": msgCount
    }
    self.Send(msgGetLuaScripts)
    expectedType = "PatternList"
    response = self.GetResponse(msgCount, expectedType)
    
    patterns = response["Patterns"]
    print("Patterns on ZC95:")
    for pattern in patterns:
      print(str(pattern["Id"]) + "\t" + pattern["Name"])
    
  def GetPatternDetails(self, pattern_id):
    msgCount = 1
    msgGetLuaScripts = {
      "Type": "GetPatternDetail",
      "MsgCount": msgCount,
      "Id": pattern_id
    }
    self.Send(msgGetLuaScripts)
    expectedType = "PatternDetail"
    response = self.GetResponse(msgCount, expectedType)
    
    print(json.dumps(response, indent=4))
    return response
