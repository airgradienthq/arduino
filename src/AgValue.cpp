#include "AgValue.h"
#include "AgConfigure.h"
#include "AirGradient.h"

#define json_prop_pmFirmware     "firmware"

void Measurements::maxUpdate(MeasurementType type, int max) {
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
  case PM03_PC:
    _pm_03_pc[0].update.max = max;
    _pm_03_pc[1].update.max = max;
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
  float invalidValue = 0;
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
  case PM03_PC:
    temporary = &_pm_03_pc[ch];
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

  // Update new value when value provided is not the invalid one
  if (val != invalidValue) {
    temporary->lastValue = val;
    temporary->sumValues = temporary->sumValues + val;
    temporary->update.success = temporary->update.success + 1;
  }

  // Increment update.counter
  temporary->update.counter = temporary->update.counter + 1;

  // Calculate value average when maximum set is reached
  if (temporary->update.counter >= temporary->update.max) {
    // TODO: Need to check if SUCCESS == 0, what should we do?
    // Calculate the average
    temporary->avg = temporary->sumValues / temporary->update.success;
    Serial.printf("%s{%d} count reached! Average value %d\n", measurementTypeStr(type), ch,
                  temporary->avg);

    // Notify if there's are invalid value when updating
    int miss = temporary->update.max - temporary->update.success;
    if (miss != 0) {
      Serial.printf("%s{%d} has %d invalid value out of %d update\n", measurementTypeStr(type), ch,
                    miss, temporary->update.max);
    }

    // Resets average related variable calculation
    temporary->sumValues = 0;
    temporary->update.counter = 0;
    temporary->update.success = 0;
    return true;
  }

  return false;
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

  // Update new value when value provided is not the invalid one
  if (val != invalidValue) {
    temporary->lastValue = val;
    temporary->sumValues = temporary->sumValues + val;
    temporary->update.success = temporary->update.success + 1;
  }

  // Increment update.counter
  temporary->update.counter = temporary->update.counter + 1;

  // Calculate value average when maximum set is reached
  if (temporary->update.counter >= temporary->update.max) {
    // TODO: Need to check if SUCCESS == 0
    // Calculate the average
    temporary->avg = temporary->sumValues / temporary->update.success;
    Serial.printf("%s{%d} count reached! Average value %0.2f\n", measurementTypeStr(type), ch,
                  temporary->avg);

    // This is just for notifying
    int miss = temporary->update.max - temporary->update.success;
    if (miss != 0) {
      Serial.printf("%s{%d} has %d invalid value out of %d update\n", measurementTypeStr(type), ch,
                    miss, temporary->update.max);
    }

    // Resets average related variable calculation
    temporary->sumValues = 0;
    temporary->update.counter = 0;
    temporary->update.success = 0;
    return true;
  }

  return false;
}

int Measurements::get(MeasurementType type, bool average, int ch) {
  // Sanity check to validate channel, assert if invalid
  validateChannel(ch);

  // Follow array indexing just for get address of the value type
  ch = ch - 1;

  // Define data point source
  IntegerValue *temporary = nullptr;
  float invalidValue = 0;
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

  if (average) {
    return temporary->avg;
  }

  return temporary->lastValue;
}

float Measurements::getFloat(MeasurementType type, bool average, int ch) {
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

  if (average) {
    return temporary->avg;
  }

  return temporary->lastValue;
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
    str = "PM25";
    break;
  case PM01:
    str = "PM01";
    break;
  case PM10:
    str = "PM10";
    break;
  case PM03_PC:
    str = "PM03";
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
  if (config.hasSensorS8 && utils::isValidCO2(_co2.avg)) {
    root["rco2"] = _co2.avg;
  }

  /// TVOx and NOx
  if (config.hasSensorSGP) {
    if (utils::isValidVOC(_tvoc.avg)) {
      root["tvocIndex"] = _tvoc.avg;
    }
    if (utils::isValidVOC(_tvoc_raw.avg)) {
      root["tvocRaw"] = _tvoc_raw.avg;
    }
    if (utils::isValidNOx(_nox.avg)) {
      root["noxIndex"] = _nox.avg;
    }
    if (utils::isValidNOx(_nox_raw.avg)) {
      root["noxRaw"] = _nox_raw.avg;
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
  Serial.printf("\n----\n %s \n-----\n", result.c_str());
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
    if (utils::isValidTemperature(_temperature[0].avg)) {
      indoor["atmp"] = ag.round2(_temperature[0].avg);
      if (localServer) {
        indoor["atmpCompensated"] = ag.round2(_temperature[0].avg);
      }
    }
    // Add humidity
    if (utils::isValidHumidity(_humidity[0].avg)) {
      indoor["rhum"] = ag.round2(_humidity[0].avg);
      if (localServer) {
        indoor["rhumCompensated"] = ag.round2(_humidity[0].avg);
      }
    }
  }

  // Add pm25 compensated value only if PM2.5 and humidity value is valid
  if (config.hasSensorPMS1 && utils::isValidPm(_pm_25[0].avg)) {
    if (config.hasSensorSHT && utils::isValidHumidity(_humidity[0].avg)) {
      int pm25 = ag.pms5003.compensate(_pm_25[0].avg, _humidity[0].avg);
      if (utils::isValidPm(pm25)) {
        indoor["pm02Compensated"] = pm25;
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

    if (utils::isValidPm(_pm_01[ch].avg)) {
      pms["pm01"] = _pm_01[ch].avg;
    }
    if (utils::isValidPm(_pm_25[ch].avg)) {
      pms["pm02"] = _pm_25[ch].avg;
    }
    if (utils::isValidPm(_pm_10[ch].avg)) {
      pms["pm10"] = _pm_10[ch].avg;
    }
    if (utils::isValidPm03Count(_pm_03_pc[ch].avg)) {
      pms["pm003Count"] = _pm_03_pc[ch].avg;
    }

    if (withTempHum) {
      float _vc;
      // Set temperature if valid
      if (utils::isValidTemperature(_temperature[ch].avg)) {
        pms["atmp"] = ag.round2(_temperature[ch].avg);
        // Compensate temperature when flag is set
        if (compensate) {
          _vc = ag.pms5003t_1.compensateTemp(_temperature[ch].avg);
          if (utils::isValidTemperature(_vc)) {
            pms["atmpCompensated"] = ag.round2(_vc);
          }
        }
      }
      // Set humidity if valid
      if (utils::isValidHumidity(_humidity[ch].avg)) {
        pms["rhum"] = ag.round2(_humidity[ch].avg);
        // Compensate relative humidity when flag is set
        if (compensate) {
          _vc = ag.pms5003t_1.compensateHum(_humidity[ch].avg);
          if (utils::isValidTemperature(_vc)) {
            pms["rhumCompensated"] = ag.round2(_vc);
          }
        }
      }

      // Add pm25 compensated value only if PM2.5 and humidity value is valid
      if (compensate) {
        if (utils::isValidPm(_pm_25[ch].avg) && utils::isValidHumidity(_humidity[ch].avg)) {
          // Note: the pms5003t object is not matter either for channel 1 or 2, compensate points to
          // the same base function
          int pm25 = ag.pms5003t_1.compensate(_pm_25[ch].avg, _humidity[ch].avg);
          if (utils::isValidPm(pm25)) {
            pms["pm02Compensated"] = pm25;
          }
        }
      }
    }

    // Directly return the json object
    return pms;
  };

  // Handle both channel with average, if one of the channel not valid, use another one
  /// PM01
  if (utils::isValidPm(_pm_01[0].avg) && utils::isValidPm(_pm_01[1].avg)) {
    float avg = (_pm_01[0].avg + _pm_01[1].avg) / 2;
    pms["pm01"] = ag.round2(avg);
    pms["channels"]["1"]["pm01"] = _pm_01[0].avg;
    pms["channels"]["2"]["pm01"] = _pm_01[1].avg;
  } else if (utils::isValidPm(_pm_01[0].avg)) {
    pms["pm01"] = _pm_01[0].avg;
    pms["channels"]["1"]["pm01"] = _pm_01[0].avg;
  } else if (utils::isValidPm(_pm_01[1].avg)) {
    pms["pm01"] = _pm_01[1].avg;
    pms["channels"]["2"]["pm01"] = _pm_01[1].avg;
  }

  /// PM2.5
  if (utils::isValidPm(_pm_25[0].avg) && utils::isValidPm(_pm_25[1].avg)) {
    float avg = (_pm_25[0].avg + _pm_25[1].avg) / 2.0f;
    pms["pm02"] = ag.round2(avg);
    pms["channels"]["1"]["pm02"] = _pm_25[0].avg;
    pms["channels"]["2"]["pm02"] = _pm_25[1].avg;
  } else if (utils::isValidPm(_pm_25[0].avg)) {
    pms["pm02"] = _pm_25[0].avg;
    pms["channels"]["1"]["pm02"] = _pm_25[0].avg;
  } else if (utils::isValidPm(_pm_25[1].avg)) {
    pms["pm02"] = _pm_25[1].avg;
    pms["channels"]["2"]["pm02"] = _pm_25[1].avg;
  }

  /// PM10
  if (utils::isValidPm(_pm_10[0].avg) && utils::isValidPm(_pm_10[1].avg)) {
    float avg = (_pm_10[0].avg + _pm_10[1].avg) / 2.0f;
    pms["pm10"] = ag.round2(avg);
    pms["channels"]["1"]["pm10"] = _pm_10[0].avg;
    pms["channels"]["2"]["pm10"] = _pm_10[1].avg;
  } else if (utils::isValidPm(_pm_10[0].avg)) {
    pms["pm10"] = _pm_10[0].avg;
    pms["channels"]["1"]["pm10"] = _pm_10[0].avg;
  } else if (utils::isValidPm(_pm_10[1].avg)) {
    pms["pm10"] = _pm_10[1].avg;
    pms["channels"]["2"]["pm10"] = _pm_10[1].avg;
  }

  /// PM03 particle count
  if (utils::isValidPm03Count(_pm_03_pc[0].avg) && utils::isValidPm03Count(_pm_03_pc[1].avg)) {
    float avg = (_pm_03_pc[0].avg + _pm_03_pc[1].avg) / 2.0f;
    pms["pm003Count"] = ag.round2(avg);
    pms["channels"]["1"]["pm003Count"] = _pm_03_pc[0].avg;
    pms["channels"]["2"]["pm003Count"] = _pm_03_pc[1].avg;
  } else if (utils::isValidPm(_pm_03_pc[0].avg)) {
    pms["pm003Count"] = _pm_03_pc[0].avg;
    pms["channels"]["1"]["pm003Count"] = _pm_03_pc[0].avg;
  } else if (utils::isValidPm(_pm_03_pc[1].avg)) {
    pms["pm003Count"] = _pm_03_pc[1].avg;
    pms["channels"]["2"]["pm003Count"] = _pm_03_pc[1].avg;
  }

  if (withTempHum) {
    /// Temperature
    if (utils::isValidTemperature(_temperature[0].avg) &&
        utils::isValidTemperature(_temperature[1].avg)) {

      float temperature = (_temperature[0].avg + _temperature[1].avg) / 2.0f;
      pms["atmp"] = ag.round2(temperature);
      pms["channels"]["1"]["atmp"] = ag.round2(_temperature[0].avg);
      pms["channels"]["2"]["atmp"] = ag.round2(_temperature[1].avg);

      if (compensate) {
        // Compensate both temperature channel
        float temp = ag.pms5003t_1.compensateTemp(temperature);
        float temp1 = ag.pms5003t_1.compensateTemp(_temperature[0].avg);
        float temp2 = ag.pms5003t_2.compensateTemp(_temperature[1].avg);
        pms["atmpCompensated"] = ag.round2(temp);
        pms["channels"]["1"]["atmpCompensated"] = ag.round2(temp1);
        pms["channels"]["2"]["atmpCompensated"] = ag.round2(temp2);
      }

    } else if (utils::isValidTemperature(_temperature[0].avg)) {
      pms["atmp"] = ag.round2(_temperature[0].avg);
      pms["channels"]["1"]["atmp"] = ag.round2(_temperature[0].avg);

      if (compensate) {
        // Compensate channel 1
        float temp1 = ag.pms5003t_1.compensateTemp(_temperature[0].avg);
        pms["atmpCompensated"] = ag.round2(temp1);
        pms["channels"]["1"]["atmpCompensated"] = ag.round2(temp1);
      }

    } else if (utils::isValidTemperature(_temperature[1].avg)) {
      pms["atmp"] = ag.round2(_temperature[1].avg);
      pms["channels"]["2"]["atmp"] = ag.round2(_temperature[1].avg);

      if (compensate) {
        // Compensate channel 2
        float temp2 = ag.pms5003t_2.compensateTemp(_temperature[1].avg);
        pms["atmpCompensated"] = ag.round2(temp2);
        pms["channels"]["2"]["atmpCompensated"] = ag.round2(temp2);
      }
    }

    /// Relative humidity
    if (utils::isValidHumidity(_humidity[0].avg) && utils::isValidHumidity(_humidity[1].avg)) {
      float humidity = (_humidity[0].avg + _humidity[1].avg) / 2.0f;
      pms["rhum"] = ag.round2(humidity);
      pms["channels"]["1"]["rhum"] = ag.round2(_humidity[0].avg);
      pms["channels"]["2"]["rhum"] = ag.round2(_humidity[1].avg);

      if (compensate) {
        // Compensate both humidity channel
        float hum = ag.pms5003t_1.compensateHum(humidity);
        float hum1 = ag.pms5003t_1.compensateHum(_humidity[0].avg);
        float hum2 = ag.pms5003t_2.compensateHum(_humidity[1].avg);
        pms["rhumCompensated"] = ag.round2(hum);
        pms["channels"]["1"]["rhumCompensated"] = ag.round2(hum1);
        pms["channels"]["2"]["rhumCompensated"] = ag.round2(hum2);
      }

    } else if (utils::isValidHumidity(_humidity[0].avg)) {
      pms["rhum"] = ag.round2(_humidity[0].avg);
      pms["channels"]["1"]["rhum"] = ag.round2(_humidity[0].avg);

      if (compensate) {
        // Compensate humidity channel 1
        float hum1 = ag.pms5003t_1.compensateHum(_humidity[0].avg);
        pms["rhumCompensated"] = ag.round2(hum1);
        pms["channels"]["1"]["rhumCompensated"] = ag.round2(hum1);
      }

    } else if (utils::isValidHumidity(_humidity[1].avg)) {
      pms["rhum"] = ag.round2(_humidity[1].avg);
      pms["channels"]["2"]["rhum"] = ag.round2(_humidity[1].avg);

      if (compensate) {
        // Compensate humidity channel 2
        float hum2 = ag.pms5003t_2.compensateHum(_humidity[1].avg);
        pms["rhumCompensated"] = ag.round2(hum2);
        pms["channels"]["2"]["rhumCompensated"] = ag.round2(hum2);
      }
    }

    if (compensate) {
      // Add pm25 compensated value
      /// First get both channel compensated value
      int pm25_comp1 = utils::getInvalidPmValue();
      int pm25_comp2 = utils::getInvalidPmValue();
      if (utils::isValidPm(_pm_25[0].avg) && utils::isValidHumidity(_humidity[0].avg)) {
        pm25_comp1 = ag.pms5003t_1.compensate(_pm_25[0].avg, _humidity[0].avg);
        pms["channels"]["1"]["pm02Compensated"] = pm25_comp1;
      }
      if (utils::isValidPm(_pm_25[1].avg) && utils::isValidHumidity(_humidity[1].avg)) {
        pm25_comp2 = ag.pms5003t_2.compensate(_pm_25[1].avg, _humidity[1].avg);
        pms["channels"]["2"]["pm02Compensated"] = pm25_comp2;
      }

      /// Get average or one of the channel compensated value if only one channel is valid
      if (utils::isValidPm(pm25_comp1) && utils::isValidPm(pm25_comp2)) {
        pms["pm02Compensated"] = (int)((pm25_comp1 / pm25_comp2) / 2);
      } else if (utils::isValidPm(pm25_comp1)) {
        pms["pm02Compensated"] = pm25_comp1;
      } else if (utils::isValidPm(pm25_comp2)) {
        pms["pm02Compensated"] = pm25_comp2;
      }
    }
  }

  return pms;
}