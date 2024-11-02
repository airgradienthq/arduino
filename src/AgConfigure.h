#ifndef _AG_CONFIG_H_
#define _AG_CONFIG_H_

#include "App/AppDef.h"
#include "Main/PrintLog.h"
#include "AirGradient.h"
#include <Arduino.h>
#include "Libraries/Arduino_JSON/src/Arduino_JSON.h"

class Configuration : public PrintLog {
public:
  struct PMCorrection {
    PMCorrectionAlgorithm algorithm;
    float intercept;
    float scalingFactor;
    bool useEPA; // EPA 2021
    bool changed;
  };

private:
  bool co2CalibrationRequested;
  bool ledBarTestRequested;
  bool updated;
  String failedMessage;
  bool _noxLearnOffsetChanged;
  bool _tvocLearningOffsetChanged;
  bool ledBarBrightnessChanged = false;
  bool displayBrightnessChanged = false;
  String otaNewFirmwareVersion;
  bool _offlineMode = false;
  bool _ledBarModeChanged = false;
  PMCorrection pmCorrection;

  AirGradient* ag;

  String getLedBarModeName(LedBarMode mode);
  PMCorrectionAlgorithm matchPmAlgorithm(String algorithm);
  bool updatePmCorrection(JSONVar &json);
  void saveConfig(void);
  void loadConfig(void);
  void defaultConfig(void);
  void printConfig(void);
  String jsonTypeInvalidMessage(String name, String type);
  String jsonValueInvalidMessage(String name, String value);
  void jsonInvalid(void);
  void configLogInfo(String name, String fromValue, String toValue);
  String getPMStandardString(bool usaqi);
  String getAbcDayString(int value);
  void toConfig(const char* buf);

public:
  Configuration(Stream &debugLog);
  ~Configuration();

  bool hasSensorS8 = true;
  bool hasSensorPMS1 = true;
  bool hasSensorPMS2 = true;
  bool hasSensorSGP = true;
  bool hasSensorSHT = true;

  bool begin(void);
  bool parse(String data, bool isLocal);
  String toString(void);
  String toString(AgFirmwareMode fwMode);
  bool isTemperatureUnitInF(void);
  String getCountry(void);
  bool isPmStandardInUSAQI(void);
  int getCO2CalibrationAbcDays(void);
  LedBarMode getLedBarMode(void);
  String getLedBarModeName(void);
  bool getDisplayMode(void);
  String getMqttBrokerUri(void);
  bool isPostDataToAirGradient(void);
  ConfigurationControl getConfigurationControl(void);
  bool isCo2CalibrationRequested(void);
  bool isLedBarTestRequested(void);
  void reset(void);
  String getModel(void);
  bool isUpdated(void);
  String getFailedMesage(void);
  void setPostToAirGradient(bool enable);
  bool noxLearnOffsetChanged(void);
  bool tvocLearnOffsetChanged(void);
  int getTvocLearningOffset(void);
  int getNoxLearningOffset(void);
  String wifiSSID(void);
  String wifiPass(void);
  void setAirGradient(AirGradient *ag);
  bool isLedBarBrightnessChanged(void);
  int getLedBarBrightness(void);
  bool isDisplayBrightnessChanged(void);
  int getDisplayBrightness(void);
  String newFirmwareVersion(void);
  bool isOfflineMode(void);
  void setOfflineMode(bool offline);
  void setOfflineModeWithoutSave(bool offline);
  bool isLedBarModeChanged(void);
  bool isMonitorDisplayCompensatedValues(void);
  bool isPMCorrectionChanged(void);
  bool isPMCorrectionEnabled(void);
  PMCorrection getPMCorrection(void);
};

#endif /** _AG_CONFIG_H_ */
