/*
This is sample code for the AirGradient library with a minimal implementation
to read PM values from the Plantower sensor.

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
*/

#include <AirGradient.h>

#ifdef ESP8266
AirGradient ag = AirGradient(DIY_BASIC);
#else
AirGradient ag = AirGradient(ONE_INDOOR);
// AirGradient ag = AirGradient(OPEN_AIR_OUTDOOR);
#endif

void failedHandler(String msg);

void setup() {
  Serial.begin(115200);
#ifdef ESP8266
  if (ag.pms5003.begin(&Serial) == false) {
    failedHandler("Init PMS5003 failed");
  }
#else
  if (ag.getBoardType() == OPEN_AIR_OUTDOOR) {
    if (ag.pms5003t_1.begin(Serial0) == false) {
      failedHandler("Init PMS5003T failed");
    }
  } else {
    if (ag.pms5003.begin(Serial0) == false) {
      failedHandler("Init PMS5003T failed");
    }
  }
#endif
}

uint32_t lastRead = 0;
void loop() {
  int PM2;
  bool readResul = false;

  uint32_t ms = (uint32_t)(millis() - lastRead);
  if (ms >= 5000) {
    lastRead = millis();
#ifdef ESP8266
    if (ag.pms5003.connected()) {
      PM2 = ag.pms5003.getPm25Ae();
      Serial.printf("PM2.5 in ug/m3: %d\r\n", PM2);
      Serial.printf("PM2.5 in US AQI: %d\r\n",
                    ag.pms5003.convertPm25ToUsAqi(PM2));
    } else {
      Serial.println("PMS sensor failed");
    }
#else
    if (ag.getBoardType() == OPEN_AIR_OUTDOOR) {
      if (ag.pms5003t_1.connected()) {
        PM2 = ag.pms5003t_1.getPm25Ae();
        readResul = true;
      }
    } else {
      if (ag.pms5003.connected()) {
        PM2 = ag.pms5003.getPm25Ae();
        readResul = true;
      }
    }

    if (readResul) {
      Serial.printf("PM2.5 in ug/m3: %d\r\n", PM2);
      if (ag.getBoardType() == OPEN_AIR_OUTDOOR) {
        Serial.printf("PM2.5 in US AQI: %d\r\n",
                      ag.pms5003t_1.convertPm25ToUsAqi(PM2));
      } else {
        Serial.printf("PM2.5 in US AQI: %d\r\n",
                      ag.pms5003.convertPm25ToUsAqi(PM2));
      }
    } else {
      Serial.println("PMS sensor failed");
    }
#endif
  }

  if (ag.getBoardType() == OPEN_AIR_OUTDOOR) {
    ag.pms5003t_1.handle();
  } else {
    ag.pms5003.handle();
  }
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}
