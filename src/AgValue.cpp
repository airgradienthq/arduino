#include "AgValue.h"
#include "AgConfigure.h"
#include "AirGradient.h"

#define json_prop_pmFirmware     "firmware"

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
  case PM01_PC:
    _pm_01_pc[0].update.max = max;
    _pm_01_pc[1].update.max = max;
    break;
  case PM25_PC:
    _pm_25_pc[0].update.max = max;
    _pm_25_pc[1].update.max = max;
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
  case PM01_PC:
    temporary = &_pm_01_pc[ch];
    invalidValue = utils::getInvalidPmValue();
    break;
  case PM25_PC:
    temporary = &_pm_25_pc[ch];
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
    Serial.printf("%s is not defined for integer data type\n", measurementTypeStr(type));
    // TODO: Just assert?
    return false;
  }

  // Restore channel value for debugging purpose
  ch = ch + 1;

  if (val == invalidValue) {
    temporary->update.invalidCounter++;
    if (temporary->update.invalidCounter >= temporary->update.max) {
      Serial.printf("%s{%d} invalid value update counter reached (%dx)! Setting its average value "
                    "to invalid!",
                    measurementTypeStr(type), ch, temporary->update.max);
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
    Serial.printf("%s{%d}: %.2f\n", measurementTypeStr(type), ch, temporary->update.avg);
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
    Serial.printf("%s is not defined for float data type\n", measurementTypeStr(type));
    // TODO: Just assert?
    return false;
  }

  // Restore channel value for debugging purpose
  ch = ch + 1;

  if (val == invalidValue) {
    temporary->update.invalidCounter++;
    if (temporary->update.invalidCounter >= temporary->update.max) {
      Serial.printf("%s{%d} invalid value update counter reached (%dx)! Setting its average value "
                    "to invalid!",
                    measurementTypeStr(type), ch, temporary->update.max);
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
    Serial.printf("%s{%d}: %.2f\n", measurementTypeStr(type), ch, temporary->update.avg);
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
    Serial.printf("%s is not defined for integer data type\n", measurementTypeStr(type));
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
    Serial.printf("%s is not defined for float data type\n", measurementTypeStr(type));
    // TODO: Just assert?
    return false;
  }

  if (temporary->listValues.empty()) {
    // Values still empty, return 0
    return 0;
  }

  return temporary->listValues.back();
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
  case PM01_PC:
    str = "PM01_PC";
    break;
  case PM25_PC:
    str = "PM25_PC";
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
    root["rco2"] = ag.round2(_co2.update.avg);
  }

  /// TVOx and NOx
  if (config.hasSensorSGP) {
    if (utils::isValidVOC(_tvoc.update.avg)) {
      root["tvocIndex"] = ag.round2(_tvoc.update.avg);
    }
    if (utils::isValidVOC(_tvoc_raw.update.avg)) {
      root["tvocRaw"] = ag.round2(_tvoc_raw.update.avg);
    }
    if (utils::isValidNOx(_nox.update.avg)) {
      root["noxIndex"] = ag.round2(_nox.update.avg);
    }
    if (utils::isValidNOx(_nox_raw.update.avg)) {
      root["noxRaw"] = ag.round2(_nox_raw.update.avg);
    }
  }

  root["boot"] = bootCount;
  root["bootCount"] = bootCount;
  root["wifi"] = rssi;

  if (localServer) {
    if (ag.isOne()) {
      root["ledMode"] = config.getLedBarModeName();
    }
    root["serialno"] = ag.deviceId();
    root["firmware"] = ag.getVersion();
    root["model"] = AgFirmwareModeName(fwMode);
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
      indoor["atmp"] = ag.round2(_temperature[0].update.avg);
      if (localServer) {
        indoor["atmpCompensated"] = ag.round2(_temperature[0].update.avg);
      }
    }
    // Add humidity
    if (utils::isValidHumidity(_humidity[0].update.avg)) {
      indoor["rhum"] = ag.round2(_humidity[0].update.avg);
      if (localServer) {
        indoor["rhumCompensated"] = ag.round2(_humidity[0].update.avg);
      }
    }
  }

  // Add pm25 compensated value only if PM2.5 and humidity value is valid
  if (config.hasSensorPMS1 && utils::isValidPm(_pm_25[0].update.avg)) {
    if (config.hasSensorSHT && utils::isValidHumidity(_humidity[0].update.avg)) {
      float pm25 = ag.pms5003.compensate(_pm_25[0].update.avg, _humidity[0].update.avg);
      if (utils::isValidPm(pm25)) {
        indoor["pm02Compensated"] = ag.round2(pm25);
      }
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
      pms["pm01"] = ag.round2(_pm_01[ch].update.avg);
    }
    if (utils::isValidPm(_pm_25[ch].update.avg)) {
      pms["pm02"] = ag.round2(_pm_25[ch].update.avg);
    }
    if (utils::isValidPm(_pm_10[ch].update.avg)) {
      pms["pm10"] = ag.round2(_pm_10[ch].update.avg);
    }
    if (utils::isValidPm(_pm_01_sp[ch].update.avg)) {
      pms["pm01_sp"] = ag.round2(_pm_01_sp[ch].update.avg);
    }
    if (utils::isValidPm(_pm_25_sp[ch].update.avg)) {
      pms["pm02_sp"] = ag.round2(_pm_25_sp[ch].update.avg);
    }
    if (utils::isValidPm(_pm_10_sp[ch].update.avg)) {
      pms["pm10_sp"] = ag.round2(_pm_10_sp[ch].update.avg);
    }
    if (utils::isValidPm03Count(_pm_03_pc[ch].update.avg)) {
      pms["pm003Count"] = ag.round2(_pm_03_pc[ch].update.avg);
    }
    if (utils::isValidPm03Count(_pm_01_pc[ch].update.avg)) {
      pms["pm01Count"] = ag.round2(_pm_01_pc[ch].update.avg);
    }
    if (utils::isValidPm03Count(_pm_25_pc[ch].update.avg)) {
      pms["pm02Count"] = ag.round2(_pm_25_pc[ch].update.avg);
    }
    if (_pm_10_pc[ch].listValues.empty() == false) {
      // Only include pm10 count when values available on its list
      // If not, means no pm10_pc available from the sensor
      if (utils::isValidPm03Count(_pm_10_pc[ch].update.avg)) {
        pms["pm10Count"] = ag.round2(_pm_10_pc[ch].update.avg);
      }
    }

    if (withTempHum) {
      float _vc;
      // Set temperature if valid
      if (utils::isValidTemperature(_temperature[ch].update.avg)) {
        pms["atmp"] = ag.round2(_temperature[ch].update.avg);
        // Compensate temperature when flag is set
        if (compensate) {
          _vc = ag.pms5003t_1.compensateTemp(_temperature[ch].update.avg);
          if (utils::isValidTemperature(_vc)) {
            pms["atmpCompensated"] = ag.round2(_vc);
          }
        }
      }
      // Set humidity if valid
      if (utils::isValidHumidity(_humidity[ch].update.avg)) {
        pms["rhum"] = ag.round2(_humidity[ch].update.avg);
        // Compensate relative humidity when flag is set
        if (compensate) {
          _vc = ag.pms5003t_1.compensateHum(_humidity[ch].update.avg);
          if (utils::isValidTemperature(_vc)) {
            pms["rhumCompensated"] = ag.round2(_vc);
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
            pms["pm02Compensated"] = ag.round2(pm25);
          }
        }
      }
    }

    // Directly return the json object
    return pms;
  };

  /** Handle both channel with average, if one of the channel not valid, use another one */

  /// PM1.0 atmospheric environment
  if (utils::isValidPm(_pm_01[0].update.avg) && utils::isValidPm(_pm_01[1].update.avg)) {
    float avg = (_pm_01[0].update.avg + _pm_01[1].update.avg) / 2.0f;
    pms["pm01"] = ag.round2(avg);
    pms["channels"]["1"]["pm01"] = ag.round2(_pm_01[0].update.avg);
    pms["channels"]["2"]["pm01"] = ag.round2(_pm_01[1].update.avg);
  } else if (utils::isValidPm(_pm_01[0].update.avg)) {
    pms["pm01"] = ag.round2(_pm_01[0].update.avg);
    pms["channels"]["1"]["pm01"] = ag.round2(_pm_01[0].update.avg);
  } else if (utils::isValidPm(_pm_01[1].update.avg)) {
    pms["pm01"] = ag.round2(_pm_01[1].update.avg);
    pms["channels"]["2"]["pm01"] = ag.round2(_pm_01[1].update.avg);
  }

  /// PM2.5 atmospheric environment
  if (utils::isValidPm(_pm_25[0].update.avg) && utils::isValidPm(_pm_25[1].update.avg)) {
    float avg = (_pm_25[0].update.avg + _pm_25[1].update.avg) / 2.0f;
    pms["pm02"] = ag.round2(avg);
    pms["channels"]["1"]["pm02"] = ag.round2(_pm_25[0].update.avg);
    pms["channels"]["2"]["pm02"] = ag.round2(_pm_25[1].update.avg);
  } else if (utils::isValidPm(_pm_25[0].update.avg)) {
    pms["pm02"] = ag.round2(_pm_25[0].update.avg);
    pms["channels"]["1"]["pm02"] = ag.round2(_pm_25[0].update.avg);
  } else if (utils::isValidPm(_pm_25[1].update.avg)) {
    pms["pm02"] = ag.round2(_pm_25[1].update.avg);
    pms["channels"]["2"]["pm02"] = ag.round2(_pm_25[1].update.avg);
  }

  /// PM10 atmospheric environment
  if (utils::isValidPm(_pm_10[0].update.avg) && utils::isValidPm(_pm_10[1].update.avg)) {
    float avg = (_pm_10[0].update.avg + _pm_10[1].update.avg) / 2.0f;
    pms["pm10"] = ag.round2(avg);
    pms["channels"]["1"]["pm10"] = ag.round2(_pm_10[0].update.avg);
    pms["channels"]["2"]["pm10"] = ag.round2(_pm_10[1].update.avg);
  } else if (utils::isValidPm(_pm_10[0].update.avg)) {
    pms["pm10"] = ag.round2(_pm_10[0].update.avg);
    pms["channels"]["1"]["pm10"] = ag.round2(_pm_10[0].update.avg);
  } else if (utils::isValidPm(_pm_10[1].update.avg)) {
    pms["pm10"] = ag.round2(_pm_10[1].update.avg);
    pms["channels"]["2"]["pm10"] = ag.round2(_pm_10[1].update.avg);
  }

  /// PM1.0 standard particle
  if (utils::isValidPm(_pm_01_sp[0].update.avg) && utils::isValidPm(_pm_01_sp[1].update.avg)) {
    float avg = (_pm_01_sp[0].update.avg + _pm_01_sp[1].update.avg) / 2.0f;
    pms["pm01_sp"] = ag.round2(avg);
    pms["channels"]["1"]["pm01_sp"] = ag.round2(_pm_01_sp[0].update.avg);
    pms["channels"]["2"]["pm01_sp"] = ag.round2(_pm_01_sp[1].update.avg);
  } else if (utils::isValidPm(_pm_01_sp[0].update.avg)) {
    pms["pm01_sp"] = ag.round2(_pm_01_sp[0].update.avg);
    pms["channels"]["1"]["pm01_sp"] = ag.round2(_pm_01_sp[0].update.avg);
  } else if (utils::isValidPm(_pm_01_sp[1].update.avg)) {
    pms["pm01_sp"] = ag.round2(_pm_01_sp[1].update.avg);
    pms["channels"]["2"]["pm01_sp"] = ag.round2(_pm_01_sp[1].update.avg);
  }

  /// PM2.5 standard particle
  if (utils::isValidPm(_pm_25_sp[0].update.avg) && utils::isValidPm(_pm_25_sp[1].update.avg)) {
    float avg = (_pm_25_sp[0].update.avg + _pm_25_sp[1].update.avg) / 2.0f;
    pms["pm02_sp"] = ag.round2(avg);
    pms["channels"]["1"]["pm02_sp"] = ag.round2(_pm_25_sp[0].update.avg);
    pms["channels"]["2"]["pm02_sp"] = ag.round2(_pm_25_sp[1].update.avg);
  } else if (utils::isValidPm(_pm_25_sp[0].update.avg)) {
    pms["pm02_sp"] = ag.round2(_pm_25_sp[0].update.avg);
    pms["channels"]["1"]["pm02_sp"] = ag.round2(_pm_25_sp[0].update.avg);
  } else if (utils::isValidPm(_pm_25_sp[1].update.avg)) {
    pms["pm02_sp"] = ag.round2(_pm_25_sp[1].update.avg);
    pms["channels"]["2"]["pm02_sp"] = ag.round2(_pm_25_sp[1].update.avg);
  }

  /// PM10 standard particle
  if (utils::isValidPm(_pm_10_sp[0].update.avg) && utils::isValidPm(_pm_10_sp[1].update.avg)) {
    float avg = (_pm_10_sp[0].update.avg + _pm_10_sp[1].update.avg) / 2.0f;
    pms["pm10_sp"] = ag.round2(avg);
    pms["channels"]["1"]["pm10_sp"] = ag.round2(_pm_10_sp[0].update.avg);
    pms["channels"]["2"]["pm10_sp"] = ag.round2(_pm_10_sp[1].update.avg);
  } else if (utils::isValidPm(_pm_10_sp[0].update.avg)) {
    pms["pm10_sp"] = ag.round2(_pm_10_sp[0].update.avg);
    pms["channels"]["1"]["pm10_sp"] = ag.round2(_pm_10_sp[0].update.avg);
  } else if (utils::isValidPm(_pm_10_sp[1].update.avg)) {
    pms["pm10_sp"] = ag.round2(_pm_10_sp[1].update.avg);
    pms["channels"]["2"]["pm10_sp"] = ag.round2(_pm_10_sp[1].update.avg);
  }

  /// PM003 particle count
  if (utils::isValidPm03Count(_pm_03_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_03_pc[1].update.avg)) {
    float avg = (_pm_03_pc[0].update.avg + _pm_03_pc[1].update.avg) / 2.0f;
    pms["pm003Count"] = ag.round2(avg);
    pms["channels"]["1"]["pm003Count"] = ag.round2(_pm_03_pc[0].update.avg);
    pms["channels"]["2"]["pm003Count"] = ag.round2(_pm_03_pc[1].update.avg);
  } else if (utils::isValidPm(_pm_03_pc[0].update.avg)) {
    pms["pm003Count"] = ag.round2(_pm_03_pc[0].update.avg);
    pms["channels"]["1"]["pm003Count"] = ag.round2(_pm_03_pc[0].update.avg);
  } else if (utils::isValidPm(_pm_03_pc[1].update.avg)) {
    pms["pm003Count"] = ag.round2(_pm_03_pc[1].update.avg);
    pms["channels"]["2"]["pm003Count"] = ag.round2(_pm_03_pc[1].update.avg);
  }

  /// PM1.0 particle count
  if (utils::isValidPm03Count(_pm_01_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_01_pc[1].update.avg)) {
    float avg = (_pm_01_pc[0].update.avg + _pm_01_pc[1].update.avg) / 2.0f;
    pms["pm01Count"] = ag.round2(avg);
    pms["channels"]["1"]["pm01Count"] = ag.round2(_pm_01_pc[0].update.avg);
    pms["channels"]["2"]["pm01Count"] = ag.round2(_pm_01_pc[1].update.avg);
  } else if (utils::isValidPm(_pm_01_pc[0].update.avg)) {
    pms["pm01Count"] = ag.round2(_pm_01_pc[0].update.avg);
    pms["channels"]["1"]["pm01Count"] = ag.round2(_pm_01_pc[0].update.avg);
  } else if (utils::isValidPm(_pm_01_pc[1].update.avg)) {
    pms["pm01Count"] = ag.round2(_pm_01_pc[1].update.avg);
    pms["channels"]["2"]["pm01Count"] = ag.round2(_pm_01_pc[1].update.avg);
  }

  /// PM2.5 particle count
  if (utils::isValidPm03Count(_pm_25_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_25_pc[1].update.avg)) {
    float avg = (_pm_25_pc[0].update.avg + _pm_25_pc[1].update.avg) / 2.0f;
    pms["pm25Count"] = ag.round2(avg);
    pms["channels"]["1"]["pm25Count"] = ag.round2(_pm_25_pc[0].update.avg);
    pms["channels"]["2"]["pm25Count"] = ag.round2(_pm_25_pc[1].update.avg);
  } else if (utils::isValidPm(_pm_25_pc[0].update.avg)) {
    pms["pm25Count"] = ag.round2(_pm_25_pc[0].update.avg);
    pms["channels"]["1"]["pm25Count"] = ag.round2(_pm_25_pc[0].update.avg);
  } else if (utils::isValidPm(_pm_25_pc[1].update.avg)) {
    pms["pm25Count"] = ag.round2(_pm_25_pc[1].update.avg);
    pms["channels"]["2"]["pm25Count"] = ag.round2(_pm_25_pc[1].update.avg);
  }

  // NOTE: No need for particle count 10. When allCh is true, basically monitor using PM5003T, which
  // don't have PC 10
  // /// PM10 particle count
  // if (utils::isValidPm03Count(_pm_10_pc[0].update.avg) &&
  //     utils::isValidPm03Count(_pm_10_pc[1].update.avg)) {
  //   float avg = (_pm_10_pc[0].update.avg + _pm_10_pc[1].update.avg) / 2.0f;
  //   pms["pm10Count"] = ag.round2(avg);
  //   pms["channels"]["1"]["pm10Count"] = ag.round2(_pm_10_pc[0].update.avg);
  //   pms["channels"]["2"]["pm10Count"] = ag.round2(_pm_10_pc[1].update.avg);
  // } else if (utils::isValidPm(_pm_10_pc[0].update.avg)) {
  //   pms["pm10Count"] = ag.round2(_pm_10_pc[0].update.avg);
  //   pms["channels"]["1"]["pm10Count"] = ag.round2(_pm_10_pc[0].update.avg);
  // } else if (utils::isValidPm(_pm_10_pc[1].update.avg)) {
  //   pms["pm10Count"] = ag.round2(_pm_10_pc[1].update.avg);
  //   pms["channels"]["2"]["pm10Count"] = ag.round2(_pm_10_pc[1].update.avg);
  // }

  if (withTempHum) {
    /// Temperature
    if (utils::isValidTemperature(_temperature[0].update.avg) &&
        utils::isValidTemperature(_temperature[1].update.avg)) {

      float temperature = (_temperature[0].update.avg + _temperature[1].update.avg) / 2.0f;
      pms["atmp"] = ag.round2(temperature);
      pms["channels"]["1"]["atmp"] = ag.round2(_temperature[0].update.avg);
      pms["channels"]["2"]["atmp"] = ag.round2(_temperature[1].update.avg);

      if (compensate) {
        // Compensate both temperature channel
        float temp = ag.pms5003t_1.compensateTemp(temperature);
        float temp1 = ag.pms5003t_1.compensateTemp(_temperature[0].update.avg);
        float temp2 = ag.pms5003t_2.compensateTemp(_temperature[1].update.avg);
        pms["atmpCompensated"] = ag.round2(temp);
        pms["channels"]["1"]["atmpCompensated"] = ag.round2(temp1);
        pms["channels"]["2"]["atmpCompensated"] = ag.round2(temp2);
      }

    } else if (utils::isValidTemperature(_temperature[0].update.avg)) {
      pms["atmp"] = ag.round2(_temperature[0].update.avg);
      pms["channels"]["1"]["atmp"] = ag.round2(_temperature[0].update.avg);

      if (compensate) {
        // Compensate channel 1
        float temp1 = ag.pms5003t_1.compensateTemp(_temperature[0].update.avg);
        pms["atmpCompensated"] = ag.round2(temp1);
        pms["channels"]["1"]["atmpCompensated"] = ag.round2(temp1);
      }

    } else if (utils::isValidTemperature(_temperature[1].update.avg)) {
      pms["atmp"] = ag.round2(_temperature[1].update.avg);
      pms["channels"]["2"]["atmp"] = ag.round2(_temperature[1].update.avg);

      if (compensate) {
        // Compensate channel 2
        float temp2 = ag.pms5003t_2.compensateTemp(_temperature[1].update.avg);
        pms["atmpCompensated"] = ag.round2(temp2);
        pms["channels"]["2"]["atmpCompensated"] = ag.round2(temp2);
      }
    }

    /// Relative humidity
    if (utils::isValidHumidity(_humidity[0].update.avg) &&
        utils::isValidHumidity(_humidity[1].update.avg)) {
      float humidity = (_humidity[0].update.avg + _humidity[1].update.avg) / 2.0f;
      pms["rhum"] = ag.round2(humidity);
      pms["channels"]["1"]["rhum"] = ag.round2(_humidity[0].update.avg);
      pms["channels"]["2"]["rhum"] = ag.round2(_humidity[1].update.avg);

      if (compensate) {
        // Compensate both humidity channel
        float hum = ag.pms5003t_1.compensateHum(humidity);
        float hum1 = ag.pms5003t_1.compensateHum(_humidity[0].update.avg);
        float hum2 = ag.pms5003t_2.compensateHum(_humidity[1].update.avg);
        pms["rhumCompensated"] = ag.round2(hum);
        pms["channels"]["1"]["rhumCompensated"] = ag.round2(hum1);
        pms["channels"]["2"]["rhumCompensated"] = ag.round2(hum2);
      }

    } else if (utils::isValidHumidity(_humidity[0].update.avg)) {
      pms["rhum"] = ag.round2(_humidity[0].update.avg);
      pms["channels"]["1"]["rhum"] = ag.round2(_humidity[0].update.avg);

      if (compensate) {
        // Compensate humidity channel 1
        float hum1 = ag.pms5003t_1.compensateHum(_humidity[0].update.avg);
        pms["rhumCompensated"] = ag.round2(hum1);
        pms["channels"]["1"]["rhumCompensated"] = ag.round2(hum1);
      }

    } else if (utils::isValidHumidity(_humidity[1].update.avg)) {
      pms["rhum"] = ag.round2(_humidity[1].update.avg);
      pms["channels"]["2"]["rhum"] = ag.round2(_humidity[1].update.avg);

      if (compensate) {
        // Compensate humidity channel 2
        float hum2 = ag.pms5003t_2.compensateHum(_humidity[1].update.avg);
        pms["rhumCompensated"] = ag.round2(hum2);
        pms["channels"]["2"]["rhumCompensated"] = ag.round2(hum2);
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
        pms["channels"]["1"]["pm02Compensated"] = ag.round2(pm25_comp1);
      }
      if (utils::isValidPm(_pm_25[1].update.avg) &&
          utils::isValidHumidity(_humidity[1].update.avg)) {
        pm25_comp2 = ag.pms5003t_2.compensate(_pm_25[1].update.avg, _humidity[1].update.avg);
        pms["channels"]["2"]["pm02Compensated"] = ag.round2(pm25_comp2);
      }

      /// Get average or one of the channel compensated value if only one channel is valid
      if (utils::isValidPm(pm25_comp1) && utils::isValidPm(pm25_comp2)) {
        pms["pm02Compensated"] = ag.round2((pm25_comp1 + pm25_comp2) / 2.0f);
      } else if (utils::isValidPm(pm25_comp1)) {
        pms["pm02Compensated"] = ag.round2(pm25_comp1);
      } else if (utils::isValidPm(pm25_comp2)) {
        pms["pm02Compensated"] = ag.round2(pm25_comp2);
      }
    }
  }

  return pms;
}

void Measurements::setDebug(bool debug) { _debug = debug; }