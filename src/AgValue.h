#ifndef _AG_VALUE_H_
#define _AG_VALUE_H_

#include "AgConfigure.h"
#include "AirGradient.h"
#include "App/AppDef.h"
#include "Libraries/Arduino_JSON/src/Arduino_JSON.h"
#include "Main/utils.h"
#include <Arduino.h>

class Measurements {
private:
  // Generic struct for update indication for respective value
  struct Update {
    int counter; // How many update attempts done
    int success; // How many update value that actually give valid value
    int max;     // Maximum update counter before calculating average
  };

  // Reading type for sensor value that outputs float
  struct FloatValue {
    float lastValue; // Last update value
    float sumValues; // Total value from each update
    float avg;       // The last average calculation after maximum update attempt reached
    Update update;
  };

  // Reading type for sensor value that outputs integer
  struct IntegerValue {
    int lastValue;           // Last update value
    unsigned long sumValues; // Total value from each update; unsigned long to accomodate TVOx and
                             // NOx raw data
    int avg;                 // The last average calculation after maximum update attempt reached
    Update update;
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
    PM03_PC,
  };

  /**
   * @brief Set each AgValueType maximum update for a value type before calculate the average
   *
   * @param type the target value type to set
   * @param max the maximum counter
   */
  void maxUpdate(AgValueType type, int max);

  /**
   * @brief update target type value with new value.
   * Each AgValueType has last raw value and last average that are calculated based on max number of
   * update. This function is for AgValueType that use INT as the data type
   *
   * @param type (AgValueType) value type that will be updated
   * @param val (int) the new value
   * @param ch (int) the AgValueType channel, not every AgValueType has more than 1 channel.
   * Currently maximum channel is 2. Default: 1 (channel 1)
   * @return true if update counter reached and new average value is calculated
   * @return false otherwise
   */
  bool updateValue(AgValueType type, int val, int ch = 1);

  /**
   * @brief update target type value with new value.
   * Each AgValueType has last raw value and last average that are calculated based on max number of
   * update. This function is for AgValueType that use FLOAT as the data type
   *
   * @param type (AgValueType) value type that will be updated
   * @param val (float) the new value
   * @param ch (int) the AgValueType channel, not every AgValueType has more than 1 channel.
   * Currently maximum channel is 2. Default: 1 (channel 1)
   * @return true if update counter reached and new average value is calculated
   * @return false otherwise
   */
  bool updateValue(AgValueType type, float val, int ch = 1);

  /**
   * @brief Get the target measurement type value
   *
   * @param type measurement type that will be retrieve
   * @param average true if expect last average value, false if expect last update value
   * @param ch target type value channel
   * @return int measurement type value
   */
  int getValue(AgValueType type, bool average = true, int ch = 1);

  /**
   * @brief Get the target measurement type value
   *
   * @param type measurement type that will be retrieve
   * @param average true if expect last average value, false if expect last update value
   * @param ch target type value channel
   * @return float measurement type value
   */
  float getValueFloat(AgValueType type, bool average = true, int ch = 1);

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
  String toStringX(bool localServer, AgFirmwareMode fwMode, int rssi, AirGradient &ag,
                   Configuration &config);

private:
  // Some declared as an array (channel), because FW_MODE_O_1PPx has two PMS5003T
  FloatValue _temperature[2];
  FloatValue _humidity[2];
  IntegerValue _co2;
  IntegerValue _tvoc;
  IntegerValue _tvoc_raw;
  IntegerValue _nox;
  IntegerValue _nox_raw;
  IntegerValue _pm_25[2];
  IntegerValue _pm_01[2];
  IntegerValue _pm_10[2];
  IntegerValue _pm_03_pc[2]; // particle count 0.3

  /**
   * @brief Get PMS5003 firmware version string
   *
   * @param fwCode
   * @return String
   */
  String pms5003FirmwareVersion(int fwCode);
  /**
   * @brief Get PMS5003T firmware version string
   *
   * @param fwCode
   * @return String
   */
  String pms5003TFirmwareVersion(int fwCode);
  /**
   * @brief Get firmware version string
   *
   * @param prefix Prefix firmware string
   * @param fwCode Version code
   * @return string
   */
  String pms5003FirmwareVersionBase(String prefix, int fwCode);

  /**
   * Convert AgValue Type to string representation of the value
   */
  String agValueTypeStr(AgValueType type);

  /**
   * @brief check if provided channel is a valid channel or not
   * abort program if invalid
   */
  void validateChannel(int ch);

  JSONVar buildOutdoor(bool localServer, AgFirmwareMode fwMode, AirGradient &ag,
                       Configuration &config);
  JSONVar buildIndoor(bool localServer, AirGradient &ag, Configuration &config);
  JSONVar buildPMS(AirGradient &ag, int ch, bool allCh, bool withTempHum, bool compensate);
};

#endif /** _AG_VALUE_H_ */
