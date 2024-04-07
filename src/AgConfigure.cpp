#include "AgConfigure.h"
#include "EEPROM.h"

const char *CONFIGURATION_CONTROL_NAME[] = {
    [ConfigurationControlLocal] = "local",
    [ConfigurationControlCloud] = "cloud",
    [ConfigurationControlBoth] = "both"};

const char *LED_BAR_MODE_NAMES[] = {
    [LedBarModeOff] = "off",
    [LedBarModePm] = "pm",
    [LedBarModeCO2] = "co2",
};

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
  config._check = 0;
  int len = sizeof(config) - sizeof(config._check);
  uint8_t *data = (uint8_t *)&config;
  for (int i = 0; i < len; i++) {
    config._check += data[i];
  }
#ifdef ESP8266
  for (int i = 0; i < sizeof(config); i++) {
    EEPROM.write(i, data[i]);
  }
#else
  EEPROM.writeBytes(0, &config, sizeof(config));
#endif
  EEPROM.commit();
  logInfo("Save Config");
}

void Configuration::loadConfig(void) {
  bool readSuccess = false;
#ifdef ESP8266
  uint8_t *data = (uint8_t *)&config;
  for (int i = 0; i < sizeof(config); i++) {
    data[i] = EEPROM.read(i);
  }
  readSuccess = true;
#else
  if (EEPROM.readBytes(0, &config, sizeof(config)) != sizeof(config)) {
    readSuccess = true;
  }
#endif

  if (readSuccess) {
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

/**
 * @brief Set configuration default
 *
 */
void Configuration::defaultConfig(void) {
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

/**
 * @brief Show configuration as JSON string message over log
 *
 */
void Configuration::printConfig(void) { logInfo(toString().c_str()); }

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
bool Configuration::parse(String data, bool isLocal) {
  JSONVar root = JSON.parse(data);
  if (JSON.typeof_(root) == "undefined") {
    logError("Configuration JSON invalid");
    return false;
  }
  logInfo("Parse configure success");

  /** Is configuration changed */
  bool changed = false;

  /** Get ConfigurationControl */
  if (isLocal) {
    if (JSON.typeof_(root["configurationControl"]) == "string") {
      String configurationControl = root["configurationControl"];
      if (configurationControl ==
          String(CONFIGURATION_CONTROL_NAME
                     [ConfigurationControl::ConfigurationControlLocal])) {
        config.configurationControl =
            (uint8_t)ConfigurationControl::ConfigurationControlLocal;
        changed = true;
      } else if (configurationControl ==
                 String(
                     CONFIGURATION_CONTROL_NAME
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

        logError(String("'configurationControl' value '" +
                        configurationControl + "' invalid")
                     .c_str());
        return false;
      }
    } else {
      return false;
    }

    if ((config.configurationControl ==
         (byte)ConfigurationControl::ConfigurationControlCloud)) {
      logWarning("Local configure ignored");
      return false;
    }
  } else {
    if (config.configurationControl ==
        (byte)ConfigurationControl::ConfigurationControlLocal) {
      logWarning("Cloud configure ignored");
      return false;
    }
  }

  if (JSON.typeof_(root["country"]) == "string") {
    String country = root["country"];
    if (country.length() == 2) {
      if (country != String(config.country)) {
        changed = true;
        snprintf(config.country, sizeof(config.country), country.c_str());
        logInfo(String("Set country: " + country).c_str());
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
    uint8_t ledBarMode = LedBarModeOff;
    if (mode == String(LED_BAR_MODE_NAMES[LedBarModeCO2])) {
      ledBarMode = LedBarModeCO2;
    } else if (mode == String(LED_BAR_MODE_NAMES[LedBarModePm])) {
      ledBarMode = LedBarModePm;
    } else if (mode == String(LED_BAR_MODE_NAMES[LedBarModeOff])) {
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

  udpated = true;
  return true;
}

/**
 * @brief Get current configuration value as JSON string
 *
 * @return String
 */
String Configuration::toString(void) {
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

/**
 * @brief Temperature unit (F or C)
 *
 * @return true F
 * @return false C
 */
bool Configuration::isTemperatureUnitInF(void) {
  return (config.temperatureUnit == 'f');
}

/**
 * @brief Country name, it's short name ex: TH = Thailand
 *
 * @return String
 */
String Configuration::getCountry(void) { return String(config.country); }

/**
 * @brief PM unit standard (USAQI, ugm3)
 *
 * @return true USAQI
 * @return false ugm3
 */
bool Configuration::isPmStandardInUSAQI(void) { return config.inUSAQI; }

/**
 * @brief Get CO2 calibration ABC time
 *
 * @return int Number of day
 */
int Configuration::getCO2CalibrationAbcDays(void) { return config.abcDays; }

/**
 * @brief Get Led Bar Mode
 *
 * @return LedBarMode
 */
LedBarMode Configuration::getLedBarMode(void) {
  return (LedBarMode)config.useRGBLedBar;
}

/**
 * @brief Get LED bar mode name
 *
 * @return String
 */
String Configuration::getLedBarModeName(void) {
  return getLedBarModeName((LedBarMode)config.useRGBLedBar);
}

/**
 * @brief Get display mode
 *
 * @return true On
 * @return false Off
 */
bool Configuration::getDisplayMode(void) { return config.displayMode; }

/**
 * @brief Get MQTT uri
 *
 * @return String
 */
String Configuration::getMqttBrokerUri(void) {
  return String(config.mqttBroker);
}

/**
 * @brief Get configuratoin post data to AirGradient cloud
 *
 * @return true Post
 * @return false No-Post
 */
bool Configuration::isPostDataToAirGradient(void) {
  return config.postDataToAirGradient;
}

/**
 * @brief Get current configuration control
 *
 * @return ConfigurationControl
 */
ConfigurationControl Configuration::getConfigurationControl(void) {
  return (ConfigurationControl)config.configurationControl;
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
String Configuration::getModel(void) { return String(config.model); }

bool Configuration::isUpdated(void) {
  bool updated = this->udpated;
  this->udpated = false;
  return updated;
}
