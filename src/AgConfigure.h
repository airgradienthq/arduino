#ifndef _AG_CONFIG_H_
#define _AG_CONFIG_H_

#include "App/AppDef.h"
#include "Main/PrintLog.h"
#include <Arduino.h>
#include <Arduino_JSON.h>

class Configuration : public PrintLog {
private:
  struct Config {
    char model[20];
    char country[3]; /** Country name has only 2 character, ex: TH = Thailand */
    char mqttBroker[256]; /** MQTT broker URI */
    bool inUSAQI; /** If PM standard "ugm3"  inUSAQI = false, otherwise is true
                   */
    bool inF;     /** Temperature unit F */
    bool postDataToAirGradient;   /** If true, monitor will not POST data to
                                    airgradient server. Make sure no error
                                    message shown on monitor */
    uint8_t configurationControl; /** If true, configuration from airgradient
                               server will be ignored */
    bool displayMode;             /** true if enable display */
    uint8_t useRGBLedBar;
    uint8_t abcDays;
    int tvocLearningOffset;
    int noxLearningOffset;
    char temperatureUnit; // 'f' or 'c'

    uint32_t _check;
  };
  struct Config config;
  bool co2CalibrationRequested;
  bool ledBarTestRequested;
  bool udpated;
  String failedMessage;

  String getLedBarModeName(LedBarMode mode);
  void saveConfig(void);
  void loadConfig(void);
  void defaultConfig(void);
  void printConfig(void);
  bool jsonTypeInvalid(JSONVar root, String validType);
  String jsonTypeInvalidMessage(String name, String type);
  String jsonValueInvalidMessage(String name, String value);
  void jsonInvalid(void);

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
};

#endif /** _AG_CONFIG_H_ */
