#ifndef _AG_CONFIG_H_
#define _AG_CONFIG_H_

#include "App/AppDef.h"
#include "Main/PrintLog.h"
#include "AirGradient.h"
#include <Arduino.h>

class Configuration : public PrintLog {
private:
  bool co2CalibrationRequested;
  bool ledBarTestRequested;
  bool udpated;
  String failedMessage;
  bool _noxLearnOffsetChanged;
  bool _tvocLearningOffsetChanged;
  bool ledBarBrightnessChanged = false;
  bool displayBrightnessChanged = false;

  AirGradient* ag;

  String getLedBarModeName(LedBarMode mode);
  void saveConfig(void);
  void loadConfig(void);
  void defaultConfig(void);
  void printConfig(void);
  String jsonTypeInvalidMessage(String name, String type);
  String jsonValueInvalidMessage(String name, String value);
  void jsonInvalid(void);
  void configLogInfo(String name, String fromValue, String toValue);
  String getPMStandardString(bool usaqi);
  String getDisplayModeString(bool dispMode);
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
  bool isOfflineMode(void);
  void setOfflineMode(bool offline);
};

#endif /** _AG_CONFIG_H_ */
