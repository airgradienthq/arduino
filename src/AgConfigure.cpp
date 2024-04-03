#include "AgConfigure.h"
#include "EEPROM.h"

const char *CONFIGURATION_CONTROL_NAME[] = {
    [ConfigurationControlLocal] = "local",
    [ConfigurationControlCloud] = "cloud",
    [ConfigurationControlBoth] = "both"};

String AgConfigure::getLedBarModeName(LedBarMode mode) {
  LedBarMode ledBarMode = mode;
  if (ledBarMode == LedBarModeOff) {
    return String("off");
  } else if (ledBarMode == LedBarModePm) {
    return String("pm");
  } else if (ledBarMode == LedBarModeCO2) {
    return String("co2");
  } else {
    return String("off");
  }
}

void AgConfigure::saveConfig(void) {
  config._check = 0;
  int len = sizeof(config) - sizeof(config._check);
  uint8_t *data = (uint8_t *)&config;
  for (int i = 0; i < len; i++) {
    config._check += data[i];
  }
  EEPROM.writeBytes(0, &config, sizeof(config));
  EEPROM.commit();
  logInfo("Save Config");
}

void AgConfigure::loadConfig(void) {
  if (EEPROM.readBytes(0, &config, sizeof(config)) != sizeof(config)) {
    logError("Load configure failed");
    defaultConfig();
  } else {
    uint32_t sum = 0;
    uint8_t *data = (uint8_t *)&config;
    int len = sizeof(config) - sizeof(config._check);
    for (int i = 0; i < len; i++) {
      sum += data[i];
    }

    if (sum != config._check) {
      logError("Configure validate invalid");
      defaultConfig();
    }
  }
}
void AgConfigure::defaultConfig(void) {
  // Default country is null
  memset(config.country, 0, sizeof(config.country));
  // Default MQTT broker is null.
  memset(config.mqttBroker, 0, sizeof(config.mqttBroker));

  config.configurationControl = ConfigurationControl::ConfigurationControlBoth;
  config.inUSAQI = false; // pmStandard = ugm3
  config.inF = false;
  config.postDataToAirGradient = true;
  config.displayMode = true;
  config.useRGBLedBar = LedBarMode::LedBarModeCO2;
  config.abcDays = 7;
  config.tvocLearningOffset = 12;
  config.noxLearningOffset = 12;
  config.temperatureUnit = 'c';

  saveConfig();
}

void AgConfigure::printConfig(void) { logInfo(toString().c_str()); }

AgConfigure::AgConfigure(Stream &debugLog) : PrintLog(debugLog, "Configure") {}

AgConfigure::~AgConfigure() {}

bool AgConfigure::begin(void) {
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
bool AgConfigure::parse(String data, bool isLocal) {
  JSONVar root = JSON.parse(data);
  if (JSON.typeof_(root) == "undefined") {
    logError("Configuration JSON invalid");
    return false;
  }
  logInfo("Parse configure success");

  /** Is configuration changed */
  bool changed = false;

  /** Get ConfigurationControl */
  if (JSON.typeof_(root["configurationControl"]) == "string") {
    String configurationControl = root["configurationControl"];
    if (configurationControl ==
        String(CONFIGURATION_CONTROL_NAME
                   [ConfigurationControl::ConfigurationControlLocal])) {
      config.configurationControl =
          (uint8_t)ConfigurationControl::ConfigurationControlLocal;
      changed = true;
    } else if (configurationControl ==
               String(CONFIGURATION_CONTROL_NAME
                          [ConfigurationControl::ConfigurationControlCloud])) {
      config.configurationControl =
          (uint8_t)ConfigurationControl::ConfigurationControlCloud;
      changed = true;
    } else if (configurationControl ==
               String(CONFIGURATION_CONTROL_NAME
                          [ConfigurationControl::ConfigurationControlBoth])) {
      config.configurationControl =
          (uint8_t)ConfigurationControl::ConfigurationControlBoth;
      changed = true;
    } else {
      logError("'configurationControl' value '" + configurationControl +
               "' invalid");
      return false;
    }
  } else {
    return false;
  }

  if ((config.configurationControl ==
       (byte)ConfigurationControl::ConfigurationControlCloud)) {
    logWarning("Ignore, cause ConfigurationControl is " +
               String(CONFIGURATION_CONTROL_NAME[config.configurationControl]));
    return false;
  }

  if (JSON.typeof_(root["country"]) == "string") {
    String country = root["country"];
    if (country.length() == 2) {
      if (country != String(config.country)) {
        changed = true;
        snprintf(config.country, sizeof(config.country), country.c_str());
        logInfo("Set country: " + country);
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
      logInfo("Country name " + country +
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
      logInfo("Set PM standard: " + pmStandard);
    }
  }

  if (JSON.typeof_(root["co2CalibrationRequested"]) == "boolean") {
    co2CalibrationRequested = root["co2CalibrationRequested"];
    logInfo("Set co2CalibrationRequested: " + String(co2CalibrationRequested));
  }

  if (JSON.typeof_(root["ledBarTestRequested"]) == "boolean") {
    ledBarTestRequested = root["ledBarTestRequested"];
    logInfo("Set ledBarTestRequested: " + String(ledBarTestRequested));
  }

  if (JSON.typeof_(root["ledBarMode"]) == "string") {
    String mode = root["ledBarMode"];
    uint8_t ledBarMode = config.useRGBLedBar;
    if (mode == "co2") {
      ledBarMode = LedBarModeCO2;
    } else if (mode == "pm") {
      ledBarMode = LedBarModePm;
    } else if (mode == "off") {
      ledBarMode = LedBarModeOff;
    } else {
      ledBarMode = config.useRGBLedBar;
      logInfo("ledBarMode value '" + mode + "' invalid");
    }

    if (ledBarMode != config.useRGBLedBar) {
      config.useRGBLedBar = ledBarMode;
      changed = true;
      logInfo("Set ledBarMode: " + mode);
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
      logInfo("displayMode '" + mode + "' invalid");
    }

    if (displayMode != config.displayMode) {
      changed = true;
      config.displayMode = displayMode;
      logInfo("Set displayMode: " + mode);
    }
  }

  if (JSON.typeof_(root["abcDays"]) == "number") {
    int abcDays = root["abcDays"];
    if (abcDays != config.abcDays) {
      config.abcDays = abcDays;
      changed = true;
      logInfo("Set abcDays: " + String(abcDays));
    }
  }

  if (JSON.typeof_(root["tvocLearningOffset"]) == "number") {
    int tvocLearningOffset = root["tvocLearningOffset"];
    if (tvocLearningOffset != config.tvocLearningOffset) {
      changed = true;
      config.tvocLearningOffset = tvocLearningOffset;
      logInfo("Set tvocLearningOffset: " + String(tvocLearningOffset));
    }
  }

  if (JSON.typeof_(root["noxLearningOffset"]) == "number") {
    int noxLearningOffset = root["noxLearningOffset"];
    if (noxLearningOffset != config.noxLearningOffset) {
      changed = true;
      config.noxLearningOffset = noxLearningOffset;
      logInfo("Set noxLearningOffset: " + String(noxLearningOffset));
    }
  }

  if (JSON.typeof_(root["mqttBrokerUrl"]) == "string") {
    String broker = root["mqttBrokerUrl"];
    if (broker.length() < sizeof(config.mqttBroker)) {
      if (broker != String(config.mqttBroker)) {
        changed = true;
        snprintf(config.mqttBroker, sizeof(config.mqttBroker), broker.c_str());
        logInfo("Set mqttBrokerUrl: " + broker);
      }
    } else {
      logError("Error: mqttBroker length invalid: " + String(broker.length()));
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
      logInfo("set temperatureUnit: null");
    } else {
      logInfo("set temperatureUnit: " + String(temperatureUnit));
    }
  }

  if (JSON.typeof_(root["postDataToAirGradient"]) == "boolean") {
    bool post = root["postDataToAirGradient"];
    if (post != config.postDataToAirGradient) {
      changed = true;
      config.postDataToAirGradient = post;
      logInfo("Set postDataToAirGradient: " + String(post));
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
        logError("Error: modal name length invalid: " + String(model.length()));
      }
    }
  }

  if (changed) {
    saveConfig();
  }
  printConfig();

  return true;
}

String AgConfigure::toString(void) {
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

bool AgConfigure::isTemperatureUnitInF(void) {
  return (config.temperatureUnit == 'f');
}

String AgConfigure::getCountry(void) { return String(config.country); }

bool AgConfigure::isPmStandardInUSAQI(void) { return config.inUSAQI; }

int AgConfigure::getCO2CalirationAbcDays(void) { return config.abcDays; }

LedBarMode AgConfigure::getLedBarMode(void) {
  return (LedBarMode)config.useRGBLedBar;
}

String AgConfigure::getLedBarModeName(void) {
  return getLedBarModeName((LedBarMode)config.useRGBLedBar);
}

bool AgConfigure::getDisplayMode(void) { return config.displayMode; }

String AgConfigure::getMqttBrokerUri(void) { return String(config.mqttBroker); }

bool AgConfigure::isPostDataToAirGradient(void) {
  return config.postDataToAirGradient;
}

ConfigurationControl AgConfigure::getConfigurationControl(void) {
  return (ConfigurationControl)config.configurationControl;
}

/**
 * @brief CO2 manual calib request, the request flag will clear after get. Must
 * call this after parse success
 *
 * @return true Requested
 * @return false Not requested
 */
bool AgConfigure::isCo2CalibrationRequested(void) {
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
bool AgConfigure::isLedBarTestRequested(void) {
  bool requested = ledBarTestRequested;
  ledBarTestRequested = false;
  return requested;
}

/**
 * @brief Reset default configure
 */
void AgConfigure::reset(void) {
  defaultConfig();
  logInfo("Reset to default configure");
  printConfig();
}

/**
 * @brief Get model name, it's usage for offline mode
 *
 * @return String
 */
String AgConfigure::getModel(void) { return String(config.model); }
