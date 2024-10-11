#ifndef _AG_VALUE_H_
#define _AG_VALUE_H_

#include "App/AppDef.h"
#include "Main/utils.h"
#include <Arduino.h>

class Measurements {
private:
  // Generic struct for reading indication for respective value
  // TODO: Reading naming is confusing, because its not actually reading but updating with new value
  struct Reading {
    int counter; // How many reading attempts done
    int success; // How many reading that success from each attempts
    int max;     // Maximum reading attempt
  };

  // Reading type for sensor value that outputs float
  struct FloatValue {
    float lastValue; // Last reading value
    float sumValues; // Total value of each reading
    float avg;       // The last average calculation after maximum reading attempt reached
    Reading read;
  };

  // Reading type for sensor value that outputs integer
  struct IntegerValue {
    int lastValue;           // Last reading value
    unsigned long sumValues; // Total value of each reading // TODO: explain why unsigned long
    int avg;                 // The last average calculation after maximum reading attempt reached
    Reading read;
  };

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

  enum class AgValueType {
    Temperature,
    Humidity,
    CO2,
    TVOC,
    TVOCRaw,
    NOx,
    NOxRaw,
    PM25,
    PM01,
    PM10,
    PM03,
  };

  void updateValue(AgValueType type, int val);
  void updateValue(AgValueType type, float val);

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

  String toString(bool isLocal, AgFirmwareMode fwMode, int rssi, void *_ag, void *_config);

private:
  FloatValue _temperature;
  FloatValue _humidity;
  IntegerValue _co2;
  IntegerValue _tvoc;
  IntegerValue _tvoc_raw;
  IntegerValue _nox;
  IntegerValue _nox_raw;
  IntegerValue _pm_25;
  IntegerValue _pm_01;
  IntegerValue _pm_10;
  IntegerValue _pm_03_pc; // particle count 0.3

  String pms5003FirmwareVersion(int fwCode);
  String pms5003TFirmwareVersion(int fwCode);
  String pms5003FirmwareVersionBase(String prefix, int fwCode);
  String agValueTypeStr(AgValueType type);
};

#endif /** _AG_VALUE_H_ */
