#include <ZcControl.h>

ZcControl _zc(&Serial2, true); // Use serial2 for comms with zc95, and enable debugging output

void setup()
{
  Serial.begin(115200);

  // Connect ZC95
  _zc.connect();
  _zc.wait_for_connection();
}

void loop()
{
  unsigned long pattern_start_time_ms = millis();
  Serial.println("Start waves pattern and set channel 1 to 50%");
  _zc.start_pattern("Waves");
  _zc.set_channel_power(1, 500);

  while (millis() < pattern_start_time_ms + 10000)
  {
    delay(250);
    _zc.loop();

    Serial.print("Channel 1 front panel dial is set to: ");
    Serial.println(_zc.get_front_panel_power(1));
  }

  Serial.println("stop pattern");
  _zc.stop_pattern();

  _zc.disconnect();

  Serial.println("done.");
  while (1)
    ;
}
