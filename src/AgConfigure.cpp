#include "AgConfigure.h"
#include "Libraries/Arduino_JSON/src/Arduino_JSON.h"
#if ESP32
#include "FS.h"
#include "SPIFFS.h"
#else
#include "EEPROM.h"
#endif
#include <time.h>

#define EEPROM_CONFIG_SIZE 1024
#define CONFIG_FILE_NAME "/cfg.json"

const char *CONFIGURATION_CONTROL_NAME[] = {
    [ConfigurationControlLocal] = "local",
    [ConfigurationControlCloud] = "cloud",
    [ConfigurationControlBoth] = "both"};

const char *LED_BAR_MODE_NAMES[] = {
    [LedBarModeOff] = "off",
    [LedBarModePm] = "pm",
    [LedBarModeCO2] = "co2",
};

#define JSON_PROP_NAME(name) jprop_##name
#define JSON_PROP_DEF(name) const char *JSON_PROP_NAME(name) = #name

JSON_PROP_DEF(model);
JSON_PROP_DEF(country);
JSON_PROP_DEF(pmStandard);
JSON_PROP_DEF(ledBarMode);
JSON_PROP_DEF(displayMode);
JSON_PROP_DEF(abcDays);
JSON_PROP_DEF(tvocLearningOffset);
JSON_PROP_DEF(noxLearningOffset);
JSON_PROP_DEF(mqttBrokerUrl);
JSON_PROP_DEF(temperatureUnit);
JSON_PROP_DEF(configurationControl);
JSON_PROP_DEF(postDataToAirGradient);
JSON_PROP_DEF(ledbarBrightness);
JSON_PROP_DEF(displayBrightness);
JSON_PROP_DEF(co2CalibrationRequested);
JSON_PROP_DEF(ledBarTestRequested);
JSON_PROP_DEF(lastOta);
JSON_PROP_DEF(offlineMode);
JSONVar jconfig;

static bool jsonTypeInvalid(JSONVar root, String validType) {
  String type = JSON.typeof_(root);
  if (type == validType || type == "undefined" || type == "unknown" ||
      type == "null") {
    return false;
  }
  return true;
}

/**
 * @brief Get LedBarMode Name
 *
 * @param mode LedBarMode value
 * @return String
 */
String Configuration::getLedBarModeName(LedBarMode mode) {
  if (mode == LedBarModeOff) {
    return String(LED_BAR_MODE_NAMES[LedBarModeOff]);
  } else if (mode == LedBarModePm) {
    return String(LED_BAR_MODE_NAMES[LedBarModePm]);
  } else if (mode == LedBarModeCO2) {
    return String(LED_BAR_MODE_NAMES[LedBarModeCO2]);
  }
  return String("unknown");
}

/**
 * @brief Save configure to device storage (EEPROM)
 *
 */
void Configuration::saveConfig(void) {
  String data = toString();
  int len = data.length();
#ifdef ESP8266
  for (int i = 0; i < len; i++) {
    EEPROM.write(i, data[i]);
  }
  EEPROM.commit();
#else
  File file = SPIFFS.open(CONFIG_FILE_NAME, "w", true);
  if (file && !file.isDirectory()) {
    if (file.write((const uint8_t *)data.c_str(), len) != len) {
      logError("Write SPIFFS file failed");
    }
    file.close();
  } else {
    logError("Open SPIFFS file to write failed");
  }
#endif
  logInfo("Save Config");
}

void Configuration::loadConfig(void) {
  bool readSuccess = false;
  char *buf = (char *)malloc(EEPROM_CONFIG_SIZE);
  if (buf == NULL) {
    logError("Malloc read file buffer failed");
    return;
  }
  memset(buf, 0, EEPROM_CONFIG_SIZE);
#ifdef ESP8266
  for (int i = 0; i < EEPROM_CONFIG_SIZE; i++) {
    buf[i] = EEPROM.read(i);
  }
  readSuccess = true;
#else
  File file = SPIFFS.open(CONFIG_FILE_NAME);
  if (file && !file.isDirectory()) {
    logInfo("Read file");
    file.readBytes(buf, file.size());
    readSuccess = true;
    file.close();
  } else {
    SPIFFS.format();
  }
#endif
  toConfig(buf);
  free(buf);
}

/**
 * @brief Set configuration default
 *
 */
void Configuration::defaultConfig(void) {
  jconfig = JSON.parse("{}");
  jconfig[jprop_country] = "";
  jconfig[jprop_mqttBrokerUrl] = "";
  jconfig[jprop_configurationControl] =
      String(CONFIGURATION_CONTROL_NAME
                 [ConfigurationControl::ConfigurationControlBoth]);
  jconfig[jprop_pmStandard] = getPMStandardString(false);
  jconfig[jprop_temperatureUnit] = "c";
  jconfig[jprop_postDataToAirGradient] = true;
  jconfig[jprop_displayMode] = getDisplayModeString(true);
  jconfig[jprop_ledbarBrightness] = 100;
  jconfig[jprop_displayBrightness] = 100;
  jconfig[jprop_ledBarMode] = getLedBarModeName(LedBarMode::LedBarModeCO2);

  jconfig[jprop_tvocLearningOffset] = 12;
  jconfig[jprop_noxLearningOffset] = 12;
  jconfig[jprop_abcDays] = 8;
  jconfig[jprop_model] = "";
  jconfig[jprop_offlineMode] = false;

  saveConfig();
}

/**
 * @brief Show configuration as JSON string message over log
 *
 */
void Configuration::printConfig(void) {
  String cfg = toString();
  logInfo(cfg);
}

/**
 * @brief Construct a new Ag Configure:: Ag Configure object
 *
 * @param debugLog Serial Stream
 */
Configuration::Configuration(Stream &debugLog)
    : PrintLog(debugLog, "Configure") {}

/**
 * @brief Destroy the Ag Configure:: Ag Configure object
 *
 */
Configuration::~Configuration() {}

/**
 * @brief Initialize configuration
 *
 * @return true Success
 * @return false Failure
 */
bool Configuration::begin(void) {
#ifdef ESP32
  if (!SPIFFS.begin(true)) {
    logError("Init SPIFFS failed");
    return false;
  }
#else
  EEPROM.begin(EEPROM_CONFIG_SIZE);
#endif

  loadConfig();
  printConfig();

  return true;
}

/**
 * @brief Parse JSON configura string to local configure
 *
 * @param data JSON string data
 * @param isLocal true of data got from local, otherwise get from Aigradient
 * server
 * @return true Success
 * @return false Failure
 */
bool Configuration::parse(String data, bool isLocal) {
  logInfo("Parse configure: " + data);

  JSONVar root = JSON.parse(data);
  failedMessage = "";
  if (root == undefined) {
    failedMessage = "JSON invalid";
    logError(failedMessage);
    return false;
  }
  logInfo("Parse configure success");

  /** Is configuration changed */
  bool changed = false;

  /** Get ConfigurationControl */
  String lasCtrl = jconfig[jprop_configurationControl];
  if (isLocal) {
    if (JSON.typeof_(root[jprop_configurationControl]) == "string") {
      String ctrl = root[jprop_configurationControl];
      if (ctrl ==
              String(CONFIGURATION_CONTROL_NAME
                         [ConfigurationControl::ConfigurationControlBoth]) ||
          ctrl ==
              String(CONFIGURATION_CONTROL_NAME
                         [ConfigurationControl::ConfigurationControlLocal]) ||
          ctrl ==
              String(CONFIGURATION_CONTROL_NAME
                         [ConfigurationControl::ConfigurationControlCloud])) {
        if (ctrl != lasCtrl) {
          jconfig[jprop_configurationControl] = ctrl;
          changed = true;
        }
      } else {
        failedMessage =
            jsonValueInvalidMessage(String(jprop_configurationControl), ctrl);
        jsonInvalid();
        return false;
      }
    } else {
      if (jsonTypeInvalid(root[jprop_configurationControl], "string")) {
        failedMessage = jsonTypeInvalidMessage(
            String(jprop_configurationControl), "string");
        jsonInvalid();
        return false;
      }
    }

    if (changed) {
      changed = false;
      saveConfig();
      configLogInfo(String(jprop_configurationControl), lasCtrl,
                    jconfig[jprop_configurationControl]);
    }

    if (jconfig[jprop_configurationControl] ==
        String(CONFIGURATION_CONTROL_NAME
                   [ConfigurationControl::ConfigurationControlCloud])) {
      failedMessage = "Local configure ignored";
      jsonInvalid();
      return false;
    }
  } else {
    if (jconfig[jprop_configurationControl] ==
        String(CONFIGURATION_CONTROL_NAME
                   [ConfigurationControl::ConfigurationControlLocal])) {
      failedMessage = "Cloud configure ignored";
      jsonInvalid();
      return false;
    }
  }

  char temperatureUnit = 0;
  if (JSON.typeof_(root[jprop_country]) == "string") {
    String country = root[jprop_country];
    if (country.length() == 2) {
      String oldCountry = jconfig[jprop_country];
      if (country != oldCountry) {
        changed = true;
        configLogInfo(String(jprop_country), oldCountry, country);
        jconfig[jprop_country] = country;
      }
    } else {
      failedMessage = "Country name " + country +
                      " invalid. Find details here (ALPHA-2): "
                      "https://www.iban.com/country-codes";
      jsonInvalid();
      return false;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_country], "string")) {
      failedMessage = jsonTypeInvalidMessage(String(jprop_country), "string");
      jsonInvalid();
      return false;
    }
  }

  if (JSON.typeof_(root[jprop_pmStandard]) == "string") {
    String standard = root[jprop_pmStandard];
    if (standard == getPMStandardString(true) ||
        standard == getPMStandardString(false)) {
      String oldStandard = jconfig[jprop_pmStandard];
      if (standard != oldStandard) {
        configLogInfo(String(jprop_pmStandard), oldStandard, standard);
        jconfig[jprop_pmStandard] = standard;
        changed = true;
      }
    } else {
      failedMessage =
          jsonValueInvalidMessage(String(jprop_pmStandard), standard);
      jsonInvalid();
      return false;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_pmStandard], "string")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_pmStandard), "string");
      jsonInvalid();
      return false;
    }
  }

  if (JSON.typeof_(root[jprop_co2CalibrationRequested]) == "boolean") {
    co2CalibrationRequested = root[jprop_co2CalibrationRequested];
    if (co2CalibrationRequested) {
      logInfo(String(jprop_co2CalibrationRequested) + String(": ") +
              String(co2CalibrationRequested ? "True" : "False"));
    }
  } else {
    if (jsonTypeInvalid(root[jprop_co2CalibrationRequested], "boolean")) {
      failedMessage = jsonTypeInvalidMessage(
          String(jprop_co2CalibrationRequested), "boolean");
      jsonInvalid();
      return false;
    }
  }

  if (JSON.typeof_(root[jprop_ledBarTestRequested]) == "boolean") {
    ledBarTestRequested = root[jprop_ledBarTestRequested];
    if (ledBarTestRequested) {
      logInfo(String(jprop_ledBarTestRequested) + String(": ") +
              String(ledBarTestRequested ? "True" : "False"));
    }
  } else {
    if (jsonTypeInvalid(root[jprop_ledBarTestRequested], "boolean")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_ledBarTestRequested), "boolean");
      jsonInvalid();
      return false;
    }
  }

  if (JSON.typeof_(root[jprop_ledBarMode]) == "string") {
    String mode = root[jprop_ledBarMode];
    if (mode == getLedBarModeName(LedBarMode::LedBarModeCO2) ||
        mode == getLedBarModeName(LedBarMode::LedBarModeOff) ||
        mode == getLedBarModeName(LedBarMode::LedBarModePm)) {
      String oldMode = jconfig[jprop_ledBarMode];
      if (mode != oldMode) {
        jconfig[jprop_ledBarMode] = mode;
        changed = true;
      }
    } else {
      failedMessage = jsonValueInvalidMessage(String(jprop_ledBarMode), mode);
      jsonInvalid();
      return false;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_ledBarMode], "string")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_ledBarMode), "string");
      jsonInvalid();
      return false;
    }
  }

  if (JSON.typeof_(root[jprop_displayMode]) == "string") {
    String mode = root[jprop_displayMode];
    if (mode == getDisplayModeString(true) ||
        mode == getDisplayModeString(false)) {
      String oldMode = jconfig[jprop_displayMode];
      if (mode != oldMode) {
        jconfig[jprop_displayMode] = mode;
        changed = true;
      }
    } else {
      jsonValueInvalidMessage(String(jprop_displayMode), mode);
      jsonInvalid();
      return false;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_displayMode], "string")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_displayMode), "string");
      jsonInvalid();
      return false;
    }
  }

  if (JSON.typeof_(root[jprop_abcDays]) == "number") {
    int value = root[jprop_abcDays];
    if (value <= 0) {
      value = 0;
    }
    int oldValue = jconfig[jprop_abcDays];
    if (value != oldValue) {
      logInfo(String("Set ") + String(jprop_abcDays) + String(": ") +
              String(value));
      configLogInfo(String(jprop_abcDays), getAbcDayString(oldValue),
                    String(getAbcDayString(value)));

      jconfig[jprop_abcDays] = value;
      changed = true;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_abcDays], "number")) {
      failedMessage = jsonTypeInvalidMessage(String(jprop_abcDays), "number");
      jsonInvalid();
      return false;
    }
  }

  _tvocLearningOffsetChanged = false;
  if (JSON.typeof_(root[jprop_tvocLearningOffset]) == "number") {
    int value = root[jprop_tvocLearningOffset];
    int oldValue = jconfig[jprop_tvocLearningOffset];
    if (value < 0) {
      jsonValueInvalidMessage(String(jprop_tvocLearningOffset), String(value));
      jsonInvalid();
      return false;
    } else {
      if (value != oldValue) {
        changed = true;
        _tvocLearningOffsetChanged = true;
        configLogInfo(String(jprop_tvocLearningOffset), String(oldValue),
                      String(value));
        jconfig[jprop_tvocLearningOffset] = value;
      }
    }
  } else {
    if (jsonTypeInvalid(root[jprop_tvocLearningOffset], "number")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_tvocLearningOffset), "number");
      jsonInvalid();
      return false;
    }
  }

  _noxLearnOffsetChanged = false;
  if (JSON.typeof_(root[jprop_noxLearningOffset]) == "number") {
    int value = root[jprop_noxLearningOffset];
    int oldValue = jconfig[jprop_noxLearningOffset];
    if (value > 0) {
      if (value != oldValue) {
        changed = true;
        _noxLearnOffsetChanged = true;
        configLogInfo(String(jprop_noxLearningOffset), String(oldValue),
                      String(value));
        jconfig[jprop_noxLearningOffset] = value;
      }
    } else {
      failedMessage = jsonValueInvalidMessage(String(jprop_noxLearningOffset),
                                              String(value));
      jsonInvalid();
      return false;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_noxLearningOffset], "number")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_noxLearningOffset), "number");
      jsonInvalid();
      return false;
    }
  }

  if (JSON.typeof_(root[jprop_mqttBrokerUrl]) == "string") {
    String broker = root[jprop_mqttBrokerUrl];
    String oldBroker = jconfig[jprop_mqttBrokerUrl];
    if (broker != oldBroker) {
      changed = true;
      configLogInfo(String(jprop_mqttBrokerUrl), oldBroker, broker);
      jconfig[jprop_mqttBrokerUrl] = broker;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_mqttBrokerUrl], "string")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_mqttBrokerUrl), "string");
      jsonInvalid();
      return false;
    }
  }

  if (JSON.typeof_(root[jprop_temperatureUnit]) == "string") {
    String unit = root[jprop_temperatureUnit];
    String oldUnit = jconfig[jprop_temperatureUnit];
    unit.toLowerCase();
    if (unit == "c" || unit == "f") {
      if (unit != oldUnit) {
        changed = true;
        jconfig[jprop_temperatureUnit] = unit;
        configLogInfo(String(jprop_temperatureUnit), oldUnit, unit);
      }
    } else {
      jsonValueInvalidMessage(String(jprop_temperatureUnit), unit);
      jsonInvalid();
      return false;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_temperatureUnit], "string")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_temperatureUnit), "string");
      jsonInvalid();
      return false;
    }
  }

  if (isLocal) {
    if (JSON.typeof_(root[jprop_postDataToAirGradient]) == "boolean") {
      bool value = root[jprop_postDataToAirGradient];
      bool oldValue = jconfig[jprop_postDataToAirGradient];
      if (value != oldValue) {
        changed = true;
        configLogInfo(String(jprop_postDataToAirGradient),
                      String(oldValue ? "true" : "false"),
                      String(value ? "true" : "false"));
        jconfig[jprop_postDataToAirGradient] = value;
      }
    } else {
      if (jsonTypeInvalid(root[jprop_postDataToAirGradient], "boolean")) {
        failedMessage = jsonTypeInvalidMessage(
            String(jprop_postDataToAirGradient), "boolean");
        jsonInvalid();
        return false;
      }
    }
  }

  /** Parse data only got from AirGradient server */
  if (isLocal == false) {
    if (JSON.typeof_(root[jprop_model]) == "string") {
      String model = root[jprop_model];
      String oldModel = jconfig[jprop_model];
      if (model != oldModel) {
        changed = true;
        configLogInfo(String(jprop_model), oldModel, model);
        jconfig[jprop_model] = model;
      }
    } else {
      if (jsonTypeInvalid(root[jprop_model], "string")) {
        failedMessage = jsonTypeInvalidMessage(String(jprop_model), "string");
        jsonInvalid();
        return false;
      }
    }
  }

  if (JSON.typeof_(root[jprop_ledbarBrightness]) == "number") {
    int value = root[jprop_ledbarBrightness];
    int oldValue = jconfig[jprop_ledbarBrightness];
    if (value >= 0 && value <= 100) {
      if (value != oldValue) {
        changed = true;
        configLogInfo(String(jprop_ledbarBrightness), String(oldValue),
                      String(value));
        ledBarBrightnessChanged = true;
        jconfig[jprop_ledbarBrightness] = value;
      }
    } else {
      failedMessage = jsonValueInvalidMessage(String(jprop_ledbarBrightness),
                                              String(value));
      jsonInvalid();
      return false;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_ledbarBrightness], "number")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_ledbarBrightness), "number");
      jsonInvalid();
      return false;
    }
  }

  if (JSON.typeof_(root[jprop_displayBrightness]) == "number") {
    int value = root[jprop_displayBrightness];
    int oldValue = jconfig[jprop_displayBrightness];
    if (value >= 0 && value <= 100) {
      if (value != oldValue) {
        changed = true;
        displayBrightnessChanged = true;
        configLogInfo(String(jprop_displayBrightness), String(oldValue),
                      String(value));
        jconfig[jprop_displayBrightness] = value;
      }
    } else {
      failedMessage = jsonValueInvalidMessage(String(jprop_displayBrightness),
                                              String(value));
      jsonInvalid();
      return false;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_displayBrightness], "number")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_displayBrightness), "number");
      jsonInvalid();
      return false;
    }
  }

  if (JSON.typeof_(root["targetFirmware"]) == "string") {
    String newVer = root["targetFirmware"];
    String curVer = String(GIT_VERSION);
    if (curVer != newVer) {
      logInfo("Detected new firwmare version: " + newVer);
      otaNewFirmwareVersion = newVer;
      udpated = true;
    } else {
      otaNewFirmwareVersion = String("");
    }
  }

  if (changed) {
    udpated = true;
    saveConfig();
    printConfig();
  } else {
    logInfo("Nothing changed ignore udpate");
    if (ledBarTestRequested || co2CalibrationRequested) {
      udpated = true;
    }
  }
  return true;
}

/**
 * @brief Get current configuration value as JSON string
 *
 * @return String
 */
String Configuration::toString(void) { return JSON.stringify(jconfig); }

/**
 * @brief Temperature unit (F or C)
 *
 * @return true F
 * @return false C
 */
bool Configuration::isTemperatureUnitInF(void) {
  String unit = jconfig[jprop_temperatureUnit];
  return (unit == "f");
}

/**
 * @brief Country name, it's short name ex: TH = Thailand
 *
 * @return String
 */
String Configuration::getCountry(void) {
  String country = jconfig[jprop_country];
  return country;
}

/**
 * @brief PM unit standard (USAQI, ugm3)
 *
 * @return true USAQI
 * @return false ugm3
 */
bool Configuration::isPmStandardInUSAQI(void) {
  String standard = jconfig[jprop_pmStandard];
  return (standard == getPMStandardString(true));
}

/**
 * @brief Get CO2 calibration ABC time
 *
 * @return int Number of day
 */
int Configuration::getCO2CalibrationAbcDays(void) {
  int value = jconfig[jprop_abcDays];
  return value;
}

/**
 * @brief Get Led Bar Mode
 *
 * @return LedBarMode
 */
LedBarMode Configuration::getLedBarMode(void) {
  String mode = jconfig[jprop_ledBarMode];
  if (mode == getLedBarModeName(LedBarModeCO2)) {
    return LedBarModeCO2;
  }
  if (mode == getLedBarModeName(LedBarModeOff)) {
    return LedBarModeOff;
  }
  if (mode == getLedBarModeName(LedBarModePm)) {
    return LedBarModePm;
  }
  return LedBarModeOff;
}

/**
 * @brief Get LED bar mode name
 *
 * @return String
 */
String Configuration::getLedBarModeName(void) {
  String mode = jconfig[jprop_ledBarMode];
  return mode;
}

/**
 * @brief Get display mode
 *
 * @return true On
 * @return false Off
 */
bool Configuration::getDisplayMode(void) {
  String mode = jconfig[jprop_displayMode];
  if (mode == getDisplayModeString(true)) {
    return true;
  }
  return false;
}

/**
 * @brief Get MQTT uri
 *
 * @return String
 */
String Configuration::getMqttBrokerUri(void) {
  String broker = jconfig[jprop_mqttBrokerUrl];
  return broker;
}

/**
 * @brief Get configuratoin post data to AirGradient cloud
 *
 * @return true Post
 * @return false No-Post
 */
bool Configuration::isPostDataToAirGradient(void) {
  bool post = jconfig[jprop_postDataToAirGradient];
  return post;
}

/**
 * @brief Get current configuration control
 *
 * @return ConfigurationControl
 */
ConfigurationControl Configuration::getConfigurationControl(void) {
  String ctrl = jconfig[jprop_configurationControl];
  if (ctrl == String(CONFIGURATION_CONTROL_NAME
                         [ConfigurationControl::ConfigurationControlBoth])) {
    return ConfigurationControl::ConfigurationControlBoth;
  }
  if (ctrl == String(CONFIGURATION_CONTROL_NAME
                         [ConfigurationControl::ConfigurationControlLocal])) {
    return ConfigurationControl::ConfigurationControlLocal;
  }
  if (ctrl == String(CONFIGURATION_CONTROL_NAME
                         [ConfigurationControl::ConfigurationControlCloud])) {
    return ConfigurationControl::ConfigurationControlCloud;
  }
  return ConfigurationControl::ConfigurationControlBoth;
}

/**
 * @brief CO2 manual calib request, the request flag will clear after get. Must
 * call this after parse success
 *
 * @return true Requested
 * @return false Not requested
 */
bool Configuration::isCo2CalibrationRequested(void) {
  bool requested = co2CalibrationRequested;
  co2CalibrationRequested = false; // clear requested
  return requested;
}

/**
 * @brief LED bar test request, the request flag will clear after get. Must call
 * this function after parse success
 *
 * @return true Requested
 * @return false Not requested
 */
bool Configuration::isLedBarTestRequested(void) {
  bool requested = ledBarTestRequested;
  ledBarTestRequested = false;
  return requested;
}

/**
 * @brief Reset default configure
 */
void Configuration::reset(void) {
  defaultConfig();
  logInfo("Reset to default configure");
  printConfig();
}

/**
 * @brief Get model name, it's usage for offline mode
 *
 * @return String
 */
String Configuration::getModel(void) {
  String model = jconfig[jprop_model];
  return model;
}

bool Configuration::isUpdated(void) {
  bool updated = this->udpated;
  this->udpated = false;
  return updated;
}

String Configuration::jsonTypeInvalidMessage(String name, String type) {
  return "'" + name + "' type invalid, it's should '" + type + "'";
}

String Configuration::jsonValueInvalidMessage(String name, String value) {
  return "'" + name + "' value '" + value + "' invalid";
}

void Configuration::jsonInvalid(void) {
  loadConfig();
  logError(failedMessage);
}

void Configuration::configLogInfo(String name, String fromValue,
                                  String toValue) {
  logInfo(String("Setting '") + name + String("' from '") + fromValue +
          String("' to '") + toValue + String("'"));
}

String Configuration::getPMStandardString(bool usaqi) {
  if (usaqi) {
    return "us-aqi";
  }
  return "ugm3";
}

String Configuration::getDisplayModeString(bool dispMode) {
  if (dispMode) {
    return String("on");
  }
  return String("off");
}

String Configuration::getAbcDayString(int value) {
  if (value <= 0) {
    return String("off");
  }
  return String(value);
}

void Configuration::toConfig(const char *buf) {
  logInfo("Parse file to JSON");
  JSONVar root = JSON.parse(buf);
  if (!(root == undefined)) {
    jconfig = root;
  }

  bool changed = false;
  bool isInvalid = false;

  /** Validate country */
  if (JSON.typeof_(jconfig[jprop_country]) != "string") {
    isInvalid = true;
  } else {
    String country = jconfig[jprop_country];
    if (country.length() != 2) {
      isInvalid = true;
    } else {
      isInvalid = false;
    }
  }
  if (isInvalid) {
    jconfig[jprop_country] = "";
    changed = true;
    logInfo("toConfig: country changed");
  }

  /** validate: PM standard */
  if (JSON.typeof_(jconfig[jprop_pmStandard]) != "string") {
    isInvalid = true;
  } else {
    String standard = jconfig[jprop_pmStandard];
    if (standard != getPMStandardString(true) &&
        standard != getPMStandardString(false)) {
      isInvalid = true;
    } else {
      isInvalid = false;
    }
  }
  if (isInvalid) {
    jconfig[jprop_pmStandard] = getPMStandardString(false);
    changed = true;
    logInfo("toConfig: pmStandard changed");
  }

  /** validate led bar mode */
  if (JSON.typeof_(jconfig[jprop_ledBarMode]) != "string") {
    isInvalid = true;
  } else {
    String mode = jconfig[jprop_ledBarMode];
    if (mode != getLedBarModeName(LedBarMode::LedBarModeCO2) &&
        mode != getLedBarModeName(LedBarMode::LedBarModeOff) &&
        mode != getLedBarModeName(LedBarMode::LedBarModePm)) {
      isInvalid = true;
    } else {
      isInvalid = false;
    }
  }
  if (isInvalid) {
    jconfig[jprop_ledBarMode] = getLedBarModeName(LedBarMode::LedBarModeCO2);
    changed = true;
    logInfo("toConfig: ledBarMode changed");
  }

  /** validate display mode */
  if (JSON.typeof_(jconfig[jprop_displayMode]) != "string") {
    isInvalid = true;
  } else {
    String mode = jconfig[jprop_displayMode];
    if (mode != getDisplayModeString(true) &&
        mode != getDisplayModeString(false)) {
      isInvalid = true;
    } else {
      isInvalid = false;
    }
  }
  if (isInvalid) {
    jconfig[jprop_displayMode] = getDisplayModeString(true);
    changed = true;
    logInfo("toConfig: displayMode changed");
  }

  /** validate abcday */
  if (JSON.typeof_(jconfig[jprop_abcDays]) != "number") {
    isInvalid = true;
  } else {
    isInvalid = false;
  }
  if (isInvalid) {
    jconfig[jprop_abcDays] = 8;
    changed = true;
    logInfo("toConfig: abcDays changed");
  }

  /** validate tvoc learning offset */
  if (JSON.typeof_(jconfig[jprop_tvocLearningOffset]) != "number") {
    isInvalid = true;
  } else {
    int value = jconfig[jprop_tvocLearningOffset];
    if (value < 0) {
      isInvalid = true;
    } else {
      isInvalid = false;
    }
  }
  if (isInvalid) {
    jconfig[jprop_tvocLearningOffset] = 12;
    changed = true;
    logInfo("toConfig: tvocLearningOffset changed");
  }

  /** validate nox learning offset */
  if (JSON.typeof_(jconfig[jprop_noxLearningOffset]) != "number") {
    isInvalid = true;
  } else {
    int value = jconfig[jprop_noxLearningOffset];
    if (value < 0) {
      isInvalid = true;
    } else {
      isInvalid = false;
    }
  }
  if (isInvalid) {
    jconfig[jprop_noxLearningOffset] = 12;
    changed = true;
    logInfo("toConfig: noxLearningOffset changed");
  }

  /** validate mqtt broker */
  if (JSON.typeof_(jconfig[jprop_mqttBrokerUrl]) != "string") {
    isInvalid = true;
  } else {
    isInvalid = false;
  }
  if (isInvalid) {
    changed = true;
    jconfig[jprop_mqttBrokerUrl] = "";
    logInfo("toConfig: mqttBroker changed");
  }

  /** Validate temperature unit */
  if (JSON.typeof_(jconfig[jprop_temperatureUnit]) != "string") {
    isInvalid = true;
  } else {
    String unit = jconfig[jprop_temperatureUnit];
    if (unit != "c" && unit != "f") {
      isInvalid = true;
    } else {
      isInvalid = false;
    }
  }
  if (isInvalid) {
    jconfig[jprop_temperatureUnit] = "c";
    changed = true;
    logInfo("toConfig: temperatureUnit changed");
  }

  /** validate configuration control */
  if (JSON.typeof_(jprop_configurationControl) != "string") {
    isInvalid = true;
  } else {
    String ctrl = jconfig[jprop_configurationControl];
    if (ctrl != String(CONFIGURATION_CONTROL_NAME
                           [ConfigurationControl::ConfigurationControlBoth]) &&
        ctrl != String(CONFIGURATION_CONTROL_NAME
                           [ConfigurationControl::ConfigurationControlLocal]) &&
        ctrl != String(CONFIGURATION_CONTROL_NAME
                           [ConfigurationControl::ConfigurationControlCloud])) {
      isInvalid = true;
    } else {
      isInvalid = false;
    }
  }
  if (isInvalid) {
    jconfig[jprop_configurationControl] =
        String(CONFIGURATION_CONTROL_NAME
                   [ConfigurationControl::ConfigurationControlBoth]);
    changed = true;
    logInfo("toConfig: configurationControl changed");
  }

  /** Validate post to airgradient cloud */
  if (JSON.typeof_(jconfig[jprop_postDataToAirGradient]) != "boolean") {
    isInvalid = true;
  } else {
    isInvalid = false;
  }
  if (isInvalid) {
    jconfig[jprop_postDataToAirGradient] = true;
    changed = true;
    logInfo("toConfig: postToAirGradient changed");
  }

  /** validate led bar brightness */
  if (JSON.typeof_(jconfig[jprop_ledbarBrightness]) != "number") {
    isInvalid = true;
  } else {
    int value = jconfig[jprop_ledbarBrightness];
    if (value < 0 || value > 100) {
      isInvalid = true;
    } else {
      isInvalid = false;
    }
  }
  if (isInvalid) {
    jconfig[jprop_ledbarBrightness] = 100;
    changed = true;
    logInfo("toConfig: ledBarBrightness changed");
  }

  /** Validate display brightness */
  if (JSON.typeof_(jconfig[jprop_displayBrightness]) != "number") {
    isInvalid = true;
  } else {
    int value = jconfig[jprop_displayBrightness];
    if (value < 0 || value > 100) {
      isInvalid = true;
    } else {
      isInvalid = false;
    }
  }
  if (isInvalid) {
    jconfig[jprop_displayBrightness] = 100;
    changed = true;
    logInfo("toConfig: displayBrightness changed");
  }

  if (JSON.typeof_(jconfig[jprop_offlineMode]) != "boolean") {
    isInvalid = true;
  } else {
    isInvalid = false;
  }
  if (isInvalid) {
    jconfig[jprop_offlineMode] = false;
  }

  /** Last OTA */
  if(JSON.typeof_(jconfig[jprop_lastOta]) != "number") {
    isInvalid = true;
  } else {
    isInvalid = false;
  }
  if(isInvalid) {
    jconfig[jprop_lastOta] = 0;
    changed = true;
  }

  if (changed) {
    saveConfig();
  }
}

String Configuration::getFailedMesage(void) { return failedMessage; }

void Configuration::setPostToAirGradient(bool enable) {
  bool oldEnabled = jconfig[jprop_postDataToAirGradient];
  if (enable != oldEnabled) {
    jconfig[jprop_postDataToAirGradient] = enable;
    logInfo("postDataToAirGradient set to: " + String(enable));
    saveConfig();
  } else {
    logInfo("postDataToAirGradient: Ignored set to " + String(enable));
  }
}

bool Configuration::noxLearnOffsetChanged(void) {
  bool changed = _noxLearnOffsetChanged;
  _noxLearnOffsetChanged = false;
  return changed;
}

bool Configuration::tvocLearnOffsetChanged(void) {
  bool changed = _tvocLearningOffsetChanged;
  _tvocLearningOffsetChanged = false;
  return changed;
}

int Configuration::getTvocLearningOffset(void) {
  int value = jconfig[jprop_tvocLearningOffset];
  return value;
}

int Configuration::getNoxLearningOffset(void) {
  int value = jconfig[jprop_noxLearningOffset];
  return value;
}

String Configuration::wifiSSID(void) { return "airgradient-" + ag->deviceId(); }

String Configuration::wifiPass(void) { return String("cleanair"); }

void Configuration::setAirGradient(AirGradient *ag) { this->ag = ag; }

int Configuration::getLedBarBrightness(void) {
  int value = jconfig[jprop_ledbarBrightness];
  return value;
}

bool Configuration::isLedBarBrightnessChanged(void) {
  bool changed = ledBarBrightnessChanged;
  ledBarBrightnessChanged = false;
  return changed;
}

int Configuration::getDisplayBrightness(void) {
  int value = jconfig[jprop_displayBrightness];
  return value;
}

bool Configuration::isOfflineMode(void) {
  bool offline = jconfig[jprop_offlineMode];
  return offline;
}

void Configuration::setOfflineMode(bool offline) {
  logInfo("Set offline mode: " + String(offline ? "True" : "False"));
  jconfig[jprop_offlineMode] = offline;
  saveConfig();
}

bool Configuration::isDisplayBrightnessChanged(void) {
  bool changed = displayBrightnessChanged;
  displayBrightnessChanged = false;
  return changed;
}

/**
 * @brief Get number of sec from last OTA
 *
 * @return int < 0 is invalid, 0 mean there is no OTA trigger.
 */
int Configuration::getLastOta(void) {
  struct tm timeInfo;
  if (getLocalTime(&timeInfo) == false) {
    logError("Get localtime failed");
    return -1;
  }
  int curYear = timeInfo.tm_year + 1900;
  if (curYear < 2024) {
    logError("Current year " + String(curYear) + String(" invalid"));
    return -1;
  }
  double _t = jconfig[jprop_lastOta];
  time_t lastOta = (time_t)_t;
  time_t curTime = mktime(&timeInfo);
  logInfo("Last ota time: " + String(lastOta));
  if (lastOta == 0) {
    return 0;
  }

  int sec = curTime - lastOta;
  logInfo("Last ota secconds: " + String(sec));
  return sec;
}

void Configuration::updateLastOta(void) {
  struct tm timeInfo;
  if (getLocalTime(&timeInfo) == false) {
    logError("updateLastOta: Get localtime failed");
    return;
  }
  int curYear = timeInfo.tm_year + 1900;
  if (curYear < 2024) {
    logError("updateLastOta: lolcal time invalid");
    return;
  }
  
  time_t lastOta = mktime(&timeInfo);
  jconfig[jprop_lastOta] = (unsigned long)lastOta;
  logInfo("Last OTA: " + String(lastOta));
  saveConfig();
}

String Configuration::newFirmwareVersion(void) {
  String newFw = otaNewFirmwareVersion;
  otaNewFirmwareVersion = String("");
  return newFw;
}
