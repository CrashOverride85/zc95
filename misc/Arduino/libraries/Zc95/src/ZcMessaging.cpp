#include "ZcMessaging.h"
#include <Arduino.h>

ZcMessaging::ZcMessaging(ZcConnection *zc_connection, bool debug)
{
  _msgId = 0;
  _debug = debug;
  _zc_connection = zc_connection;
}

void ZcMessaging::Send(const JsonDocument &doc)
{
  std::string msgToSend;
  serializeJson(doc, msgToSend);
  _zc_connection->send(msgToSend);
}

JsonObject ZcMessaging::GetResponse(int expectedMsgId, const char *expectedType)
{
  std::string resultJson = _zc_connection->recv(expectedMsgId);
  DynamicJsonDocument doc(2048);
  deserializeJson(doc, resultJson);

  JsonObject result = doc.as<JsonObject>();

  if (result == JsonObject() || !result.containsKey("Type"))
  {
    if (_debug)
    {
      Serial.println("Didn't get any message, expected " + String(expectedType) + " (_msgId = " + String(expectedMsgId) + ")");
    }
    return JsonObject();
  }

  if (strcmp(result["Type"], expectedType) != 0)
  {
    if (_debug)
    {
      Serial.println("Didn't get expected " + String(expectedType) + " message type (_msgId = " + String(expectedMsgId) + ")");
    }
    return JsonObject();
  }

  if (result["MsgId"] != expectedMsgId)
  {
    if (_debug)
    {
      Serial.println("Unexpected MsgId received (expected _msgId = " + String(expectedMsgId) + ")");
    }
    return JsonObject();
  }

  if (!result.containsKey("Result") || strcmp(result["Result"], "OK") != 0)
  {
    if (result.containsKey("Error"))
    {
      if (result["Error"] == nullptr)
      {
        if (_debug)
        {
          Serial.println("Unknown error.");
        }
      }
      else
      {
        if (_debug)
        {
          Serial.println("Got error message: ");
          //    Serial.println("    " + String(result["Error"]));
        }
      }
    }
    else
    {
      if (_debug)
      {
        Serial.println("Result not OK");
      }
    }
    return JsonObject();
  }

  return result;
}

JsonArray ZcMessaging::GetPatterns()
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "GetPatterns";
  doc["MsgId"] = _msgId;
  Send(doc);
  JsonObject response = GetResponse(_msgId, "PatternList");

  return response["Patterns"].as<JsonArray>();
}

JsonObject ZcMessaging::GetPatternDetails(int pattern_id)
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "GetPatternDetail";
  doc["MsgId"] = _msgId;
  doc["Id"] = pattern_id;
  Send(doc);
  JsonObject response = GetResponse(_msgId, "PatternDetail");

  if (_debug)
  {
    String jsonString;
    serializeJson(response, jsonString);
    Serial.println(jsonString);
  }

  return response;
}

void ZcMessaging::PatternStart(int pattern_id)
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "PatternStart";
  doc["MsgId"] = _msgId;
  doc["Index"] = pattern_id;
  Send(doc);
  GetResponse(_msgId, "Ack");
}

void ZcMessaging::PatternMinMaxChange(int menu_id, int new_value)
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "PatternMinMaxChange";
  doc["MsgId"] = _msgId;
  doc["MenuId"] = menu_id;
  doc["NewValue"] = new_value;
  Send(doc);
  GetResponse(_msgId, "Ack");
}

void ZcMessaging::PatternMultiChoiceChange(int menu_id, int choice_id)
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "PatternMultiChoiceChange";
  doc["MsgId"] = _msgId;
  doc["MenuId"] = menu_id;
  doc["ChoiceId"] = choice_id;
  Send(doc);
  GetResponse(_msgId, "Ack");
}

void ZcMessaging::PatternSoftButton(bool pressed)
{
  _msgId++;
  int pressed_val = pressed ? 1 : 0;
  StaticJsonDocument<300> doc;
  doc["Type"] = "PatternSoftButton";
  doc["MsgId"] = _msgId;
  doc["Pressed"] = pressed_val;
  Send(doc);
  GetResponse(_msgId, "Ack");
}

void ZcMessaging::SendSetPowerMessage(int chan1, int chan2, int chan3, int chan4)
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "SetPower";
  doc["MsgId"] = _msgId;
  doc["Chan1"] = chan1;
  doc["Chan2"] = chan2;
  doc["Chan3"] = chan3;
  doc["Chan4"] = chan4;
  Send(doc);
  GetResponse(_msgId, "Ack");
}

void ZcMessaging::SendPatternStopMessage()
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "PatternStop";
  doc["MsgId"] = _msgId;
  Send(doc);
  GetResponse(_msgId, "Ack");
}

JsonObject ZcMessaging::GetVersionDetails()
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "GetVersion";
  doc["MsgId"] = _msgId;
  Send(doc);
  JsonObject response = GetResponse(_msgId, "VersionDetails");

  return response;
}

JsonObject ZcMessaging::SendLuaStart(int index)
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "LuaStart";
  doc["Index"] = index;
  doc["MsgId"] = _msgId;
  Send(doc);
  return GetResponse(_msgId, "Ack");
}

JsonObject ZcMessaging::SendLuaLine(int lineNumber, const char *text)
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "LuaLine";
  doc["LineNumber"] = lineNumber;
  doc["Text"] = text;
  doc["MsgId"] = _msgId;
  Send(doc);
  return GetResponse(_msgId, "Ack");
}

JsonObject ZcMessaging::SendLuaEnd()
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "LuaEnd";
  doc["MsgId"] = _msgId;
  Send(doc);
  return GetResponse(_msgId, "Ack");
}

JsonArray ZcMessaging::SendGetLuaScripts()
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "GetLuaScripts";
  doc["MsgId"] = _msgId;
  Send(doc);
  JsonObject response = GetResponse(_msgId, "LuaScripts");

  return response["Scripts"].as<JsonArray>();
}

JsonObject ZcMessaging::SendDeleteLuaScript(int scriptIndexToDelete)
{
  _msgId++;
  StaticJsonDocument<300> doc;
  doc["Type"] = "DeleteLuaScript";
  doc["MsgId"] = _msgId;
  doc["Index"] = scriptIndexToDelete;
  Send(doc);
  return GetResponse(_msgId, "Ack");
}
