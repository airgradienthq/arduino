#include "LocalConfig.h"
#include "EEPROM.h"

const char *CONFIGURATION_CONTROL_NAME[] = {
    [Local] = "local", [Cloud] = "cloud", [Both] = "both"};

void LocalConfig::printLog(String log) {
  debugLog.printf("[LocalConfig] %s\r\n", log.c_str());
}

String LocalConfig::getLedBarModeName(UseLedBar mode) {
  UseLedBar ledBarMode = mode;
  if (ledBarMode == UseLedBarOff) {
    return String("off");
  } else if (ledBarMode == UseLedBarPM) {
    return String("pm");
  } else if (ledBarMode == UseLedBarCO2) {
    return String("co2");
  } else {
    return String("off");
  }
}

void LocalConfig::saveConfig(void) {
  config._check = 0;
  int len = sizeof(config) - sizeof(config._check);
  uint8_t *data = (uint8_t *)&config;
  for (int i = 0; i < len; i++) {
    config._check += data[i];
  }
  EEPROM.writeBytes(0, &config, sizeof(config));
  EEPROM.commit();
  printLog("Save Config");
}

void LocalConfig::loadConfig(void) {
  if (EEPROM.readBytes(0, &config, sizeof(config)) != sizeof(config)) {
    printLog("Load configure failed");
    defaultConfig();
  } else {
    uint32_t sum = 0;
    uint8_t *data = (uint8_t *)&config;
    int len = sizeof(config) - sizeof(config._check);
    for (int i = 0; i < len; i++) {
      sum += data[i];
    }

    if (sum != config._check) {
      printLog("Configure validate invalid");
      defaultConfig();
    }
  }
}
void LocalConfig::defaultConfig(void) {
  // Default country is null
  memset(config.country, 0, sizeof(config.country));
  // Default MQTT broker is null.
  memset(config.mqttBroker, 0, sizeof(config.mqttBroker));

  config.configurationControl = ConfigurationControl::Both;
  config.inUSAQI = false; // pmStandard = ugm3
  config.inF = false;
  config.postDataToAirGradient = true;
  config.displayMode = true;
  config.useRGBLedBar = UseLedBar::UseLedBarCO2;
  config.abcDays = 7;
  config.tvocLearningOffset = 12;
  config.noxLearningOffset = 12;
  config.temperatureUnit = 'c';

  saveConfig();
}

void LocalConfig::printConfig(void) { printLog(toString()); }

LocalConfig::LocalConfig(Stream &debugLog) : debugLog(debugLog) {}

LocalConfig::~LocalConfig() {}

bool LocalConfig::begin(void) {
  EEPROM.begin(512);
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
bool LocalConfig::parse(String data, bool isLocal) {
  JSONVar root = JSON.parse(data);
  if (JSON.typeof_(root) == "undefined") {
    printLog("Configuration JSON invalid");
    return false;
  }
  printLog("Parse configure success");

  /** Is configuration changed */
  bool changed = false;

  /** Get ConfigurationControl */
  if (JSON.typeof_(root["configurationControl"]) == "string") {
    String configurationControl = root["configurationControl"];
    if (configurationControl ==
        String(CONFIGURATION_CONTROL_NAME[ConfigurationControl::Local])) {
      config.configurationControl = (uint8_t)ConfigurationControl::Local;
      changed = true;
    } else if (configurationControl ==
               String(
                   CONFIGURATION_CONTROL_NAME[ConfigurationControl::Cloud])) {
      config.configurationControl = (uint8_t)ConfigurationControl::Cloud;
      changed = true;
    } else if (configurationControl ==
               String(CONFIGURATION_CONTROL_NAME[ConfigurationControl::Both])) {
      config.configurationControl = (uint8_t)ConfigurationControl::Both;
      changed = true;
    } else {
      printLog("'configurationControl' value '" + configurationControl +
               "' invalid");
      return false;
    }
  } else {
    return false;
  }

  if ((config.configurationControl == (byte)ConfigurationControl::Cloud)) {
    printLog("Ignore, cause ConfigurationControl is " +
             String(CONFIGURATION_CONTROL_NAME[config.configurationControl]));
    return false;
  }

  if (JSON.typeof_(root["country"]) == "string") {
    String country = root["country"];
    if (country.length() == 2) {
      if (country != String(config.country)) {
        changed = true;
        snprintf(config.country, sizeof(config.country), country.c_str());
        printLog("Set country: " + country);
      }

      // Update temperature unit if get configuration from server
      if (isLocal == false) {
        if (country == "US") {
          if (config.temperatureUnit == 'c') {
            changed = true;
            config.temperatureUnit = 'f';
          }
        } else {
          if (config.temperatureUnit == 'f') {
            changed = true;
            config.temperatureUnit = 'c';
          }
        }
      }
    } else {
      printLog("Country name " + country +
               " invalid. Find details here (ALPHA-2): "
               "https://www.iban.com/country-codes");
    }
  }

  if (JSON.typeof_(root["pmStandard"]) == "string") {
    String pmStandard = root["pmStandard"];
    bool inUSAQI = true;
    if (pmStandard == "ugm3") {
      inUSAQI = false;
    }

    if (inUSAQI != config.inUSAQI) {
      config.inUSAQI = inUSAQI;
      changed = true;
      printLog("Set PM standard: " + pmStandard);
    }
  }

  if (JSON.typeof_(root["co2CalibrationRequested"]) == "boolean") {
    co2CalibrationRequested = root["co2CalibrationRequested"];
    printLog("Set co2CalibrationRequested: " + String(co2CalibrationRequested));
  }

  if (JSON.typeof_(root["ledBarTestRequested"]) == "boolean") {
    ledBarTestRequested = root["ledBarTestRequested"];
    printLog("Set ledBarTestRequested: " + String(ledBarTestRequested));
  }

  if (JSON.typeof_(root["ledBarMode"]) == "string") {
    String mode = root["ledBarMode"];
    uint8_t ledBarMode = config.useRGBLedBar;
    if (mode == "co2") {
      ledBarMode = UseLedBarCO2;
    } else if (mode == "pm") {
      ledBarMode = UseLedBarPM;
    } else if (mode == "off") {
      ledBarMode = UseLedBarOff;
    } else {
      ledBarMode = config.useRGBLedBar;
      printLog("ledBarMode value '" + mode + "' invalid");
    }

    if (ledBarMode != config.useRGBLedBar) {
      config.useRGBLedBar = ledBarMode;
      changed = true;
      printLog("Set ledBarMode: " + mode);
    }
  }

  if (JSON.typeof_(root["displayMode"]) == "string") {
    String mode = root["displayMode"];
    bool displayMode = false;
    if (mode == "on") {
      displayMode = true;
    } else if (mode == "off") {
      displayMode = false;
    } else {
      displayMode = config.displayMode;
      printLog("displayMode '" + mode + "' invalid");
    }

    if (displayMode != config.displayMode) {
      changed = true;
      config.displayMode = displayMode;
      printLog("Set displayMode: " + mode);
    }
  }

  if (JSON.typeof_(root["abcDays"]) == "number") {
    int abcDays = root["abcDays"];
    if (abcDays != config.abcDays) {
      config.abcDays = abcDays;
      changed = true;
      printLog("Set abcDays: " + String(abcDays));
    }
  }

  if (JSON.typeof_(root["tvocLearningOffset"]) == "number") {
    int tvocLearningOffset = root["tvocLearningOffset"];
    if (tvocLearningOffset != config.tvocLearningOffset) {
      changed = true;
      config.tvocLearningOffset = tvocLearningOffset;
      printLog("Set tvocLearningOffset: " + String(tvocLearningOffset));
    }
  }

  if (JSON.typeof_(root["noxLearningOffset"]) == "number") {
    int noxLearningOffset = root["noxLearningOffset"];
    if (noxLearningOffset != config.noxLearningOffset) {
      changed = true;
      config.noxLearningOffset = noxLearningOffset;
      printLog("Set noxLearningOffset: " + String(noxLearningOffset));
    }
  }

  if (JSON.typeof_(root["mqttBrokerUrl"]) == "string") {
    String broker = root["mqttBrokerUrl"];
    if (broker.length() < sizeof(config.mqttBroker)) {
      if (broker != String(config.mqttBroker)) {
        changed = true;
        snprintf(config.mqttBroker, sizeof(config.mqttBroker), broker.c_str());
        printLog("Set mqttBrokerUrl: " + broker);
      }
    } else {
      printLog("Error: mqttBroker length invalid: " + String(broker.length()));
    }
  }

  char temperatureUnit = 0;
  if (JSON.typeof_(root["temperatureUnit"]) == "string") {
    String unit = root["temperatureUnit"];
    if (unit == "c" || unit == "C") {
      temperatureUnit = 'c';
    } else if (unit == "f" || unit == "F") {
      temperatureUnit = 'f';
    } else {
      temperatureUnit = 0;
    }
  }

  if (temperatureUnit != config.temperatureUnit) {
    changed = true;
    config.temperatureUnit = temperatureUnit;
    if (temperatureUnit == 0) {
      printLog("set temperatureUnit: null");
    } else {
      printLog("set temperatureUnit: " + String(temperatureUnit));
    }
  }

  if (JSON.typeof_(root["postDataToAirGradient"]) == "boolean") {
    bool post = root["postDataToAirGradient"];
    if (post != config.postDataToAirGradient) {
      changed = true;
      config.postDataToAirGradient = post;
      printLog("Set postDataToAirGradient: " + String(post));
    }
  }

  /** Parse data only got from AirGradient server */
  if (isLocal == false) {
    if (JSON.typeof_(root["model"]) == "string") {
      String model = root["model"];
      if (model.length() < sizeof(config.model)) {
        if (model != String(config.model)) {
          changed = true;
          snprintf(config.model, sizeof(config.model), model.c_str());
        }
      } else {
        printLog("Error: modal name length invalid: " + String(model.length()));
      }
    }
  }

  if (changed) {
    saveConfig();
  }
  printConfig();

  return true;
}

String LocalConfig::toString(void) {
  JSONVar root;

  /** "country" */
  root["Country"] = String(config.country);

  /** "pmStandard" */
  if (config.inUSAQI) {
    root["pmStandard"] = "USAQI";
  } else {
    root["pmStandard"] = "ugm3";
  }

  /** co2CalibrationRequested */
  /** ledBarTestRequested */

  /** "ledBarMode" */
  root["ledBarMode"] = getLedBarModeName();

  /** "displayMode" */
  root["displayMode"] = config.displayMode;

  /** "abcDays" */
  root["abcDays"] = config.abcDays;

  /** "tvocLearningOffset" */
  root["tvocLearningOffset"] = config.tvocLearningOffset;

  /** "noxLearningOffset" */
  root["noxLearningOffset"] = config.noxLearningOffset;

  /** "mqttBrokerUrl" */
  root["mqttBrokerUrl"] = String(config.mqttBroker);

  /** "temperatureUnit" */
  root["temperatureUnit"] = String(config.temperatureUnit);

  /** configurationControl */
  root["configurationControl"] =
      String(CONFIGURATION_CONTROL_NAME[config.configurationControl]);

  /** "postDataToAirGradient" */
  root["postDataToAirGradient"] = config.postDataToAirGradient;

  return JSON.stringify(root);
}

bool LocalConfig::isTemperatureUnitInF(void) {
  return (config.temperatureUnit == 'f');
}

String LocalConfig::getCountry(void) { return String(config.country); }

bool LocalConfig::isPmStandardInUSAQI(void) { return config.inUSAQI; }

int LocalConfig::getCO2CalirationAbcDays(void) { return config.abcDays; }

UseLedBar LocalConfig::getLedBarMode(void) {
  return (UseLedBar)config.useRGBLedBar;
}

String LocalConfig::getLedBarModeName(void) {
  return getLedBarModeName((UseLedBar)config.useRGBLedBar);
}

bool LocalConfig::getDisplayMode(void) { return config.displayMode; }

String LocalConfig::getMqttBrokerUri(void) { return String(config.mqttBroker); }

bool LocalConfig::isPostDataToAirGradient(void) {
  return config.postDataToAirGradient;
}

ConfigurationControl LocalConfig::getConfigurationControl(void) {
  return (ConfigurationControl)config.configurationControl;
}

/**
 * @brief CO2 manual calib request, the request flag will clear after get. Must
 * call this after parse success
 *
 * @return true Requested
 * @return false Not requested
 */
bool LocalConfig::isCo2CalibrationRequested(void) {
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
bool LocalConfig::isLedBarTestRequested(void) {
  bool requested = ledBarTestRequested;
  ledBarTestRequested = false;
  return requested;
}

/**
 * @brief Reset default configure
 */
void LocalConfig::reset(void) {
  defaultConfig();
  printLog("Reset to default configure");
  printConfig();
}

/**
 * @brief Get model name, it's usage for offline mode
 *
 * @return String
 */
String LocalConfig::getModel(void) { return String(config.model); }
