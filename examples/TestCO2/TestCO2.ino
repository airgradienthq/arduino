/*
This is sample code for the AirGradient library with a minimal implementation to read CO2 values from the SenseAir S8 sensor.

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
*/

#include <AirGradient.h>

#ifdef ESP8266
AirGradient ag = AirGradient(BOARD_DIY_PRO_INDOOR_V4_2);
// AirGradient ag = AirGradient(BOARD_DIY_BASIC_KIT);
#else
// AirGradient ag = AirGradient(BOARD_ONE_INDOOR_MONITOR_V9_0);
AirGradient ag = AirGradient(BOARD_OUTDOOR_MONITOR_V1_3);
#endif

void failedHandler(String msg);

void setup() {
  Serial.begin(115200);

  /** Init CO2 sensor */
#ifdef ESP8266
  if (ag.s8.begin(&Serial) == false) {
#else
  if (ag.s8.begin(Serial1) == false) {
#endif
    failedHandler("SenseAir S8 init failed");
  }
}

void loop() {
  int CO2 = ag.s8.getCo2();
  Serial.printf("CO2: %d\r\n", CO2);
  delay(5000);
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}
