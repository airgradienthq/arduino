#ifndef _AG_VALUE_H_
#define _AG_VALUE_H_

#include "AgConfigure.h"
#include "AirGradient.h"
#include "App/AppDef.h"
#include "Libraries/Arduino_JSON/src/Arduino_JSON.h"
#include "Main/utils.h"
#include <Arduino.h>
#include <vector>

class Measurements {
private:
  // Generic struct for update indication for respective value
  struct Update {
    int invalidCounter; // Counting on how many invalid value that are passed to update function
    int max;            // Maximum length of the period of the moving average
    float avg;          // Moving average value, updated every update function called
  };

  // Reading type for sensor value that outputs float
  struct FloatValue {
    float sumValues; // Total value from each update
    std::vector<float> listValues; // List of update value that are kept
    Update update;
  };

  // Reading type for sensor value that outputs integer
  struct IntegerValue {
    unsigned long sumValues; // Total value from each update; unsigned long to accomodate TVOx and
                             // NOx raw data
    std::vector<int> listValues; // List of update value that are kept
    Update update;
  };

public:
  Measurements() {}
  ~Measurements() {}

  // Enumeration for every AG measurements
  enum MeasurementType {
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
   * @brief Set each MeasurementType maximum period length for moving average
   *
   * @param type the target measurement type to set
   * @param max the maximum period length
   */
  void maxPeriod(MeasurementType, int max);

  /**
   * @brief update target measurement type with new value.
   * Each MeasurementType has last raw value and moving average value based on max period
   * This function is for MeasurementType that use INT as the data type
   *
   * @param type measurement type that will be updated
   * @param val (int) the new value
   * @param ch (int) the MeasurementType channel, not every MeasurementType has more than 1 channel.
   * Currently maximum channel is 2. Default: 1 (channel 1)
   * @return false if new value invalid consecutively reach threshold (max period)
   * @return true otherwise
   */
  bool update(MeasurementType type, int val, int ch = 1);

  /**
   * @brief update target measurement type with new value.
   * Each MeasurementType has last raw value and moving average value based on max period
   * This function is for MeasurementType that use FLOAT as the data type
   *
   * @param type measurement type that will be updated
   * @param val (float) the new value
   * @param ch (int) the MeasurementType channel, not every MeasurementType has more than 1 channel.
   * Currently maximum channel is 2. Default: 1 (channel 1)
   * @return false if new value invalid consecutively reach threshold (max period)
   * @return true otherwise
   */
  bool update(MeasurementType type, float val, int ch = 1);

  /**
   * @brief Get the target measurement latest value
   *
   * @param type measurement type that will be retrieve
   * @param ch target type value channel
   * @return int measurement type value
   */
  int get(MeasurementType type, int ch = 1);

  /**
   * @brief Get the target measurement latest value
   *
   * @param type measurement type that will be retrieve
   * @param ch target type value channel
   * @return float measurement type value
   */
  float getFloat(MeasurementType type, int ch = 1);

  /**
   * build json payload for every measurements
   */
  String toString(bool localServer, AgFirmwareMode fwMode, int rssi, AirGradient &ag,
                  Configuration &config);

  /**
   * Set to true if want to debug every update value
   */
  void setDebug(bool debug);

  // TODO: update this to use setter
  int bootCount;

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

  bool _debug = false;

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
  String measurementTypeStr(MeasurementType type);

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
