/*
This is sample code for the AirGradient library with a minimal implementation to read PM values from the Plantower sensor.

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
#ifdef ESP8266
  if(ag.pms5003.begin(&Serial) == false) {
    failedHandler("Init PMS5003 failed");
  }
#else
  if (ag.getBoardType() == BOARD_OUTDOOR_MONITOR_V1_3) {
    if(ag.pms5003t_1.begin(Serial0) == false) {
      failedHandler("Init PMS5003T failed");
    }
  } else {
    if(ag.pms5003.begin(Serial0) == false) {
      failedHandler("Init PMS5003T failed");
    }
  }
#endif
}

void loop() {
  int PM2;
#ifdef ESP8266
  PM2 = ag.pms5003.getPm25Ae();
  Serial.printf("PM2.5 in ug/m3: %d\r\n", PM2);
  Serial.printf("PM2.5 in US AQI: %d\r\n", ag.pms5003.convertPm25ToUsAqi(PM2));
#else
  if (ag.getBoardType() == BOARD_OUTDOOR_MONITOR_V1_3) {
    PM2 = ag.pms5003t_1.getPm25Ae();
  } else {
    PM2 = ag.pms5003.getPm25Ae();
  }
  
  Serial.printf("PM2.5 in ug/m3: %d\r\n", PM2);
  if (ag.getBoardType() == BOARD_OUTDOOR_MONITOR_V1_3) {
    Serial.printf("PM2.5 in US AQI: %d\r\n",
                  ag.pms5003t_1.convertPm25ToUsAqi(PM2));
  } else {
    Serial.printf("PM2.5 in US AQI: %d\r\n",
                  ag.pms5003.convertPm25ToUsAqi(PM2));
  }
#endif

  delay(5000);
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}
