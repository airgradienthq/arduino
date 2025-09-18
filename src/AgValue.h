#ifndef _AG_VALUE_H_
#define _AG_VALUE_H_

#include "AgConfigure.h"
#include "AirGradient.h"
#include "App/AppDef.h"
#include "Libraries/Arduino_JSON/src/Arduino_JSON.h"
#include "Main/utils.h"
#include <Arduino.h>
#include <cstdint>
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
  Measurements(Configuration &config);
  ~Measurements() {}

  struct Measures {
    float temperature[2];
    float humidity[2];
    float co2;
    float tvoc; // Index value
    float tvoc_raw;
    float nox; // Index value
    float nox_raw;
    float pm_01[2];    // pm 1.0 atmospheric environment
    float pm_25[2];    // pm 2.5 atmospheric environment
    float pm_10[2];    // pm 10 atmospheric environment
    float pm_01_sp[2]; // pm 1.0 standard particle
    float pm_25_sp[2]; // pm 2.5 standard particle
    float pm_10_sp[2]; // pm 10 standard particle
    float pm_03_pc[2]; // particle count 0.3
    float pm_05_pc[2]; // particle count 0.5
    float pm_01_pc[2]; // particle count 1.0
    float pm_25_pc[2]; // particle count 2.5
    float pm_5_pc[2];  // particle count 5.0
    float pm_10_pc[2]; // particle count 10
    int bootCount;
    int signal;
    uint32_t freeHeap;
  };

  void setAirGradient(AirGradient *ag);

  // Enumeration for every AG measurements
  enum MeasurementType {
    Temperature,
    Humidity,
    CO2,
    TVOC, // index value
    TVOCRaw,
    NOx, // index value
    NOxRaw,
    PM01,    // PM1.0 under atmospheric environment
    PM25,    // PM2.5 under athompheric environment
    PM10,    // PM10 under atmospheric environment
    PM01_SP, // PM1.0 standard particle
    PM25_SP, // PM2.5 standard particle
    PM10_SP, // PM10 standard particle
    PM03_PC, // Particle 0.3 count
    PM05_PC, // Particle 0.5 count
    PM01_PC, // Particle 1.0 count
    PM25_PC, // Particle 2.5 count
    PM5_PC,  // Particle 5.0 count
    PM10_PC, // Particle 10 count
  };

  void printCurrentAverage();

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
   * @brief Get the target measurement average value
   *
   * @param type measurement type that will be retrieve
   * @param ch target type value channel
   * @return moving average value of target measurements type
   */
  float getAverage(MeasurementType type, int ch = 1);

  /**
   * @brief Get Temperature or Humidity correction value
   * Only if correction is applied from configuration or forceCorrection is True
   *
   * @param type measurement type either Temperature or Humidity
   * @param ch target type value channel
   * @param forceCorrection force using correction even though config correction is not applied, but
   * not for CUSTOM
   * @return correction value
   */
  float getCorrectedTempHum(MeasurementType type, int ch = 1, bool forceCorrection = false);

  /**
   * @brief Get the Corrected PM25 object based on the correction algorithm from configuration
   *
   * If correction is not enabled, then will return the raw value (either average or last value)
   *
   * @param useAvg Use moving average value if true, otherwise use latest value
   * @param ch MeasurementType channel
   * @param forceCorrection force using correction even though config correction is not applied, default to EPA
   * @return float Corrected PM2.5 value
   */
  float getCorrectedPM25(bool useAvg = false, int ch = 1, bool forceCorrection = false);

  /**
   * build json payload for every measurements
   */
  String toString(bool localServer, AgFirmwareMode fwMode, int rssi);

  Measures getMeasures();

  std::string buildMeasuresPayload(Measures &measures);

  /**
   * Set to true if want to debug every update value
   */
  void setDebug(bool debug);

  int bootCount();
  void setBootCount(int bootCount);

#ifndef ESP8266
  void setResetReason(esp_reset_reason_t reason);
#endif

private:
  Configuration &config;
  AirGradient *ag;

  // Some declared as an array (channel), because FW_MODE_O_1PPx has two PMS5003T
  FloatValue _temperature[2];
  FloatValue _humidity[2];
  IntegerValue _co2;
  IntegerValue _tvoc; // Index value
  IntegerValue _tvoc_raw;
  IntegerValue _nox; // Index value
  IntegerValue _nox_raw;
  IntegerValue _pm_01[2];    // pm 1.0 atmospheric environment
  IntegerValue _pm_25[2];    // pm 2.5 atmospheric environment
  IntegerValue _pm_10[2];    // pm 10 atmospheric environment
  IntegerValue _pm_01_sp[2]; // pm 1.0 standard particle
  IntegerValue _pm_25_sp[2]; // pm 2.5 standard particle
  IntegerValue _pm_10_sp[2]; // pm 10 standard particle
  IntegerValue _pm_03_pc[2]; // particle count 0.3
  IntegerValue _pm_05_pc[2]; // particle count 0.5
  IntegerValue _pm_01_pc[2]; // particle count 1.0
  IntegerValue _pm_25_pc[2]; // particle count 2.5
  IntegerValue _pm_5_pc[2];  // particle count 5.0
  IntegerValue _pm_10_pc[2]; // particle count 10
  int _bootCount;
  int _resetReason;
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

  void printCurrentPMAverage(int ch);

  JSONVar buildOutdoor(bool localServer, AgFirmwareMode fwMode);
  JSONVar buildIndoor(bool localServer);
  JSONVar buildPMS(int ch, bool allCh, bool withTempHum, bool compensate);
};

#endif /** _AG_VALUE_H_ */
