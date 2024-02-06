/*
This is sample code for the AirGradient library with a minimal implementation to read CO2 values from the SenseAir S8 sensor.

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include <AirGradient.h>

#ifdef ESP8266
AirGradient ag = AirGradient(DIY_BASIC);
#else
/** Create airgradient instance for 'OPEN_AIR_OUTDOOR' board */
AirGradient ag = AirGradient(OPEN_AIR_OUTDOOR);
#endif

void failedHandler(String msg);

void setup()
{
  Serial.begin(115200);

  /** Init CO2 sensor */
#ifdef ESP8266
  if (ag.s8.begin(&Serial) == false)
  {
#else
  if (ag.s8.begin(Serial1) == false)
  {
#endif
    failedHandler("SenseAir S8 init failed");
  }
}

void loop()
{
  int co2Ppm = ag.s8.getCo2();
  Serial.printf("CO2: %d\r\n", co2Ppm);
  delay(5000);
}

void failedHandler(String msg)
{
  while (true)
  {
    Serial.println(msg);
    delay(1000);
  }
}
