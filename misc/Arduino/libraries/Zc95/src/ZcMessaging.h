#ifndef ZC_MESSAGES_H
#define ZC_MESSAGES_H

#include "ZcConnection.h"

#include <ArduinoJson.h>

class ZcMessaging
{
private:
  int _msgId;
  bool _debug;

  ZcConnection *_zc_connection;

public:
  ZcMessaging(ZcConnection *zc_connection, bool debug);
  void Send(const JsonDocument &doc);
  JsonObject GetResponse(int expectedMsgId, const char *expectedType);
  JsonArray GetPatterns();
  JsonObject GetPatternDetails(int pattern_id);
  void PatternStart(int pattern_id);
  void PatternMinMaxChange(int menu_id, int new_value);
  void PatternMultiChoiceChange(int menu_id, int choice_id);
  void PatternSoftButton(bool pressed);
  void SendSetPowerMessage(int chan1, int chan2, int chan3, int chan4);
  void SendPatternStopMessage();
  JsonObject GetVersionDetails();

  // Lua script upload/delete/list
  JsonArray SendGetLuaScripts();
  JsonObject SendLuaStart(int index);
  JsonObject SendLuaLine(int lineNumber, const char *text);
  JsonObject SendLuaEnd();
  JsonObject SendDeleteLuaScript(int scriptIndexToDelete);
};

#endif
