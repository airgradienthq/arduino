#ifndef _AG_VALUE_H_
#define _AG_VALUE_H_

#include <Arduino.h>
#include "App/AppDef.h"

class Measurements {
private:
public:
  Measurements() {
    pm25_1 = -1;
    pm01_1 = -1;
    pm10_1 = -1;
    pm03PCount_1 = -1;
    temp_1 = -1001;
    hum_1 = -1;

    pm25_2 = -1;
    pm01_2 = -1;
    pm10_2 = -1;
    pm03PCount_2 = -1;
    temp_2 = -1001;
    hum_2 = -1;

    Temperature = -1001;
    Humidity = -1;
    CO2 = -1;
    TVOC = -1;
    TVOCRaw = -1;
    NOx = -1;
    NOxRaw = -1;
  }
  ~Measurements() {}

  float Temperature;
  int Humidity;
  int CO2;
  int TVOC;
  int TVOCRaw;
  int NOx;
  int NOxRaw;

  int pm25_1;
  int pm01_1;
  int pm10_1;
  int pm03PCount_1;
  float temp_1;
  int hum_1;

  int pm25_2;
  int pm01_2;
  int pm10_2;
  int pm03PCount_2;
  float temp_2;
  int hum_2;

  int pm1Value01;
  int pm1Value25;
  int pm1Value10;
  int pm1PCount;
  int pm1temp;
  int pm1hum;
  int pm2Value01;
  int pm2Value25;
  int pm2Value10;
  int pm2PCount;
  int pm2temp;
  int pm2hum;
  int countPosition;
  const int targetCount = 20;
  int bootCount;
  int otaBootCount = -1;

  String toString(bool isLocal, AgFirmwareMode fwMode, int rssi, void* _ag, void* _config);
};

#endif /** _AG_VALUE_H_ */
