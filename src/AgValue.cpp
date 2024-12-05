#include "AgValue.h"
#include "AgConfigure.h"
#include "AirGradient.h"
#include "App/AppDef.h"

#define json_prop_pmFirmware     "firmware"
#define json_prop_pm01Ae "pm01"
#define json_prop_pm25Ae "pm02"
#define json_prop_pm10Ae "pm10"
#define json_prop_pm01Sp "pm01Standard"
#define json_prop_pm25Sp "pm02Standard"
#define json_prop_pm10Sp "pm10Standard"
#define json_prop_pm25Compensated "pm02Compensated"
#define json_prop_pm03Count "pm003Count"
#define json_prop_pm05Count "pm005Count"
#define json_prop_pm1Count "pm01Count"
#define json_prop_pm25Count "pm02Count"
#define json_prop_pm5Count "pm50Count"
#define json_prop_pm10Count "pm10Count"
#define json_prop_temp "atmp"
#define json_prop_tempCompensated "atmpCompensated"
#define json_prop_rhum "rhum"
#define json_prop_rhumCompensated "rhumCompensated"
#define json_prop_tvoc "tvocIndex"
#define json_prop_tvocRaw "tvocRaw"
#define json_prop_nox "noxIndex"
#define json_prop_noxRaw "noxRaw"
#define json_prop_co2 "rco2"

Measurements::Measurements() {
#ifndef ESP8266
  _resetReason = (int)ESP_RST_UNKNOWN;
#endif
}

void Measurements::maxPeriod(MeasurementType type, int max) {
  switch (type) {
  case Temperature:
    _temperature[0].update.max = max;
    _temperature[1].update.max = max;
    break;
  case Humidity:
    _humidity[0].update.max = max;
    _humidity[1].update.max = max;
    break;
  case CO2:
    _co2.update.max = max;
    break;
  case TVOC:
    _tvoc.update.max = max;
    break;
  case TVOCRaw:
    _tvoc_raw.update.max = max;
    break;
  case NOx:
    _nox.update.max = max;
    break;
  case NOxRaw:
    _nox_raw.update.max = max;
    break;
  case PM25:
    _pm_25[0].update.max = max;
    _pm_25[1].update.max = max;
    break;
  case PM01:
    _pm_01[0].update.max = max;
    _pm_01[1].update.max = max;
    break;
  case PM10:
    _pm_10[0].update.max = max;
    _pm_10[1].update.max = max;
    break;
  case PM01_SP:
    _pm_01_sp[0].update.max = max;
    _pm_01_sp[1].update.max = max;
    break;
  case PM25_SP:
    _pm_25_sp[0].update.max = max;
    _pm_25_sp[1].update.max = max;
    break;
  case PM10_SP:
    _pm_10_sp[0].update.max = max;
    _pm_10_sp[1].update.max = max;
    break;
  case PM03_PC:
    _pm_03_pc[0].update.max = max;
    _pm_03_pc[1].update.max = max;
    break;
  case PM05_PC:
    _pm_05_pc[0].update.max = max;
    _pm_05_pc[1].update.max = max;
    break;
  case PM01_PC:
    _pm_01_pc[0].update.max = max;
    _pm_01_pc[1].update.max = max;
    break;
  case PM25_PC:
    _pm_25_pc[0].update.max = max;
    _pm_25_pc[1].update.max = max;
    break;
  case PM5_PC:
    _pm_5_pc[0].update.max = max;
    _pm_5_pc[1].update.max = max;
    break;
  case PM10_PC:
    _pm_10_pc[0].update.max = max;
    _pm_10_pc[1].update.max = max;
    break;
  };
}

bool Measurements::update(MeasurementType type, int val, int ch) {
  // Sanity check to validate channel, assert if invalid
  validateChannel(ch);

  // Follow array indexing just for get address of the value type
  ch = ch - 1;

  // Define data point source
  IntegerValue *temporary = nullptr;
  // Act as reference invalid value respective to target measurements
  int invalidValue = 0;
  switch (type) {
  case CO2:
    temporary = &_co2;
    invalidValue = utils::getInvalidCO2();
    break;
  case TVOC:
    temporary = &_tvoc;
    invalidValue = utils::getInvalidVOC();
    break;
  case TVOCRaw:
    temporary = &_tvoc_raw;
    invalidValue = utils::getInvalidVOC();
    break;
  case NOx:
    temporary = &_nox;
    invalidValue = utils::getInvalidNOx();
    break;
  case NOxRaw:
    temporary = &_nox_raw;
    invalidValue = utils::getInvalidNOx();
    break;
  case PM25:
    temporary = &_pm_25[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM01:
    temporary = &_pm_01[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM10:
    temporary = &_pm_10[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM01_SP:
    temporary = &_pm_01_sp[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM25_SP:
    temporary = &_pm_25_sp[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM10_SP:
    temporary = &_pm_10_sp[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM03_PC:
    temporary = &_pm_03_pc[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM05_PC:
    temporary = &_pm_05_pc[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM01_PC:
    temporary = &_pm_01_pc[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM25_PC:
    temporary = &_pm_25_pc[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM5_PC:
    temporary = &_pm_5_pc[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM10_PC:
    temporary = &_pm_10_pc[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  default:
    break;
  };

  // Sanity check if measurement type is defined for integer data type or not
  if (temporary == nullptr) {
    Serial.printf("%s is not defined for integer data type\n", measurementTypeStr(type).c_str());
    // TODO: Just assert?
    return false;
  }

  // Restore channel value for debugging purpose
  ch = ch + 1;

  if (val == invalidValue) {
    temporary->update.invalidCounter++;
    if (temporary->update.invalidCounter >= temporary->update.max) {
      Serial.printf("%s{%d} invalid value update counter reached (%dx)! Setting its average value "
                    "to invalid!\n",
                    measurementTypeStr(type).c_str(), ch, temporary->update.max);
      temporary->update.avg = invalidValue;
      return false;
    }

    // Still consider updating value to valid
    return true;
  }

  // Reset invalid counter when update new valid value
  temporary->update.invalidCounter = 0;

  // Add new value to the end of the list
  temporary->listValues.push_back(val);
  // Sum the new value
  temporary->sumValues = temporary->sumValues + val;
  // Remove the oldest value on the list when the list exceed max elements
  if (temporary->listValues.size() > temporary->update.max) {
    auto it = temporary->listValues.begin();
    temporary->sumValues = temporary->sumValues - *it; // subtract the oldest value from sum
    temporary->listValues.erase(it);                   // And remove it from the list
  }

  // Calculate average based on how many elements on the list
  temporary->update.avg = temporary->sumValues / (float)temporary->listValues.size();
  if (_debug) {
    Serial.printf("%s{%d}: %.2f\n", measurementTypeStr(type).c_str(), ch, temporary->update.avg);
  }

  return true;
}

bool Measurements::update(MeasurementType type, float val, int ch) {
  // Sanity check to validate channel, assert if invalid
  validateChannel(ch);

  // Follow array indexing just for get address of the value type
  ch = ch - 1;

  // Define data point source
  FloatValue *temporary = nullptr;
  // Act as reference invalid value respective to target measurements
  float invalidValue = 0;
  switch (type) {
  case Temperature:
    temporary = &_temperature[ch];
    invalidValue = utils::getInvalidTemperature();
    break;
  case Humidity:
    temporary = &_humidity[ch];
    invalidValue = utils::getInvalidHumidity();
    break;
  default:
    break;
  }

  // Sanity check if measurement type is defined for float data type or not
  if (temporary == nullptr) {
    Serial.printf("%s is not defined for float data type\n", measurementTypeStr(type).c_str());
    // TODO: Just assert?
    return false;
  }

  // Restore channel value for debugging purpose
  ch = ch + 1;

  if (val == invalidValue) {
    temporary->update.invalidCounter++;
    if (temporary->update.invalidCounter >= temporary->update.max) {
      Serial.printf("%s{%d} invalid value update counter reached (%dx)! Setting its average value "
                    "to invalid!\n",
                    measurementTypeStr(type).c_str(), ch, temporary->update.max);
      temporary->update.avg = invalidValue;
      return false;
    }

    // Still consider updating value to valid
    return true;
  }

  // Reset invalid counter when update new valid value
  temporary->update.invalidCounter = 0;

  // Add new value to the end of the list
  temporary->listValues.push_back(val);
  // Sum the new value
  temporary->sumValues = temporary->sumValues + val;
  // Remove the oldest value on the list when the list exceed max elements
  if (temporary->listValues.size() > temporary->update.max) {
    auto it = temporary->listValues.begin();
    temporary->sumValues = temporary->sumValues - *it; // subtract the oldest value from sum
    temporary->listValues.erase(it);                   // And remove it from the list
  }

  // Calculate average based on how many elements on the list
  temporary->update.avg = temporary->sumValues / (float)temporary->listValues.size();
  if (_debug) {
    Serial.printf("%s{%d}: %.2f\n", measurementTypeStr(type).c_str(), ch, temporary->update.avg);
  }

  return true;
}

int Measurements::get(MeasurementType type, int ch) {
  // Sanity check to validate channel, assert if invalid
  validateChannel(ch);

  // Follow array indexing just for get address of the value type
  ch = ch - 1;

  // Define data point source
  IntegerValue *temporary = nullptr;
  switch (type) {
  case CO2:
    temporary = &_co2;
    break;
  case TVOC:
    temporary = &_tvoc;
    break;
  case TVOCRaw:
    temporary = &_tvoc_raw;
    break;
  case NOx:
    temporary = &_nox;
    break;
  case NOxRaw:
    temporary = &_nox_raw;
    break;
  case PM25:
    temporary = &_pm_25[ch];
    break;
  case PM01:
    temporary = &_pm_01[ch];
    break;
  case PM10:
    temporary = &_pm_10[ch];
    break;
  case PM03_PC:
    temporary = &_pm_03_pc[ch];
    break;
  default:
    break;
  };

  // Sanity check if measurement type is defined for integer data type or not
  if (temporary == nullptr) {
    Serial.printf("%s is not defined for integer data type\n", measurementTypeStr(type).c_str());
    // TODO: Just assert?
    return false;
  }

  if (temporary->listValues.empty()) {
    // Values still empty, return 0
    return 0;
  }

  return temporary->listValues.back();
}

float Measurements::getFloat(MeasurementType type, int ch) {
  // Sanity check to validate channel, assert if invalid
  validateChannel(ch);

  // Follow array indexing just for get address of the value type
  ch = ch - 1;

  // Define data point source
  FloatValue *temporary = nullptr;
  switch (type) {
  case Temperature:
    temporary = &_temperature[ch];
    break;
  case Humidity:
    temporary = &_humidity[ch];
    break;
  default:
    break;
  }

  // Sanity check if measurement type is defined for float data type or not
  if (temporary == nullptr) {
    Serial.printf("%s is not defined for float data type\n", measurementTypeStr(type).c_str());
    // TODO: Just assert?
    return false;
  }

  if (temporary->listValues.empty()) {
    // Values still empty, return 0
    return 0;
  }

  return temporary->listValues.back();
}

float Measurements::getAverage(MeasurementType type, int ch) {
  // Sanity check to validate channel, assert if invalid
  validateChannel(ch);

  // Follow array indexing just for get address of the value type
  ch = ch - 1;

  // Define data point source. Data type doesn't matter because only to get the average value
  FloatValue *temporary = nullptr;
  Update update;
  float measurementAverage;
  switch (type) {
  case CO2:
    measurementAverage = _co2.update.avg;
    break;
  case TVOC:
    measurementAverage = _tvoc.update.avg;
    break;
  case NOx:
    measurementAverage = _nox.update.avg;
    break;
  case PM25:
    measurementAverage = _pm_25[ch].update.avg;
    break;
  case Temperature:
    measurementAverage = _temperature[ch].update.avg;
    break;
  case Humidity:
    measurementAverage = _humidity[ch].update.avg;
    break;
  default:
    // Invalidate, measurements type not handled
    measurementAverage = -1000;
    break;
  };

  // Sanity check if measurement type is not defined 
  if (measurementAverage == -1000) {
    Serial.printf("ERROR! %s is not defined on get average value function\n", measurementTypeStr(type).c_str());
    delay(1000);
    assert(0);
  }

  return measurementAverage; 
}

String Measurements::pms5003FirmwareVersion(int fwCode) {
  return pms5003FirmwareVersionBase("PMS5003x", fwCode);
}

String Measurements::pms5003TFirmwareVersion(int fwCode) {
  return pms5003FirmwareVersionBase("PMS5003x", fwCode);
}

String Measurements::pms5003FirmwareVersionBase(String prefix, int fwCode) {
  return prefix + String("-") + String(fwCode);
}

String Measurements::measurementTypeStr(MeasurementType type) {
  String str;
  switch (type) {
  case Temperature:
    str = "Temperature";
    break;
  case Humidity:
    str = "Humidity";
    break;
  case CO2:
    str = "CO2";
    break;
  case TVOC:
    str = "TVOC";
    break;
  case TVOCRaw:
    str = "TVOCRaw";
    break;
  case NOx:
    str = "NOx";
    break;
  case NOxRaw:
    str = "NOxRaw";
    break;
  case PM25:
    str = "PM25_AE";
    break;
  case PM01:
    str = "PM1_AE";
    break;
  case PM10:
    str = "PM10_AE";
    break;
  case PM25_SP:
    str = "PM25_SP";
    break;
  case PM01_SP:
    str = "PM1_SP";
    break;
  case PM10_SP:
    str = "PM10_SP";
    break;
  case PM03_PC:
    str = "PM003_PC";
    break;
  case PM05_PC:
    str = "PM005_PC";
    break;
  case PM01_PC:
    str = "PM01_PC";
    break;
  case PM25_PC:
    str = "PM25_PC";
    break;
  case PM5_PC:
    str = "PM05_PC";
    break;
  case PM10_PC:
    str = "PM10_PC";
    break;
  default:
    break;
  };

  return str;
}

void Measurements::validateChannel(int ch) {
  if (ch != 1 && ch != 2) {
    Serial.printf("ERROR! Channel %d is undefined. Only channel 1 or 2 is the optional value!", ch);
    delay(1000);
    assert(0);
  }
}

float Measurements::getCorrectedPM25(AirGradient &ag, Configuration &config, bool useAvg, int ch) {
  float pm25;
  float corrected;
  float humidity;
  float pm003Count;
  if (useAvg) {
    // Directly call from the index
    int channel = ch - 1; // Array index
    pm25 = _pm_25[channel].update.avg;
    humidity = _humidity[channel].update.avg;
    pm003Count = _pm_03_pc[channel].update.avg;
  } else {
    pm25 = get(PM25, ch);
    humidity = getFloat(Humidity, ch);
    pm003Count = get(PM03_PC, ch);
  }

  Configuration::PMCorrection pmCorrection = config.getPMCorrection();
  switch (pmCorrection.algorithm) {
  case PMCorrectionAlgorithm::Unknown:
  case PMCorrectionAlgorithm::None:
    // If correction is Unknown, then default is None
    corrected = pm25;
    break;
  case PMCorrectionAlgorithm::EPA_2021:
    corrected = ag.pms5003.compensate(pm25, humidity);
    break;
  default: {
    // All SLR correction using the same flow, hence default condition
    corrected = ag.pms5003.slrCorrection(pm25, pm003Count, pmCorrection.scalingFactor,
                                    pmCorrection.intercept);
    if (pmCorrection.useEPA) {
      // Add EPA compensation on top of SLR
      corrected = ag.pms5003.compensate(corrected, humidity);
    }
  }
  }

  return corrected;
}

String Measurements::toString(bool localServer, AgFirmwareMode fwMode, int rssi, AirGradient &ag,
                              Configuration &config) {
  JSONVar root;

  if (ag.isOne() || (ag.isPro4_2()) || ag.isPro3_3() || ag.isBasic()) {
    root = buildIndoor(localServer, ag, config);
  } else {
    root = buildOutdoor(localServer, fwMode, ag, config);
  }

  // CO2
  if (config.hasSensorS8 && utils::isValidCO2(_co2.update.avg)) {
    root[json_prop_co2] = ag.round2(_co2.update.avg);
  }

  /// TVOx and NOx
  if (config.hasSensorSGP) {
    if (utils::isValidVOC(_tvoc.update.avg)) {
      root[json_prop_tvoc] = ag.round2(_tvoc.update.avg);
    }
    if (utils::isValidVOC(_tvoc_raw.update.avg)) {
      root[json_prop_tvocRaw] = ag.round2(_tvoc_raw.update.avg);
    }
    if (utils::isValidNOx(_nox.update.avg)) {
      root[json_prop_nox] = ag.round2(_nox.update.avg);
    }
    if (utils::isValidNOx(_nox_raw.update.avg)) {
      root[json_prop_noxRaw] = ag.round2(_nox_raw.update.avg);
    }
  }

  root["boot"] = _bootCount;
  root["bootCount"] = _bootCount;
  root["wifi"] = rssi;

  if (localServer) {
    if (ag.isOne()) {
      root["ledMode"] = config.getLedBarModeName();
    }
    root["serialno"] = ag.deviceId();
    root["firmware"] = ag.getVersion();
    root["model"] = AgFirmwareModeName(fwMode);
  } else {
#ifndef ESP8266
    root["resetReason"] = _resetReason;
    root["freeHeap"] = ESP.getFreeHeap();
#endif
  }

  String result = JSON.stringify(root);
  Serial.printf("\n---- PAYLOAD\n %s \n-----\n", result.c_str());
  return result;
}

JSONVar Measurements::buildOutdoor(bool localServer, AgFirmwareMode fwMode, AirGradient &ag,
                                   Configuration &config) {
  JSONVar outdoor;
  if (fwMode == FW_MODE_O_1P || fwMode == FW_MODE_O_1PS || fwMode == FW_MODE_O_1PST) {
    // buildPMS params:
    /// Because only have 1 PMS, allCh is set to false
    /// But enable temp hum from PMS
    /// compensated values if requested by local server
    /// Set ch based on hasSensorPMSx
    if (config.hasSensorPMS1) {
      outdoor = buildPMS(ag, 1, false, true, localServer);
      if (!localServer) {
        outdoor[json_prop_pmFirmware] = pms5003TFirmwareVersion(ag.pms5003t_1.getFirmwareVersion());
      }
    } else {
      outdoor = buildPMS(ag, 2, false, true, localServer);
      if (!localServer) {
        outdoor[json_prop_pmFirmware] = pms5003TFirmwareVersion(ag.pms5003t_2.getFirmwareVersion());
      }
    }
  } else {
    // FW_MODE_O_1PPT && FW_MODE_O_1PP: Outdoor monitor that have 2 PMS sensor
    // buildPMS params:
    /// Have 2 PMS sensor, allCh is set to true (ch params ignored)
    /// Enable temp hum from PMS
    /// compensated values if requested by local server
    outdoor = buildPMS(ag, 1, true, true, localServer);
    // PMS5003T version
    if (!localServer) {
      outdoor["channels"]["1"][json_prop_pmFirmware] =
          pms5003TFirmwareVersion(ag.pms5003t_1.getFirmwareVersion());
      outdoor["channels"]["2"][json_prop_pmFirmware] =
          pms5003TFirmwareVersion(ag.pms5003t_2.getFirmwareVersion());
    }
  }

  return outdoor;
}

JSONVar Measurements::buildIndoor(bool localServer, AirGradient &ag, Configuration &config) {
  JSONVar indoor;

  if (config.hasSensorPMS1) {
    // buildPMS params:
    /// PMS channel 1 (indoor only have 1 PMS; hence allCh false)
    /// Not include temperature and humidity from PMS sensor
    /// Not include compensated calculation
    indoor = buildPMS(ag, 1, false, false, false);
    if (!localServer) {
      // Indoor is using PMS5003
      indoor[json_prop_pmFirmware] = this->pms5003FirmwareVersion(ag.pms5003.getFirmwareVersion());
    }
  }

  if (config.hasSensorSHT) {
    // Add temperature
    if (utils::isValidTemperature(_temperature[0].update.avg)) {
      indoor[json_prop_temp] = ag.round2(_temperature[0].update.avg);
      if (localServer) {
        indoor[json_prop_tempCompensated] = ag.round2(_temperature[0].update.avg);
      }
    }
    // Add humidity
    if (utils::isValidHumidity(_humidity[0].update.avg)) {
      indoor[json_prop_rhum] = ag.round2(_humidity[0].update.avg);
      if (localServer) {
        indoor[json_prop_rhumCompensated] = ag.round2(_humidity[0].update.avg);
      }
    }
  }

  // Add pm25 compensated value only if PM2.5 and humidity value is valid
  if (config.hasSensorPMS1 && utils::isValidPm(_pm_25[0].update.avg)) {
    if (config.hasSensorSHT && utils::isValidHumidity(_humidity[0].update.avg)) {
      // Correction using moving average value
      float tmp = getCorrectedPM25(ag, config, true);
      indoor[json_prop_pm25Compensated] = ag.round2(tmp);
    }
  }

  return indoor;
}

JSONVar Measurements::buildPMS(AirGradient &ag, int ch, bool allCh, bool withTempHum,
                               bool compensate) {
  JSONVar pms;

  // When only one of the channel
  if (allCh == false) {
    // Sanity check to validate channel, assert if invalid
    validateChannel(ch);

    // Follow array indexing just for get address of the value type
    ch = ch - 1;

    if (utils::isValidPm(_pm_01[ch].update.avg)) {
      pms[json_prop_pm01Ae] = ag.round2(_pm_01[ch].update.avg);
    }
    if (utils::isValidPm(_pm_25[ch].update.avg)) {
      pms[json_prop_pm25Ae] = ag.round2(_pm_25[ch].update.avg);
    }
    if (utils::isValidPm(_pm_10[ch].update.avg)) {
      pms[json_prop_pm10Ae] = ag.round2(_pm_10[ch].update.avg);
    }
    if (utils::isValidPm(_pm_01_sp[ch].update.avg)) {
      pms[json_prop_pm01Sp] = ag.round2(_pm_01_sp[ch].update.avg);
    }
    if (utils::isValidPm(_pm_25_sp[ch].update.avg)) {
      pms[json_prop_pm25Sp] = ag.round2(_pm_25_sp[ch].update.avg);
    }
    if (utils::isValidPm(_pm_10_sp[ch].update.avg)) {
      pms[json_prop_pm10Sp] = ag.round2(_pm_10_sp[ch].update.avg);
    }
    if (utils::isValidPm03Count(_pm_03_pc[ch].update.avg)) {
      pms[json_prop_pm03Count] = ag.round2(_pm_03_pc[ch].update.avg);
    }
    if (utils::isValidPm03Count(_pm_05_pc[ch].update.avg)) {
      pms[json_prop_pm05Count] = ag.round2(_pm_05_pc[ch].update.avg);
    }
    if (utils::isValidPm03Count(_pm_01_pc[ch].update.avg)) {
      pms[json_prop_pm1Count] = ag.round2(_pm_01_pc[ch].update.avg);
    }
    if (utils::isValidPm03Count(_pm_25_pc[ch].update.avg)) {
      pms[json_prop_pm25Count] = ag.round2(_pm_25_pc[ch].update.avg);
    }
    if (_pm_5_pc[ch].listValues.empty() == false) {
      // Only include pm5.0 count when values available on its list
      // If not, means no pm5_pc available from the sensor
      if (utils::isValidPm03Count(_pm_5_pc[ch].update.avg)) {
        pms[json_prop_pm5Count] = ag.round2(_pm_5_pc[ch].update.avg);
      }
    }
    if (_pm_10_pc[ch].listValues.empty() == false) {
      // Only include pm10 count when values available on its list
      // If not, means no pm10_pc available from the sensor
      if (utils::isValidPm03Count(_pm_10_pc[ch].update.avg)) {
        pms[json_prop_pm10Count] = ag.round2(_pm_10_pc[ch].update.avg);
      }
    }

    if (withTempHum) {
      float _vc;
      // Set temperature if valid
      if (utils::isValidTemperature(_temperature[ch].update.avg)) {
        pms[json_prop_temp] = ag.round2(_temperature[ch].update.avg);
        // Compensate temperature when flag is set
        if (compensate) {
          _vc = ag.pms5003t_1.compensateTemp(_temperature[ch].update.avg);
          if (utils::isValidTemperature(_vc)) {
            pms[json_prop_tempCompensated] = ag.round2(_vc);
          }
        }
      }
      // Set humidity if valid
      if (utils::isValidHumidity(_humidity[ch].update.avg)) {
        pms[json_prop_rhum] = ag.round2(_humidity[ch].update.avg);
        // Compensate relative humidity when flag is set
        if (compensate) {
          _vc = ag.pms5003t_1.compensateHum(_humidity[ch].update.avg);
          if (utils::isValidTemperature(_vc)) {
            pms[json_prop_rhumCompensated] = ag.round2(_vc);
          }
        }
      }

      // Add pm25 compensated value only if PM2.5 and humidity value is valid
      if (compensate) {
        if (utils::isValidPm(_pm_25[ch].update.avg) &&
            utils::isValidHumidity(_humidity[ch].update.avg)) {
          // Note: the pms5003t object is not matter either for channel 1 or 2, compensate points to
          // the same base function
          float pm25 = ag.pms5003t_1.compensate(_pm_25[ch].update.avg, _humidity[ch].update.avg);
          if (utils::isValidPm(pm25)) {
            pms[json_prop_pm25Compensated] = ag.round2(pm25);
          }
        }
      }
    }

    // Directly return the json object
    return pms;
  };

  /** Handle both channels by averaging their values; if one channel's value is not valid, skip
   * averaging and use the valid value from the other channel */

  /// PM1.0 atmospheric environment
  if (utils::isValidPm(_pm_01[0].update.avg) && utils::isValidPm(_pm_01[1].update.avg)) {
    float avg = (_pm_01[0].update.avg + _pm_01[1].update.avg) / 2.0f;
    pms[json_prop_pm01Ae] = ag.round2(avg);
    pms["channels"]["1"][json_prop_pm01Ae] = ag.round2(_pm_01[0].update.avg);
    pms["channels"]["2"][json_prop_pm01Ae] = ag.round2(_pm_01[1].update.avg);
  } else if (utils::isValidPm(_pm_01[0].update.avg)) {
    pms[json_prop_pm01Ae] = ag.round2(_pm_01[0].update.avg);
    pms["channels"]["1"][json_prop_pm01Ae] = ag.round2(_pm_01[0].update.avg);
  } else if (utils::isValidPm(_pm_01[1].update.avg)) {
    pms[json_prop_pm01Ae] = ag.round2(_pm_01[1].update.avg);
    pms["channels"]["2"][json_prop_pm01Ae] = ag.round2(_pm_01[1].update.avg);
  }

  /// PM2.5 atmospheric environment
  if (utils::isValidPm(_pm_25[0].update.avg) && utils::isValidPm(_pm_25[1].update.avg)) {
    float avg = (_pm_25[0].update.avg + _pm_25[1].update.avg) / 2.0f;
    pms[json_prop_pm25Ae] = ag.round2(avg);
    pms["channels"]["1"][json_prop_pm25Ae] = ag.round2(_pm_25[0].update.avg);
    pms["channels"]["2"][json_prop_pm25Ae] = ag.round2(_pm_25[1].update.avg);
  } else if (utils::isValidPm(_pm_25[0].update.avg)) {
    pms[json_prop_pm25Ae] = ag.round2(_pm_25[0].update.avg);
    pms["channels"]["1"][json_prop_pm25Ae] = ag.round2(_pm_25[0].update.avg);
  } else if (utils::isValidPm(_pm_25[1].update.avg)) {
    pms[json_prop_pm25Ae] = ag.round2(_pm_25[1].update.avg);
    pms["channels"]["2"][json_prop_pm25Ae] = ag.round2(_pm_25[1].update.avg);
  }

  /// PM10 atmospheric environment
  if (utils::isValidPm(_pm_10[0].update.avg) && utils::isValidPm(_pm_10[1].update.avg)) {
    float avg = (_pm_10[0].update.avg + _pm_10[1].update.avg) / 2.0f;
    pms[json_prop_pm10Ae] = ag.round2(avg);
    pms["channels"]["1"][json_prop_pm10Ae] = ag.round2(_pm_10[0].update.avg);
    pms["channels"]["2"][json_prop_pm10Ae] = ag.round2(_pm_10[1].update.avg);
  } else if (utils::isValidPm(_pm_10[0].update.avg)) {
    pms[json_prop_pm10Ae] = ag.round2(_pm_10[0].update.avg);
    pms["channels"]["1"][json_prop_pm10Ae] = ag.round2(_pm_10[0].update.avg);
  } else if (utils::isValidPm(_pm_10[1].update.avg)) {
    pms[json_prop_pm10Ae] = ag.round2(_pm_10[1].update.avg);
    pms["channels"]["2"][json_prop_pm10Ae] = ag.round2(_pm_10[1].update.avg);
  }

  /// PM1.0 standard particle
  if (utils::isValidPm(_pm_01_sp[0].update.avg) && utils::isValidPm(_pm_01_sp[1].update.avg)) {
    float avg = (_pm_01_sp[0].update.avg + _pm_01_sp[1].update.avg) / 2.0f;
    pms[json_prop_pm01Sp] = ag.round2(avg);
    pms["channels"]["1"][json_prop_pm01Sp] = ag.round2(_pm_01_sp[0].update.avg);
    pms["channels"]["2"][json_prop_pm01Sp] = ag.round2(_pm_01_sp[1].update.avg);
  } else if (utils::isValidPm(_pm_01_sp[0].update.avg)) {
    pms[json_prop_pm01Sp] = ag.round2(_pm_01_sp[0].update.avg);
    pms["channels"]["1"][json_prop_pm01Sp] = ag.round2(_pm_01_sp[0].update.avg);
  } else if (utils::isValidPm(_pm_01_sp[1].update.avg)) {
    pms[json_prop_pm01Sp] = ag.round2(_pm_01_sp[1].update.avg);
    pms["channels"]["2"][json_prop_pm01Sp] = ag.round2(_pm_01_sp[1].update.avg);
  }

  /// PM2.5 standard particle
  if (utils::isValidPm(_pm_25_sp[0].update.avg) && utils::isValidPm(_pm_25_sp[1].update.avg)) {
    float avg = (_pm_25_sp[0].update.avg + _pm_25_sp[1].update.avg) / 2.0f;
    pms[json_prop_pm25Sp] = ag.round2(avg);
    pms["channels"]["1"][json_prop_pm25Sp] = ag.round2(_pm_25_sp[0].update.avg);
    pms["channels"]["2"][json_prop_pm25Sp] = ag.round2(_pm_25_sp[1].update.avg);
  } else if (utils::isValidPm(_pm_25_sp[0].update.avg)) {
    pms[json_prop_pm25Sp] = ag.round2(_pm_25_sp[0].update.avg);
    pms["channels"]["1"][json_prop_pm25Sp] = ag.round2(_pm_25_sp[0].update.avg);
  } else if (utils::isValidPm(_pm_25_sp[1].update.avg)) {
    pms[json_prop_pm25Sp] = ag.round2(_pm_25_sp[1].update.avg);
    pms["channels"]["2"][json_prop_pm25Sp] = ag.round2(_pm_25_sp[1].update.avg);
  }

  /// PM10 standard particle
  if (utils::isValidPm(_pm_10_sp[0].update.avg) && utils::isValidPm(_pm_10_sp[1].update.avg)) {
    float avg = (_pm_10_sp[0].update.avg + _pm_10_sp[1].update.avg) / 2.0f;
    pms[json_prop_pm10Sp] = ag.round2(avg);
    pms["channels"]["1"][json_prop_pm10Sp] = ag.round2(_pm_10_sp[0].update.avg);
    pms["channels"]["2"][json_prop_pm10Sp] = ag.round2(_pm_10_sp[1].update.avg);
  } else if (utils::isValidPm(_pm_10_sp[0].update.avg)) {
    pms[json_prop_pm10Sp] = ag.round2(_pm_10_sp[0].update.avg);
    pms["channels"]["1"][json_prop_pm10Sp] = ag.round2(_pm_10_sp[0].update.avg);
  } else if (utils::isValidPm(_pm_10_sp[1].update.avg)) {
    pms[json_prop_pm10Sp] = ag.round2(_pm_10_sp[1].update.avg);
    pms["channels"]["2"][json_prop_pm10Sp] = ag.round2(_pm_10_sp[1].update.avg);
  }

  /// PM003 particle count
  if (utils::isValidPm03Count(_pm_03_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_03_pc[1].update.avg)) {
    float avg = (_pm_03_pc[0].update.avg + _pm_03_pc[1].update.avg) / 2.0f;
    pms[json_prop_pm03Count] = ag.round2(avg);
    pms["channels"]["1"][json_prop_pm03Count] = ag.round2(_pm_03_pc[0].update.avg);
    pms["channels"]["2"][json_prop_pm03Count] = ag.round2(_pm_03_pc[1].update.avg);
  } else if (utils::isValidPm03Count(_pm_03_pc[0].update.avg)) {
    pms[json_prop_pm03Count] = ag.round2(_pm_03_pc[0].update.avg);
    pms["channels"]["1"][json_prop_pm03Count] = ag.round2(_pm_03_pc[0].update.avg);
  } else if (utils::isValidPm03Count(_pm_03_pc[1].update.avg)) {
    pms[json_prop_pm03Count] = ag.round2(_pm_03_pc[1].update.avg);
    pms["channels"]["2"][json_prop_pm03Count] = ag.round2(_pm_03_pc[1].update.avg);
  }

  /// PM0.5 particle count
  if (utils::isValidPm03Count(_pm_05_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_05_pc[1].update.avg)) {
    float avg = (_pm_05_pc[0].update.avg + _pm_05_pc[1].update.avg) / 2.0f;
    pms[json_prop_pm05Count] = ag.round2(avg);
    pms["channels"]["1"][json_prop_pm05Count] = ag.round2(_pm_05_pc[0].update.avg);
    pms["channels"]["2"][json_prop_pm05Count] = ag.round2(_pm_05_pc[1].update.avg);
  } else if (utils::isValidPm03Count(_pm_05_pc[0].update.avg)) {
    pms[json_prop_pm05Count] = ag.round2(_pm_05_pc[0].update.avg);
    pms["channels"]["1"][json_prop_pm05Count] = ag.round2(_pm_05_pc[0].update.avg);
  } else if (utils::isValidPm03Count(_pm_05_pc[1].update.avg)) {
    pms[json_prop_pm05Count] = ag.round2(_pm_05_pc[1].update.avg);
    pms["channels"]["2"][json_prop_pm05Count] = ag.round2(_pm_05_pc[1].update.avg);
  }
  /// PM1.0 particle count
  if (utils::isValidPm03Count(_pm_01_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_01_pc[1].update.avg)) {
    float avg = (_pm_01_pc[0].update.avg + _pm_01_pc[1].update.avg) / 2.0f;
    pms[json_prop_pm1Count] = ag.round2(avg);
    pms["channels"]["1"][json_prop_pm1Count] = ag.round2(_pm_01_pc[0].update.avg);
    pms["channels"]["2"][json_prop_pm1Count] = ag.round2(_pm_01_pc[1].update.avg);
  } else if (utils::isValidPm03Count(_pm_01_pc[0].update.avg)) {
    pms[json_prop_pm1Count] = ag.round2(_pm_01_pc[0].update.avg);
    pms["channels"]["1"][json_prop_pm1Count] = ag.round2(_pm_01_pc[0].update.avg);
  } else if (utils::isValidPm03Count(_pm_01_pc[1].update.avg)) {
    pms[json_prop_pm1Count] = ag.round2(_pm_01_pc[1].update.avg);
    pms["channels"]["2"][json_prop_pm1Count] = ag.round2(_pm_01_pc[1].update.avg);
  }

  /// PM2.5 particle count
  if (utils::isValidPm03Count(_pm_25_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_25_pc[1].update.avg)) {
    float avg = (_pm_25_pc[0].update.avg + _pm_25_pc[1].update.avg) / 2.0f;
    pms[json_prop_pm25Count] = ag.round2(avg);
    pms["channels"]["1"][json_prop_pm25Count] = ag.round2(_pm_25_pc[0].update.avg);
    pms["channels"]["2"][json_prop_pm25Count] = ag.round2(_pm_25_pc[1].update.avg);
  } else if (utils::isValidPm03Count(_pm_25_pc[0].update.avg)) {
    pms[json_prop_pm25Count] = ag.round2(_pm_25_pc[0].update.avg);
    pms["channels"]["1"][json_prop_pm25Count] = ag.round2(_pm_25_pc[0].update.avg);
  } else if (utils::isValidPm03Count(_pm_25_pc[1].update.avg)) {
    pms[json_prop_pm25Count] = ag.round2(_pm_25_pc[1].update.avg);
    pms["channels"]["2"][json_prop_pm25Count] = ag.round2(_pm_25_pc[1].update.avg);
  }

  // NOTE: No need for particle count 5.0 and 10. When allCh is true, basically monitor using
  // PM5003T, which don't have PC 5.0 and 10

  if (withTempHum) {
    /// Temperature
    if (utils::isValidTemperature(_temperature[0].update.avg) &&
        utils::isValidTemperature(_temperature[1].update.avg)) {

      float temperature = (_temperature[0].update.avg + _temperature[1].update.avg) / 2.0f;
      pms[json_prop_temp] = ag.round2(temperature);
      pms["channels"]["1"][json_prop_temp] = ag.round2(_temperature[0].update.avg);
      pms["channels"]["2"][json_prop_temp] = ag.round2(_temperature[1].update.avg);

      if (compensate) {
        // Compensate both temperature channel
        float temp = ag.pms5003t_1.compensateTemp(temperature);
        float temp1 = ag.pms5003t_1.compensateTemp(_temperature[0].update.avg);
        float temp2 = ag.pms5003t_2.compensateTemp(_temperature[1].update.avg);
        pms[json_prop_tempCompensated] = ag.round2(temp);
        pms["channels"]["1"][json_prop_tempCompensated] = ag.round2(temp1);
        pms["channels"]["2"][json_prop_tempCompensated] = ag.round2(temp2);
      }

    } else if (utils::isValidTemperature(_temperature[0].update.avg)) {
      pms[json_prop_temp] = ag.round2(_temperature[0].update.avg);
      pms["channels"]["1"][json_prop_temp] = ag.round2(_temperature[0].update.avg);

      if (compensate) {
        // Compensate channel 1
        float temp1 = ag.pms5003t_1.compensateTemp(_temperature[0].update.avg);
        pms[json_prop_tempCompensated] = ag.round2(temp1);
        pms["channels"]["1"][json_prop_tempCompensated] = ag.round2(temp1);
      }

    } else if (utils::isValidTemperature(_temperature[1].update.avg)) {
      pms[json_prop_temp] = ag.round2(_temperature[1].update.avg);
      pms["channels"]["2"][json_prop_temp] = ag.round2(_temperature[1].update.avg);

      if (compensate) {
        // Compensate channel 2
        float temp2 = ag.pms5003t_2.compensateTemp(_temperature[1].update.avg);
        pms[json_prop_tempCompensated] = ag.round2(temp2);
        pms["channels"]["2"][json_prop_tempCompensated] = ag.round2(temp2);
      }
    }

    /// Relative humidity
    if (utils::isValidHumidity(_humidity[0].update.avg) &&
        utils::isValidHumidity(_humidity[1].update.avg)) {
      float humidity = (_humidity[0].update.avg + _humidity[1].update.avg) / 2.0f;
      pms[json_prop_rhum] = ag.round2(humidity);
      pms["channels"]["1"][json_prop_rhum] = ag.round2(_humidity[0].update.avg);
      pms["channels"]["2"][json_prop_rhum] = ag.round2(_humidity[1].update.avg);

      if (compensate) {
        // Compensate both humidity channel
        float hum = ag.pms5003t_1.compensateHum(humidity);
        float hum1 = ag.pms5003t_1.compensateHum(_humidity[0].update.avg);
        float hum2 = ag.pms5003t_2.compensateHum(_humidity[1].update.avg);
        pms[json_prop_rhumCompensated] = ag.round2(hum);
        pms["channels"]["1"][json_prop_rhumCompensated] = ag.round2(hum1);
        pms["channels"]["2"][json_prop_rhumCompensated] = ag.round2(hum2);
      }

    } else if (utils::isValidHumidity(_humidity[0].update.avg)) {
      pms[json_prop_rhum] = ag.round2(_humidity[0].update.avg);
      pms["channels"]["1"][json_prop_rhum] = ag.round2(_humidity[0].update.avg);

      if (compensate) {
        // Compensate humidity channel 1
        float hum1 = ag.pms5003t_1.compensateHum(_humidity[0].update.avg);
        pms[json_prop_rhumCompensated] = ag.round2(hum1);
        pms["channels"]["1"][json_prop_rhumCompensated] = ag.round2(hum1);
      }

    } else if (utils::isValidHumidity(_humidity[1].update.avg)) {
      pms[json_prop_rhum] = ag.round2(_humidity[1].update.avg);
      pms["channels"]["2"][json_prop_rhum] = ag.round2(_humidity[1].update.avg);

      if (compensate) {
        // Compensate humidity channel 2
        float hum2 = ag.pms5003t_2.compensateHum(_humidity[1].update.avg);
        pms[json_prop_rhumCompensated] = ag.round2(hum2);
        pms["channels"]["2"][json_prop_rhumCompensated] = ag.round2(hum2);
      }
    }

    if (compensate) {
      // Add pm25 compensated value
      /// First get both channel compensated value
      float pm25_comp1 = utils::getInvalidPmValue();
      float pm25_comp2 = utils::getInvalidPmValue();
      if (utils::isValidPm(_pm_25[0].update.avg) &&
          utils::isValidHumidity(_humidity[0].update.avg)) {
        pm25_comp1 = ag.pms5003t_1.compensate(_pm_25[0].update.avg, _humidity[0].update.avg);
        pms["channels"]["1"][json_prop_pm25Compensated] = ag.round2(pm25_comp1);
      }
      if (utils::isValidPm(_pm_25[1].update.avg) &&
          utils::isValidHumidity(_humidity[1].update.avg)) {
        pm25_comp2 = ag.pms5003t_2.compensate(_pm_25[1].update.avg, _humidity[1].update.avg);
        pms["channels"]["2"][json_prop_pm25Compensated] = ag.round2(pm25_comp2);
      }

      /// Get average or one of the channel compensated value if only one channel is valid
      if (utils::isValidPm(pm25_comp1) && utils::isValidPm(pm25_comp2)) {
        pms[json_prop_pm25Compensated] = ag.round2((pm25_comp1 + pm25_comp2) / 2.0f);
      } else if (utils::isValidPm(pm25_comp1)) {
        pms[json_prop_pm25Compensated] = ag.round2(pm25_comp1);
      } else if (utils::isValidPm(pm25_comp2)) {
        pms[json_prop_pm25Compensated] = ag.round2(pm25_comp2);
      }
    }
  }

  return pms;
}

void Measurements::setDebug(bool debug) { _debug = debug; }

int Measurements::bootCount() { return _bootCount; }

void Measurements::setBootCount(int bootCount) { _bootCount = bootCount; }

#ifndef ESP8266
void Measurements::setResetReason(esp_reset_reason_t reason) {
  switch (reason) {
  case ESP_RST_UNKNOWN:
    Serial.println("Reset reason: ESP_RST_UNKNOWN");
    break;
  case ESP_RST_POWERON:
    Serial.println("Reset reason: ESP_RST_POWERON");
    break;
  case ESP_RST_EXT:
    Serial.println("Reset reason: ESP_RST_EXT");
    break;
  case ESP_RST_SW:
    Serial.println("Reset reason: ESP_RST_SW");
    break;
  case ESP_RST_PANIC:
    Serial.println("Reset reason: ESP_RST_PANIC");
    break;
  case ESP_RST_INT_WDT:
    Serial.println("Reset reason: ESP_RST_INT_WDT");
    break;
  case ESP_RST_TASK_WDT:
    Serial.println("Reset reason: ESP_RST_TASK_WDT");
    break;
  case ESP_RST_WDT:
    Serial.println("Reset reason: ESP_RST_WDT");
    break;
  case ESP_RST_BROWNOUT:
    Serial.println("Reset reason: ESP_RST_BROWNOUT");
    break;
  case ESP_RST_SDIO:
    Serial.println("Reset reason: ESP_RST_SDIO");
    break;
  default:
    Serial.println("Reset reason: unknown");
    break;
  }

  _resetReason = (int)reason;
}
#endif