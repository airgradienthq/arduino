#include "AgConfigure.h"
#if ESP32
#include "FS.h"
#include "SPIFFS.h"
#else
#include "EEPROM.h"
#endif
#include <time.h>

#define EEPROM_CONFIG_SIZE 1024
#define CONFIG_FILE_NAME "/AgConfigure_Configuration.json"

const char *CONFIGURATION_CONTROL_NAME[] = {
    [ConfigurationControlLocal] = "local",
    [ConfigurationControlCloud] = "cloud",
    [ConfigurationControlBoth] = "both"};

const char *LED_BAR_MODE_NAMES[] = {
    [LedBarModeOff] = "off",
    [LedBarModePm] = "pm",
    [LedBarModeCO2] = "co2",
};

const char *PM_CORRECTION_ALGORITHM_NAMES[] = {
    [COR_ALGO_PM_UNKNOWN] = "-", // This is only to pass "non-trivial designated initializers" error
    [COR_ALGO_PM_NONE] = "none",
    [COR_ALGO_PM_EPA_2021] = "epa_2021",
    [COR_ALGO_PM_SLR_CUSTOM] = "custom",
};

const char *TEMP_HUM_CORRECTION_ALGORITHM_NAMES[] = {
    [COR_ALGO_TEMP_HUM_UNKNOWN] = "-", // This is only to pass "non-trivial designated initializers" error
    [COR_ALGO_TEMP_HUM_NONE] = "none",
    [COR_ALGO_TEMP_HUM_AG_PMS5003T_2024] = "ag_pms5003t_2024",
    [COR_ALGO_TEMP_HUM_SLR_CUSTOM] = "custom",
};

#define JSON_PROP_NAME(name) jprop_##name
#define JSON_PROP_DEF(name) const char *JSON_PROP_NAME(name) = #name

JSON_PROP_DEF(model);
JSON_PROP_DEF(country);
JSON_PROP_DEF(pmStandard);
JSON_PROP_DEF(ledBarMode);
JSON_PROP_DEF(abcDays);
JSON_PROP_DEF(tvocLearningOffset);
JSON_PROP_DEF(noxLearningOffset);
JSON_PROP_DEF(mqttBrokerUrl);
JSON_PROP_DEF(httpDomain);
JSON_PROP_DEF(temperatureUnit);
JSON_PROP_DEF(configurationControl);
JSON_PROP_DEF(postDataToAirGradient);
JSON_PROP_DEF(disableCloudConnection);
JSON_PROP_DEF(ledBarBrightness);
JSON_PROP_DEF(displayBrightness);
JSON_PROP_DEF(co2CalibrationRequested);
JSON_PROP_DEF(ledBarTestRequested);
JSON_PROP_DEF(offlineMode);
JSON_PROP_DEF(monitorDisplayCompensatedValues);
JSON_PROP_DEF(corrections);
JSON_PROP_DEF(atmp);
JSON_PROP_DEF(rhum);

#define jprop_model_default                           ""
#define jprop_country_default                         "TH"
#define jprop_pmStandard_default                      getPMStandardString(false)
#define jprop_ledBarMode_default                      getLedBarModeName(LedBarMode::LedBarModeCO2)
#define jprop_abcDays_default                         8
#define jprop_tvocLearningOffset_default              12
#define jprop_noxLearningOffset_default               12
#define jprop_mqttBrokerUrl_default                   ""
#define jprop_httpDomain_default                      ""
#define jprop_temperatureUnit_default                 "c"
#define jprop_configurationControl_default            String(CONFIGURATION_CONTROL_NAME[ConfigurationControl::ConfigurationControlBoth])
#define jprop_postDataToAirGradient_default           true
#define jprop_disableCloudConnection_default          false
#define jprop_ledBarBrightness_default                100
#define jprop_displayBrightness_default               100
#define jprop_offlineMode_default                     false
#define jprop_monitorDisplayCompensatedValues_default false

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

PMCorrectionAlgorithm Configuration::matchPmAlgorithm(String algorithm) {
  // Loop through all algorithm names in the PM_CORRECTION_ALGORITHM_NAMES array
  // If the input string matches an algorithm name, return the corresponding enum value
  // Else return Unknown

  const size_t enumSize = COR_ALGO_PM_SLR_CUSTOM + 1; // Get the actual size of the enum
  PMCorrectionAlgorithm result = COR_ALGO_PM_UNKNOWN;;

  // Loop through enum values
  for (size_t enumVal = 0; enumVal < enumSize; enumVal++) {
    if (algorithm == PM_CORRECTION_ALGORITHM_NAMES[enumVal]) {
      result = static_cast<PMCorrectionAlgorithm>(enumVal);
    }
  }

  // If string not match from enum, check if correctionAlgorithm is one of the PM batch corrections
  if (result == COR_ALGO_PM_UNKNOWN) {
    // Check the substring "slr_PMS5003_xxxxxxxx"
    if (algorithm.substring(0, 11) == "slr_PMS5003") {
      // If it is, then its a custom correction
      result = COR_ALGO_PM_SLR_CUSTOM;
    }
  }

  return result;
}

TempHumCorrectionAlgorithm Configuration::matchTempHumAlgorithm(String algorithm) {
  // Get the actual size of the enum
  const int enumSize = static_cast<int>(COR_ALGO_TEMP_HUM_SLR_CUSTOM);
  TempHumCorrectionAlgorithm result = COR_ALGO_TEMP_HUM_UNKNOWN;

  // Loop through enum values
  for (size_t enumVal = 0; enumVal <= enumSize; enumVal++) {
    if (algorithm == TEMP_HUM_CORRECTION_ALGORITHM_NAMES[enumVal]) {
      result = static_cast<TempHumCorrectionAlgorithm>(enumVal);
    }
  }

  return result;
}

bool Configuration::updatePmCorrection(JSONVar &json) {
  if (!json.hasOwnProperty("corrections")) {
    logInfo("corrections not found");
    return false;
  }

  JSONVar corrections = json["corrections"];
  if (!corrections.hasOwnProperty("pm02")) {
    logWarning("pm02 not found");
    return false;
  }

  JSONVar pm02 = corrections["pm02"];
  if (!pm02.hasOwnProperty("correctionAlgorithm")) {
    logWarning("pm02 correctionAlgorithm not found");
    return false;
  }

  // Check algorithm
  String algorithm = pm02["correctionAlgorithm"];
  PMCorrectionAlgorithm algo = matchPmAlgorithm(algorithm);
  if (algo == COR_ALGO_PM_UNKNOWN) {
    logWarning("Unknown algorithm");
    return false;
  }
  logInfo("Correction algorithm: " + algorithm);

  // If algo is None or EPA_2021, no need to check slr
  // But first check if pmCorrection different from algo
  if (algo == COR_ALGO_PM_NONE || algo == COR_ALGO_PM_EPA_2021) {
    if (pmCorrection.algorithm != algo) {
      // Deep copy corrections from root to jconfig, so it will be saved later
      jconfig[jprop_corrections]["pm02"]["correctionAlgorithm"] = algorithm;
      jconfig[jprop_corrections]["pm02"]["slr"] = JSON.parse("{}"); // Clear slr
      // Update pmCorrection with new values
      pmCorrection.algorithm = algo;
      pmCorrection.changed = true;
      logInfo("PM2.5 correction updated");
      return true;
    }

    return false;
  }

  // Check if pm02 has slr object
  if (!pm02.hasOwnProperty("slr")) {
    logWarning("slr not found");
    return false;
  }

  JSONVar slr = pm02["slr"];

  // Validate required slr properties exist
  if (!slr.hasOwnProperty("intercept") || !slr.hasOwnProperty("scalingFactor") ||
      !slr.hasOwnProperty("useEpa2021")) {
    logWarning("Missing required slr properties");
    return false;
  }

  // arduino_json doesn't support float type, need to cast to double first
  float intercept = (float)((double)slr["intercept"]);
  float scalingFactor = (float)((double)slr["scalingFactor"]);

  // Compare with current pmCorrection
  if (pmCorrection.algorithm == algo && pmCorrection.intercept == intercept &&
      pmCorrection.scalingFactor == scalingFactor &&
      pmCorrection.useEPA == (bool)slr["useEpa2021"]) {
    return false; // No changes needed
  }

  // Deep copy corrections from root to jconfig, so it will be saved later
  jconfig[jprop_corrections] = corrections;

  // Update pmCorrection with new values
  pmCorrection.algorithm = algo;
  pmCorrection.intercept = intercept;
  pmCorrection.scalingFactor = scalingFactor;
  pmCorrection.useEPA = (bool)slr["useEpa2021"];
  pmCorrection.changed = true;

  // Correction values were updated
  logInfo("PM2.5 correction updated");
  return true;
}

bool Configuration::updateTempHumCorrection(JSONVar &json, TempHumCorrection &target,
                                            const char *correctionName) {
  if (!json.hasOwnProperty(jprop_corrections)) {
    return false;
  }

  JSONVar corrections = json[jprop_corrections];
  if (!corrections.hasOwnProperty(correctionName)) {
    logInfo(String(correctionName) + " correction field not found on configuration");
    return false;
  }

  JSONVar correctionTarget = corrections[correctionName];
  if (!correctionTarget.hasOwnProperty("correctionAlgorithm")) {
    Serial.println("correctionAlgorithm not found");
    return false;
  }

  String algorithm = correctionTarget["correctionAlgorithm"];
  TempHumCorrectionAlgorithm algo = matchTempHumAlgorithm(algorithm);
  if (algo == COR_ALGO_TEMP_HUM_UNKNOWN) {
    logInfo("Uknown temp/hum algorithm");
    return false;
  }
  logInfo(String(correctionName) + " correction algorithm: " + algorithm);

  // If algo is None or Standard, then no need to check slr
  // But first check if target correction different from algo
  if (algo == COR_ALGO_TEMP_HUM_NONE || algo == COR_ALGO_TEMP_HUM_AG_PMS5003T_2024) {
    if (target.algorithm != algo) {
      // Deep copy corrections from root to jconfig, so it will be saved later
      jconfig[jprop_corrections][correctionName]["correctionAlgorithm"] = algorithm;
      jconfig[jprop_corrections][correctionName]["slr"] = JSON.parse("{}"); // Clear slr
      // Update pmCorrection with new values
      target.algorithm = algo;
      target.changed = true;
      logInfo(String(correctionName) + " correction updated");
      return true;
    }

    return false;
  }

  // Check if correction.target (atmp or rhum) has slr object
  if (!correctionTarget.hasOwnProperty("slr")) {
    logWarning(String(correctionName) + " slr not found");
    return false;
  }

  JSONVar slr = correctionTarget["slr"];

  // Validate required slr properties exist
  if (!slr.hasOwnProperty("intercept") || !slr.hasOwnProperty("scalingFactor")) {
    Serial.println("Missing required slr properties");
    return false;
  }

  // arduino_json doesn't support float type, need to cast to double first
  float intercept = (float)((double)slr["intercept"]);
  float scalingFactor = (float)((double)slr["scalingFactor"]);

  // Compare with current target correciont
  if (target.algorithm == algo && target.intercept == intercept &&
      target.scalingFactor == scalingFactor) {
    return false; // No changes needed
  }

  // Deep copy corrections from root to jconfig, so it will be saved later
  jconfig[jprop_corrections] = corrections;

  // Update target with new values
  target.algorithm = algo;
  target.intercept = intercept;
  target.scalingFactor = scalingFactor;
  target.changed = true;

  // Correction values were updated
  logInfo(String(correctionName) + " correction updated");
  return true;
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
#else
  File file = SPIFFS.open(CONFIG_FILE_NAME);
  if (file && !file.isDirectory()) {
    logInfo("Reading file...");
    if(file.readBytes(buf, file.size()) != file.size()) {
      logError("Reading file: failed - size not match");
    } else {
      logInfo("Reading file: success");
    }
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

  jconfig[jprop_country] = jprop_country_default;
  jconfig[jprop_mqttBrokerUrl] = jprop_mqttBrokerUrl_default;
  jconfig[jprop_httpDomain] = jprop_httpDomain_default;
  jconfig[jprop_configurationControl] = jprop_configurationControl_default;
  jconfig[jprop_pmStandard] = jprop_pmStandard_default;
  jconfig[jprop_temperatureUnit] = jprop_temperatureUnit_default;
  jconfig[jprop_disableCloudConnection] = jprop_disableCloudConnection_default;
  jconfig[jprop_postDataToAirGradient] = jprop_postDataToAirGradient_default;
  if (ag->isOne()) {
    jconfig[jprop_ledBarBrightness] = jprop_ledBarBrightness_default;
  }
  if (ag->isOne() || ag->isPro3_3() || ag->isPro4_2() || ag->isBasic()) {
    jconfig[jprop_displayBrightness] = jprop_displayBrightness_default;
  }
  if (ag->isOne()) {
    jconfig[jprop_ledBarMode] = jprop_ledBarMode_default;
  }
  jconfig[jprop_tvocLearningOffset] = jprop_tvocLearningOffset_default;
  jconfig[jprop_noxLearningOffset] = jprop_noxLearningOffset_default;
  jconfig[jprop_abcDays] = jprop_abcDays_default;
  jconfig[jprop_model] = jprop_model_default;
  jconfig[jprop_offlineMode] = jprop_offlineMode_default;
  jconfig[jprop_monitorDisplayCompensatedValues] = jprop_monitorDisplayCompensatedValues_default;

  // PM2.5 default correction
  pmCorrection.algorithm = COR_ALGO_PM_NONE;
  pmCorrection.changed = false;
  pmCorrection.intercept = 0;
  pmCorrection.scalingFactor = 1;
  pmCorrection.useEPA = false;

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

void Configuration::setConfigurationUpdatedCallback(ConfigurationUpdatedCallback_t callback) {
  _callback = callback;
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
  logInfo("Parsing configuration: " + data);

  JSONVar root = JSON.parse(data);
  failedMessage = "";
  if (root == undefined || JSONVar::typeof_(root) != "object") {
    logError("Parse configuration failed, JSON invalid (" + JSONVar::typeof_(root) + ")");
    failedMessage = "JSON invalid";
    return false;
  }
  logInfo("Parse configuration success");

  /** Is configuration changed */
  bool changed = false;

  /** Get ConfigurationControl */
  String lastCtrl = jconfig[jprop_configurationControl];
  const char *msg = "Monitor set to accept only configuration from the "
                    "cloud. Use property configurationControl to change.";

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
        if (ctrl != lastCtrl) {
          jconfig[jprop_configurationControl] = ctrl;
          saveConfig();
          configLogInfo(String(jprop_configurationControl), lastCtrl,
                        jconfig[jprop_configurationControl]);
        }

        /** Return failed if new "configurationControl" new and old is "cloud" */
        if (ctrl == String(CONFIGURATION_CONTROL_NAME [ConfigurationControl::ConfigurationControlCloud])) {
          if(ctrl != lastCtrl) {
            return true;
          } else {
            failedMessage = String(msg);
            return false;
          }
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

    /** Ignore all configuration value if 'configurationControl' is 'cloud' */
    if (jconfig[jprop_configurationControl] ==
        String(CONFIGURATION_CONTROL_NAME
                   [ConfigurationControl::ConfigurationControlCloud])) {
      failedMessage = String(msg);
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

  _ledBarModeChanged = false;
  if (JSON.typeof_(root[jprop_ledBarMode]) == "string") {
    String mode = root[jprop_ledBarMode];
    if (mode == getLedBarModeName(LedBarMode::LedBarModeCO2) ||
        mode == getLedBarModeName(LedBarMode::LedBarModeOff) ||
        mode == getLedBarModeName(LedBarMode::LedBarModePm)) {
      String oldMode = jconfig[jprop_ledBarMode];
      if (mode != oldMode) {
        jconfig[jprop_ledBarMode] = mode;
        _ledBarModeChanged = true;
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
    if (broker.length() <= 255) {
      if (broker != oldBroker) {
        changed = true;
        configLogInfo(String(jprop_mqttBrokerUrl), oldBroker, broker);
        jconfig[jprop_mqttBrokerUrl] = broker;
      }
    } else {
      failedMessage = "\"mqttBrokerUrl\" length should less than 255 character";
      jsonInvalid();
      return false;
    }
  }
  else if (JSON.typeof_(root[jprop_mqttBrokerUrl]) == "null" and !isLocal) {
    // So if its not available on the json and json comes from aigradient server
    // then set its value to default (empty)
    jconfig[jprop_mqttBrokerUrl] = jprop_mqttBrokerUrl_default;
  }
  else {
    if (jsonTypeInvalid(root[jprop_mqttBrokerUrl], "string")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_mqttBrokerUrl), "string");
      jsonInvalid();
      return false;
    }
  }

  if (isLocal) {
    if (JSON.typeof_(root[jprop_httpDomain]) == "string") {
      String httpDomain = root[jprop_httpDomain];
      String oldHttpDomain = jconfig[jprop_httpDomain];
      if (httpDomain.length() <= 255) {
        if (httpDomain != oldHttpDomain) {
          changed = true;
          configLogInfo(String(jprop_httpDomain), oldHttpDomain, httpDomain);
          jconfig[jprop_httpDomain] = httpDomain;
        }
      } else {
        failedMessage = "\"httpDomain\" length should less than 255 character";
        jsonInvalid();
        return false;
      }
    }
    else {
      if (jsonTypeInvalid(root[jprop_httpDomain], "string")) {
        failedMessage =
            jsonTypeInvalidMessage(String(jprop_httpDomain), "string");
        jsonInvalid();
        return false;
      }
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
      failedMessage = jsonValueInvalidMessage(String(jprop_temperatureUnit), unit);
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

  ledBarBrightnessChanged = false;
  if (JSON.typeof_(root[jprop_ledBarBrightness]) == "number") {
    int value = root[jprop_ledBarBrightness];
    int oldValue = jconfig[jprop_ledBarBrightness];
    if (value >= 0 && value <= 100) {
      if (value != oldValue) {
        changed = true;
        configLogInfo(String(jprop_ledBarBrightness), String(oldValue),
                      String(value));
        ledBarBrightnessChanged = true;
        jconfig[jprop_ledBarBrightness] = value;
      }
    } else {
      failedMessage = jsonValueInvalidMessage(String(jprop_ledBarBrightness),
                                              String(value));
      jsonInvalid();
      return false;
    }
  } else {
    if (jsonTypeInvalid(root[jprop_ledBarBrightness], "number")) {
      failedMessage =
          jsonTypeInvalidMessage(String(jprop_ledBarBrightness), "number");
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

  if (JSON.typeof_(root[jprop_monitorDisplayCompensatedValues]) == "boolean") {
    bool value = root[jprop_monitorDisplayCompensatedValues];
    bool oldValue = jconfig[jprop_monitorDisplayCompensatedValues];
    if (value != oldValue) {
      changed = true;
      jconfig[jprop_monitorDisplayCompensatedValues] = value;

      configLogInfo(String(jprop_monitorDisplayCompensatedValues),
                    String(oldValue ? "true" : "false"),
                    String(value ? "true" : "false"));
    }
  } else {
    if (jsonTypeInvalid(root[jprop_monitorDisplayCompensatedValues],
                        "boolean")) {
      failedMessage = jsonTypeInvalidMessage(
          String(jprop_monitorDisplayCompensatedValues), "boolean");
      jsonInvalid();
      return false;
    }
  }

  if (ag->getBoardType() == ONE_INDOOR ||
      ag->getBoardType() == OPEN_AIR_OUTDOOR) {
    if (JSON.typeof_(root["targetFirmware"]) == "string") {
      String newVer = root["targetFirmware"];
      String curVer = String(GIT_VERSION);
      if (curVer != newVer) {
        logInfo("Detected new firmware version: " + newVer);
        otaNewFirmwareVersion = newVer;
        updated = true;
      } else {
        otaNewFirmwareVersion = String("");
      }
    }
  }

  // PM2.5 Corrections
  if (updatePmCorrection(root)) {
    changed = true;
  }

  // Temperature correction
  if (updateTempHumCorrection(root, tempCorrection, jprop_atmp)) {
    changed = true;
  }

  // Relative humidity correction
  if (updateTempHumCorrection(root, rhumCorrection, jprop_rhum)) {
    changed = true;
  }

  if (ledBarTestRequested || co2CalibrationRequested) {
    commandRequested = true;
    updated = true;
  }

  if (changed) {
    updated = true;
    saveConfig();
    printConfig();
    _callback();
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
 * @brief Get current configuration value as JSON string
 *
 * @param fwMode Firmware mode value
 * @return String
 */
String Configuration::toString(AgFirmwareMode fwMode) {
  String model = jconfig[jprop_model];
  jconfig[jprop_model] = AgFirmwareModeName(fwMode);
  String value = toString();
  jconfig[jprop_model] = model;
  return value;
}

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
 * @brief Get MQTT uri
 *
 * @return String
 */
String Configuration::getMqttBrokerUri(void) {
  String broker = jconfig[jprop_mqttBrokerUrl];
  return broker;
}

/**
 * @brief Get HTTP domain for post measures and get configuration
 *
 * @return String http domain, might be empty string
 */
String Configuration::getHttpDomain(void) {
  String httpDomain = jconfig[jprop_httpDomain];
  return httpDomain;
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
  bool updated = this->updated;
  this->updated = false;
  return updated;
}

bool Configuration::isCommandRequested(void) {
  bool oldState = this->commandRequested;
  this->commandRequested = false;
  return oldState;
}

String Configuration::jsonTypeInvalidMessage(String name, String type) {
  return "'" + name + "' type is invalid, expecting '" + type + "'";
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
  bool isConfigFieldInvalid = false;

  /** Validate country */
  if (JSON.typeof_(jconfig[jprop_country]) != "string") {
    isConfigFieldInvalid = true;
  } else {
    String country = jconfig[jprop_country];
    if (country.length() != 2) {
      isConfigFieldInvalid = true;
    } else {
      isConfigFieldInvalid = false;
    }
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_country] = jprop_country_default;
    changed = true;
    logInfo("toConfig: country changed");
  }

  /** validate: PM standard */
  if (JSON.typeof_(jconfig[jprop_pmStandard]) != "string") {
    isConfigFieldInvalid = true;
  } else {
    String standard = jconfig[jprop_pmStandard];
    if (standard != getPMStandardString(true) &&
        standard != getPMStandardString(false)) {
      isConfigFieldInvalid = true;
    } else {
      isConfigFieldInvalid = false;
    }
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_pmStandard] = jprop_pmStandard_default;
    changed = true;
    logInfo("toConfig: pmStandard changed");
  }

  /** validate led bar mode */
  if (JSON.typeof_(jconfig[jprop_ledBarMode]) != "string") {
    isConfigFieldInvalid = true;
  } else {
    String mode = jconfig[jprop_ledBarMode];
    if (mode != getLedBarModeName(LedBarMode::LedBarModeCO2) &&
        mode != getLedBarModeName(LedBarMode::LedBarModeOff) &&
        mode != getLedBarModeName(LedBarMode::LedBarModePm)) {
      isConfigFieldInvalid = true;
    } else {
      isConfigFieldInvalid = false;
    }
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_ledBarMode] = jprop_ledBarMode_default;
    changed = true;
    logInfo("toConfig: ledBarMode changed");
  }

  /** validate abcday */
  if (JSON.typeof_(jconfig[jprop_abcDays]) != "number") {
    isConfigFieldInvalid = true;
  } else {
    isConfigFieldInvalid = false;
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_abcDays] = jprop_abcDays_default;
    changed = true;
    logInfo("toConfig: abcDays changed");
  }

  /** validate tvoc learning offset */
  if (JSON.typeof_(jconfig[jprop_tvocLearningOffset]) != "number") {
    isConfigFieldInvalid = true;
  } else {
    int value = jconfig[jprop_tvocLearningOffset];
    if (value < 0) {
      isConfigFieldInvalid = true;
    } else {
      isConfigFieldInvalid = false;
    }
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_tvocLearningOffset] = jprop_tvocLearningOffset_default;
    changed = true;
    logInfo("toConfig: tvocLearningOffset changed");
  }

  /** validate nox learning offset */
  if (JSON.typeof_(jconfig[jprop_noxLearningOffset]) != "number") {
    isConfigFieldInvalid = true;
  } else {
    int value = jconfig[jprop_noxLearningOffset];
    if (value < 0) {
      isConfigFieldInvalid = true;
    } else {
      isConfigFieldInvalid = false;
    }
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_noxLearningOffset] = jprop_noxLearningOffset_default;
    changed = true;
    logInfo("toConfig: noxLearningOffset changed");
  }

  /** validate mqtt broker */
  if (JSON.typeof_(jconfig[jprop_mqttBrokerUrl]) != "string") {
    isConfigFieldInvalid = true;
  } else {
    isConfigFieldInvalid = false;
  }
  if (isConfigFieldInvalid) {
    changed = true;
    jconfig[jprop_mqttBrokerUrl] = jprop_mqttBrokerUrl_default;
    logInfo("toConfig: mqttBroker changed");
  }

  /** validate http domain */
  if (JSON.typeof_(jconfig[jprop_httpDomain]) != "string") {
    isConfigFieldInvalid = true;
  } else {
    isConfigFieldInvalid = false;
  }
  if (isConfigFieldInvalid) {
    changed = true;
    jconfig[jprop_httpDomain] = jprop_httpDomain_default;
    logInfo("toConfig: httpDomain changed");
  }

  /** Validate temperature unit */
  if (JSON.typeof_(jconfig[jprop_temperatureUnit]) != "string") {
    isConfigFieldInvalid = true;
  } else {
    String unit = jconfig[jprop_temperatureUnit];
    if (unit != "c" && unit != "f") {
      isConfigFieldInvalid = true;
    } else {
      isConfigFieldInvalid = false;
    }
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_temperatureUnit] = jprop_temperatureUnit_default;
    changed = true;
    logInfo("toConfig: temperatureUnit changed");
  }

  /** validate disableCloudConnection configuration */
  if (JSON.typeof_(jconfig[jprop_disableCloudConnection]) != "boolean") {
    isConfigFieldInvalid = true;
  } else {
    isConfigFieldInvalid = false;
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_disableCloudConnection] = jprop_disableCloudConnection_default;
    changed = true;
    logInfo("toConfig: disableCloudConnection changed");
  }

  /** validate configuration control */
  if (JSON.typeof_(jprop_configurationControl) != "string") {
    isConfigFieldInvalid = true;
  } else {
    String ctrl = jconfig[jprop_configurationControl];
    if (ctrl != String(CONFIGURATION_CONTROL_NAME
                           [ConfigurationControl::ConfigurationControlBoth]) &&
        ctrl != String(CONFIGURATION_CONTROL_NAME
                           [ConfigurationControl::ConfigurationControlLocal]) &&
        ctrl != String(CONFIGURATION_CONTROL_NAME
                           [ConfigurationControl::ConfigurationControlCloud])) {
      isConfigFieldInvalid = true;
    } else {
      isConfigFieldInvalid = false;
    }
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_configurationControl] =jprop_configurationControl_default;
    changed = true;
    logInfo("toConfig: configurationControl changed");
  }

  /** Validate post to airgradient cloud */
  if (JSON.typeof_(jconfig[jprop_postDataToAirGradient]) != "boolean") {
    isConfigFieldInvalid = true;
  } else {
    isConfigFieldInvalid = false;
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_postDataToAirGradient] = jprop_postDataToAirGradient_default;
    changed = true;
    logInfo("toConfig: postToAirGradient changed");
  }

  /** validate led bar brightness */
  if (JSON.typeof_(jconfig[jprop_ledBarBrightness]) != "number") {
    isConfigFieldInvalid = true;
  } else {
    int value = jconfig[jprop_ledBarBrightness];
    if (value < 0 || value > 100) {
      isConfigFieldInvalid = true;
    } else {
      isConfigFieldInvalid = false;
    }
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_ledBarBrightness] = jprop_ledBarBrightness_default;
    changed = true;
    logInfo("toConfig: ledBarBrightness changed");
  }

  /** Validate display brightness */
  if (JSON.typeof_(jconfig[jprop_displayBrightness]) != "number") {
    isConfigFieldInvalid = true;
  } else {
    int value = jconfig[jprop_displayBrightness];
    if (value < 0 || value > 100) {
      isConfigFieldInvalid = true;
    } else {
      isConfigFieldInvalid = false;
    }
  }
  if (isConfigFieldInvalid) {
    jconfig[jprop_displayBrightness] = jprop_displayBrightness_default;
    changed = true;
    logInfo("toConfig: displayBrightness changed");
  }

  if (JSON.typeof_(jconfig[jprop_offlineMode]) != "boolean") {
    changed = true;
    jconfig[jprop_offlineMode] = jprop_offlineMode_default;
  }

  /** Validate monitorDisplayCompensatedValues */
  if (JSON.typeof_(jconfig[jprop_monitorDisplayCompensatedValues]) !=
      "boolean") {
    changed = true;
    jconfig[jprop_monitorDisplayCompensatedValues] =
        jprop_monitorDisplayCompensatedValues_default;
  }

  // PM2.5 correction
  /// Set default first before parsing local config
  pmCorrection.algorithm = COR_ALGO_PM_NONE;
  pmCorrection.intercept = 0;
  pmCorrection.scalingFactor = 0;
  pmCorrection.useEPA = false;
  /// Load correction from saved config
  updatePmCorrection(jconfig);

  // Temperature correction
  /// Set default first before parsing local config
  tempCorrection.algorithm = COR_ALGO_TEMP_HUM_NONE;
  tempCorrection.intercept = 0;
  tempCorrection.scalingFactor = 0;
  /// Load correction from saved config
  updateTempHumCorrection(jconfig, tempCorrection, jprop_atmp);

  // Relative humidity correction
  /// Set default first before parsing local config
  rhumCorrection.algorithm = COR_ALGO_TEMP_HUM_NONE;
  rhumCorrection.intercept = 0;
  rhumCorrection.scalingFactor = 0;
  /// Load correction from saved config
  updateTempHumCorrection(jconfig, rhumCorrection, jprop_rhum);

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
  int value = jconfig[jprop_ledBarBrightness];
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
  return (offline || _offlineMode);
}

void Configuration::setOfflineMode(bool offline) {
  logInfo("Set offline mode: " + String(offline ? "True" : "False"));
  jconfig[jprop_offlineMode] = offline;
  saveConfig();
}

void Configuration::setOfflineModeWithoutSave(bool offline) {
  _offlineMode = offline;
}

bool Configuration::isCloudConnectionDisabled(void) {
  bool disabled = jconfig[jprop_disableCloudConnection];
  return disabled;
}

void Configuration::setDisableCloudConnection(bool disable) {
  logInfo("Set DisableCloudConnection to " + String(disable ? "True" : "False"));
  jconfig[jprop_disableCloudConnection] = disable;
  saveConfig();
}

bool Configuration::isLedBarModeChanged(void) {
  bool changed = _ledBarModeChanged;
  _ledBarModeChanged = false;
  return changed;
}

bool Configuration::isMonitorDisplayCompensatedValues(void) {
  return jconfig[jprop_monitorDisplayCompensatedValues];
}

bool Configuration::isDisplayBrightnessChanged(void) {
  bool changed = displayBrightnessChanged;
  displayBrightnessChanged = false;
  return changed;
}

String Configuration::newFirmwareVersion(void) {
  String newFw = otaNewFirmwareVersion;
  otaNewFirmwareVersion = String("");
  return newFw;
}

bool Configuration::isPMCorrectionChanged(void) {
  bool changed = pmCorrection.changed;
  pmCorrection.changed = false;
  return changed;
}

/**
 * @brief Check if PM correction is enabled
 *
 * @return true if PM correction algorithm is not None, otherwise false
 */
bool Configuration::isPMCorrectionEnabled(void) {
  PMCorrection pmCorrection = getPMCorrection();
  if (pmCorrection.algorithm == COR_ALGO_PM_NONE ||
      pmCorrection.algorithm == COR_ALGO_PM_UNKNOWN) {
    return false;
  }

  return true;
}

Configuration::PMCorrection Configuration::getPMCorrection(void) { return pmCorrection; }

Configuration::TempHumCorrection Configuration::getTempCorrection(void) { return tempCorrection; }

Configuration::TempHumCorrection Configuration::getHumCorrection(void) { return rhumCorrection; }
