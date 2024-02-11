#include <ZcControl.h>

#include "ConnectionCredentials.h"

ZcControl _zc(zc95_address, true);

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Wait some time to connect to wifi
  Serial.println("Connecting to WiFi ");
  for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++)
  {
    Serial.print(".");
    delay(1000);
  }

  // Check if connected to wifi
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("No Wifi!");
    while (1)
      ;
  }

  Serial.println("Connected to WiFi");

  // Connect ZC95
  _zc.connect();
}

void loop()
{
  while (!_zc.is_connected())
  {
    _zc.loop();
    delay(10);
  }

  unsigned long pattern_start_time_ms = millis();
  Serial.println("Start waves pattern and set channel 1 to 50%");
  _zc.start_pattern("Waves");
  _zc.set_channel_power(1, 500);

  while (millis() < pattern_start_time_ms + 10000)
  {
    delay(250);
    _zc.loop();

    Serial.print("Channel 1 front pannel dial is set to: ");
    Serial.println(_zc.get_front_panel_power(1));
  }

  Serial.println("stop pattern");
  _zc.stop_pattern();

  _zc.disconnect();

  Serial.println("done.");
  while (1)
    ;
}
