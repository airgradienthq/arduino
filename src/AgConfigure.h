#ifndef _AG_CONFIG_H_
#define _AG_CONFIG_H_

#include "App/AppDef.h"
#include "Main/PrintLog.h"
#include <Arduino.h>
#include <Arduino_JSON.h>

class AgConfigure : public PrintLog {
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

  String getLedBarModeName(LedBarMode mode);
  void saveConfig(void);
  void loadConfig(void);
  void defaultConfig(void);
  void printConfig(void);

public:
  AgConfigure(Stream &debugLog);
  ~AgConfigure();

  bool begin(void);
  bool parse(String data, bool isLocal);
  String toString(void);
  bool isTemperatureUnitInF(void);
  String getCountry(void);
  bool isPmStandardInUSAQI(void);
  int getCO2CalirationAbcDays(void);
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
};

#endif /** _AG_CONFIG_H_ */
