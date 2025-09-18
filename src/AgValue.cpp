#include "AgValue.h"
#include "AgConfigure.h"
#include "AirGradient.h"
#include "App/AppDef.h"
#include <cmath>
#include <sstream>

#define json_prop_pmFirmware "firmware"
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

Measurements::Measurements(Configuration &config) : config(config) {
#ifndef ESP8266
  _resetReason = (int)ESP_RST_UNKNOWN;
#endif

  /* Set invalid value for each measurements as default value when initialized*/
  _temperature[0].update.avg = utils::getInvalidTemperature();
  _temperature[1].update.avg = utils::getInvalidTemperature();
  _humidity[0].update.avg = utils::getInvalidHumidity();
  _humidity[1].update.avg = utils::getInvalidHumidity();
  _co2.update.avg = utils::getInvalidCO2();
  _tvoc.update.avg = utils::getInvalidVOC();
  _tvoc_raw.update.avg = utils::getInvalidVOC();
  _nox.update.avg = utils::getInvalidNOx();
  _nox_raw.update.avg = utils::getInvalidNOx();

  _pm_03_pc[0].update.avg = utils::getInvalidPmValue();
  _pm_03_pc[1].update.avg = utils::getInvalidPmValue();
  _pm_05_pc[0].update.avg = utils::getInvalidPmValue();
  _pm_05_pc[1].update.avg = utils::getInvalidPmValue();
  _pm_5_pc[0].update.avg = utils::getInvalidPmValue();
  _pm_5_pc[1].update.avg = utils::getInvalidPmValue();

  _pm_01[0].update.avg = utils::getInvalidPmValue();
  _pm_01_sp[0].update.avg = utils::getInvalidPmValue();
  _pm_01_pc[0].update.avg = utils::getInvalidPmValue();
  _pm_01[1].update.avg = utils::getInvalidPmValue();
  _pm_01_sp[1].update.avg = utils::getInvalidPmValue();
  _pm_01_pc[1].update.avg = utils::getInvalidPmValue();

  _pm_25[0].update.avg = utils::getInvalidPmValue();
  _pm_25_sp[0].update.avg = utils::getInvalidPmValue();
  _pm_25_pc[0].update.avg = utils::getInvalidPmValue();
  _pm_25[1].update.avg = utils::getInvalidPmValue();
  _pm_25_sp[1].update.avg = utils::getInvalidPmValue();
  _pm_25_pc[1].update.avg = utils::getInvalidPmValue();

  _pm_10[0].update.avg = utils::getInvalidPmValue();
  _pm_10_sp[0].update.avg = utils::getInvalidPmValue();
  _pm_10_pc[0].update.avg = utils::getInvalidPmValue();
  _pm_10[1].update.avg = utils::getInvalidPmValue();
  _pm_10_sp[1].update.avg = utils::getInvalidPmValue();
  _pm_10_pc[1].update.avg = utils::getInvalidPmValue();
}

void Measurements::setAirGradient(AirGradient *ag) { this->ag = ag; }

void Measurements::printCurrentAverage() {
  Serial.println();
  if (config.hasSensorS8) {
    if (utils::isValidCO2(_co2.update.avg)) {
      Serial.printf("CO2 = %.2f ppm\n", _co2.update.avg);
    } else {
      Serial.printf("CO2 = -\n");
    }
  }

  if (config.hasSensorSHT) {
    if (utils::isValidTemperature(_temperature[0].update.avg)) {
      Serial.printf("Temperature = %.2f C\n", _temperature[0].update.avg);
    } else {
      Serial.printf("Temperature = -\n");
    }
    if (utils::isValidHumidity(_humidity[0].update.avg)) {
      Serial.printf("Relative Humidity = %.2f\n", _humidity[0].update.avg);
    } else {
      Serial.printf("Relative Humidity = -\n");
    }
  }

  if (config.hasSensorSGP) {
    if (utils::isValidVOC(_tvoc.update.avg)) {
      Serial.printf("TVOC Index = %.1f\n", _tvoc.update.avg);
    } else {
      Serial.printf("TVOC Index = -\n");
    }
    if (utils::isValidVOC(_tvoc_raw.update.avg)) {
      Serial.printf("TVOC Raw = %.1f\n", _tvoc_raw.update.avg);
    } else {
      Serial.printf("TVOC Raw = -\n");
    }
    if (utils::isValidNOx(_nox.update.avg)) {
      Serial.printf("NOx Index = %.1f\n", _nox.update.avg);
    } else {
      Serial.printf("NOx Index = -\n");
    }
    if (utils::isValidNOx(_nox_raw.update.avg)) {
      Serial.printf("NOx Raw = %.1f\n", _nox_raw.update.avg);
    } else {
      Serial.printf("NOx Raw = -\n");
    }
  }

  if (config.hasSensorPMS1) {
    printCurrentPMAverage(1);
    if (!config.hasSensorSHT) {
      if (utils::isValidTemperature(_temperature[0].update.avg)) {
        Serial.printf("[1] Temperature = %.2f C\n", _temperature[0].update.avg);
      } else {
        Serial.printf("[1] Temperature = -\n");
      }
      if (utils::isValidHumidity(_humidity[0].update.avg)) {
        Serial.printf("[1] Relative Humidity = %.2f\n", _humidity[0].update.avg);
      } else {
        Serial.printf("[1] Relative Humidity = -\n");
      }
    }
  }
  if (config.hasSensorPMS2) {
    printCurrentPMAverage(2);
    if (!config.hasSensorSHT) {
      if (utils::isValidTemperature(_temperature[1].update.avg)) {
        Serial.printf("[2] Temperature = %.2f C\n", _temperature[1].update.avg);
      } else {
        Serial.printf("[2] Temperature = -\n");
      }
      if (utils::isValidHumidity(_humidity[1].update.avg)) {
        Serial.printf("[2] Relative Humidity = %.2f\n", _humidity[1].update.avg);
      } else {
        Serial.printf("[2] Relative Humidity = -\n");
      }
    }
  }

  Serial.println();
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

  bool undefined = false;

  // Follow array indexing just for get address of the value type
  ch = ch - 1;

  // Define data point source. Data type doesn't matter because only to get the average value
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
    undefined = true;
    break;
  };

  // Sanity check if measurement type is not defined
  if (undefined) {
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

void Measurements::printCurrentPMAverage(int ch) {
  int idx = ch - 1;

  if (utils::isValidPm(_pm_01[idx].update.avg)) {
    Serial.printf("[%d] Atmospheric PM 1.0 = %.2f ug/m3\n", ch, _pm_01[idx].update.avg);
  } else {
    Serial.printf("[%d] Atmospheric PM 1.0 = -\n", ch);
  }
  if (utils::isValidPm(_pm_25[idx].update.avg)) {
    Serial.printf("[%d] Atmospheric PM 2.5 = %.2f ug/m3\n", ch, _pm_25[idx].update.avg);
  } else {
    Serial.printf("[%d] Atmospheric PM 2.5 = -\n", ch);
  }
  if (utils::isValidPm(_pm_10[idx].update.avg)) {
    Serial.printf("[%d] Atmospheric PM 10 = %.2f ug/m3\n", ch, _pm_10[idx].update.avg);
  } else {
    Serial.printf("[%d] Atmospheric PM 10 = -\n", ch);
  }
  if (utils::isValidPm(_pm_01_sp[idx].update.avg)) {
    Serial.printf("[%d] Standard Particle PM 1.0 = %.2f ug/m3\n", ch, _pm_01_sp[idx].update.avg);
  } else {
    Serial.printf("[%d] Standard Particle PM 1.0 = -\n", ch);
  }
  if (utils::isValidPm(_pm_25_sp[idx].update.avg)) {
    Serial.printf("[%d] Standard Particle PM 2.5 = %.2f ug/m3\n", ch, _pm_25_sp[idx].update.avg);
  } else {
    Serial.printf("[%d] Standard Particle PM 2.5 = -\n", ch);
  }
  if (utils::isValidPm(_pm_10_sp[idx].update.avg)) {
    Serial.printf("[%d] Standard Particle PM 10 = %.2f ug/m3\n", ch, _pm_10_sp[idx].update.avg);
  } else {
    Serial.printf("[%d] Standard Particle PM 10 = -\n", ch);
  }
  if (utils::isValidPm03Count(_pm_03_pc[idx].update.avg)) {
    Serial.printf("[%d] Particle Count 0.3 = %.1f\n", ch, _pm_03_pc[idx].update.avg);
  } else {
    Serial.printf("[%d] Particle Count 0.3 = -\n", ch);
  }
  if (utils::isValidPm03Count(_pm_05_pc[idx].update.avg)) {
    Serial.printf("[%d] Particle Count 0.5 = %.1f\n", ch, _pm_05_pc[idx].update.avg);
  } else {
    Serial.printf("[%d] Particle Count 0.5 = -\n", ch);
  }
  if (utils::isValidPm03Count(_pm_01_pc[idx].update.avg)) {
    Serial.printf("[%d] Particle Count 1.0 = %.1f\n", ch, _pm_01_pc[idx].update.avg);
  } else {
    Serial.printf("[%d] Particle Count 1.0 = -\n", ch);
  }
  if (utils::isValidPm03Count(_pm_25_pc[idx].update.avg)) {
    Serial.printf("[%d] Particle Count 2.5 = %.1f\n", ch, _pm_25_pc[idx].update.avg);
  } else {
    Serial.printf("[%d] Particle Count 2.5 = -\n", ch);
  }

  if (_pm_5_pc[idx].listValues.empty() == false) {
    if (utils::isValidPm03Count(_pm_5_pc[idx].update.avg)) {
      Serial.printf("[%d] Particle Count 5.0 = %.1f\n", ch, _pm_5_pc[idx].update.avg);
    } else {
      Serial.printf("[%d] Particle Count 5.0 = -\n", ch);
    }
  }

  if (_pm_10_pc[idx].listValues.empty() == false) {
    if (utils::isValidPm03Count(_pm_10_pc[idx].update.avg)) {
      Serial.printf("[%d] Particle Count 10 = %.1f\n", ch, _pm_10_pc[idx].update.avg);
    } else {
      Serial.printf("[%d] Particle Count 10 = -\n", ch);
    }
  }
}

void Measurements::validateChannel(int ch) {
  if (ch != 1 && ch != 2) {
    Serial.printf("ERROR! Channel %d is undefined. Only channel 1 or 2 is the optional value!", ch);
    delay(1000);
    assert(0);
  }
}

float Measurements::getCorrectedTempHum(MeasurementType type, int ch, bool forceCorrection) {
  // Sanity check to validate channel, assert if invalid
  validateChannel(ch);

  // Follow array indexing just for get address of the value type
  ch = ch - 1;
  bool undefined = false;

  float rawValue;
  Configuration::TempHumCorrection correction;

  switch (type) {
  case Temperature: {
    rawValue = _temperature[ch].update.avg;
    Configuration::TempHumCorrection tmp = config.getTempCorrection();

    // Apply 'standard' correction if its defined or correction forced
    if (tmp.algorithm == TempHumCorrectionAlgorithm::COR_ALGO_TEMP_HUM_AG_PMS5003T_2024) {
      return ag->pms5003t_1.compensateTemp(rawValue);
    } else if (tmp.algorithm == TempHumCorrectionAlgorithm::COR_ALGO_TEMP_HUM_NONE && forceCorrection) {
      return ag->pms5003t_1.compensateTemp(rawValue);
    }

    correction.algorithm = tmp.algorithm;
    correction.intercept = tmp.intercept;
    correction.scalingFactor = tmp.scalingFactor;
    break;
  }
  case Humidity: {
    rawValue = _humidity[ch].update.avg;
    Configuration::TempHumCorrection tmp = config.getHumCorrection();

    // Apply 'standard' correction if its defined or correction forced
    if (tmp.algorithm == TempHumCorrectionAlgorithm::COR_ALGO_TEMP_HUM_AG_PMS5003T_2024) {
      return ag->pms5003t_1.compensateHum(rawValue);
    } else if (tmp.algorithm == TempHumCorrectionAlgorithm::COR_ALGO_TEMP_HUM_NONE && forceCorrection) {
      return ag->pms5003t_1.compensateHum(rawValue);
    }

    correction.algorithm = tmp.algorithm;
    correction.intercept = tmp.intercept;
    correction.scalingFactor = tmp.scalingFactor;
    break;
  }
  default:
    // Should not be called for other measurements
    delay(1000);
    assert(0);
  }

  // Use raw if correction not defined
  if (correction.algorithm == TempHumCorrectionAlgorithm::COR_ALGO_TEMP_HUM_NONE ||
      correction.algorithm == TempHumCorrectionAlgorithm::COR_ALGO_TEMP_HUM_UNKNOWN) {
    return rawValue;
  }

  // Custom correction constants
  float corrected = (rawValue * correction.scalingFactor) + correction.intercept;
  Serial.println("Custom correction applied");

  return corrected;
}

float Measurements::getCorrectedPM25(bool useAvg, int ch, bool forceCorrection) {
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
  case PMCorrectionAlgorithm::COR_ALGO_PM_UNKNOWN:
  case PMCorrectionAlgorithm::COR_ALGO_PM_NONE: {
    // If correction is Unknown or None, then default is None
    // Unless forceCorrection enabled
    if (forceCorrection) {
      corrected = ag->pms5003.compensate(pm25, humidity);
    } else {
      corrected = pm25;
    }
    break;
  }
  case PMCorrectionAlgorithm::COR_ALGO_PM_EPA_2021:
    corrected = ag->pms5003.compensate(pm25, humidity);
    break;
  default: {
    // All SLR correction using the same flow, hence default condition
    corrected = ag->pms5003.slrCorrection(pm25, pm003Count, pmCorrection.scalingFactor,
                                          pmCorrection.intercept);
    if (pmCorrection.useEPA) {
      // Add EPA compensation on top of SLR
      corrected = ag->pms5003.compensate(corrected, humidity);
    }
  }
  }

  return corrected;
}

Measurements::Measures Measurements::getMeasures() {
  Measures mc;
  mc.bootCount = _bootCount;
  mc.freeHeap = ESP.getFreeHeap();
  // co2, tvoc, nox
  mc.co2 = _co2.update.avg;
  mc.tvoc = _tvoc.update.avg;
  mc.tvoc_raw = _tvoc_raw.update.avg;
  mc.nox = _nox.update.avg;
  mc.nox_raw = _nox_raw.update.avg;
  // Temperature & Humidity
  mc.temperature[0] = _temperature[0].update.avg;
  mc.humidity[0] = _humidity[0].update.avg;
  mc.temperature[1] = _temperature[1].update.avg;
  mc.humidity[1] = _humidity[1].update.avg;
  // PM atmospheric
  mc.pm_01[0] = _pm_01[0].update.avg;
  mc.pm_25[0] = _pm_25[0].update.avg;
  mc.pm_10[0] = _pm_10[0].update.avg;
  mc.pm_01[1] = _pm_01[1].update.avg;
  mc.pm_25[1] = _pm_25[1].update.avg;
  mc.pm_10[1] = _pm_10[1].update.avg;
  // PM standard particle
  mc.pm_01_sp[0] = _pm_01_sp[0].update.avg;
  mc.pm_25_sp[0] = _pm_25_sp[0].update.avg;
  mc.pm_10_sp[0] = _pm_10_sp[0].update.avg;
  mc.pm_01_sp[1] = _pm_01_sp[1].update.avg;
  mc.pm_25_sp[1] = _pm_25_sp[1].update.avg;
  mc.pm_10_sp[1] = _pm_10_sp[1].update.avg;
  // Particle Count
  mc.pm_03_pc[0] = _pm_03_pc[0].update.avg;
  mc.pm_05_pc[0] = _pm_05_pc[0].update.avg;
  mc.pm_01_pc[0] = _pm_01_pc[0].update.avg;
  mc.pm_25_pc[0] = _pm_25_pc[0].update.avg;
  mc.pm_5_pc[0] = _pm_5_pc[0].update.avg;
  mc.pm_10_pc[0] = _pm_10_pc[0].update.avg;
  mc.pm_03_pc[1] = _pm_03_pc[1].update.avg;
  mc.pm_05_pc[1] = _pm_05_pc[1].update.avg;
  mc.pm_01_pc[1] = _pm_01_pc[1].update.avg;
  mc.pm_25_pc[1] = _pm_25_pc[1].update.avg;
  mc.pm_5_pc[1] = _pm_5_pc[1].update.avg;
  mc.pm_10_pc[1] = _pm_10_pc[1].update.avg;

  return mc;
}

std::string Measurements::buildMeasuresPayload(Measures &mc) {
  std::ostringstream oss;

  // CO2
  if (utils::isValidCO2(mc.co2)) {
    oss << std::round(mc.co2);
  }

  oss << ",";

  // Temperature
  if (utils::isValidTemperature(mc.temperature[0]) && utils::isValidTemperature(mc.temperature[1])) {
    float temp = (mc.temperature[0] + mc.temperature[1]) / 2.0f;
    oss << std::round(temp * 10);
  } else if (utils::isValidTemperature(mc.temperature[0])) {
    oss << std::round(mc.temperature[0] * 10);
  } else if (utils::isValidTemperature(mc.temperature[1])) {
    oss << std::round(mc.temperature[1] * 10);
  }

  oss << ",";

  // Humidity
  if (utils::isValidHumidity(mc.humidity[0]) && utils::isValidHumidity(mc.humidity[1])) {
    float hum = (mc.humidity[0] + mc.humidity[1]) / 2.0f;
    oss << std::round(hum * 10);
  } else if (utils::isValidHumidity(mc.humidity[0])) {
    oss << std::round(mc.humidity[0] * 10);
  } else if (utils::isValidHumidity(mc.humidity[1])) {
    oss << std::round(mc.humidity[1] * 10);
  }

  oss << ",";

  /// PM1.0 atmospheric environment
  if (utils::isValidPm(mc.pm_01[0]) && utils::isValidPm(mc.pm_01[1])) {
    float pm01 = (mc.pm_01[0] + mc.pm_01[1]) / 2.0f;
    oss << std::round(pm01 * 10);
  } else if (utils::isValidPm(mc.pm_01[0])) {
    oss << std::round(mc.pm_01[0] * 10);
  } else if (utils::isValidPm(mc.pm_01[1])) {
    oss << std::round(mc.pm_01[1] * 10);
  }

  oss << ",";

  /// PM2.5 atmospheric environment
  if (utils::isValidPm(mc.pm_25[0]) && utils::isValidPm(mc.pm_25[1])) {
    float pm25 = (mc.pm_25[0] + mc.pm_25[1]) / 2.0f;
    oss << std::round(pm25 * 10);
  } else if (utils::isValidPm(mc.pm_25[0])) {
    oss << std::round(mc.pm_25[0] * 10);
  } else if (utils::isValidPm(mc.pm_25[1])) {
    oss << std::round(mc.pm_25[1] * 10);
  }

  oss << ",";

  /// PM10 atmospheric environment
  if (utils::isValidPm(mc.pm_10[0]) && utils::isValidPm(mc.pm_10[1])) {
    float pm10 = (mc.pm_10[0] + mc.pm_10[1]) / 2.0f;
    oss << std::round(pm10 * 10);
  } else if (utils::isValidPm(mc.pm_10[0])) {
    oss << std::round(mc.pm_10[0] * 10);
  } else if (utils::isValidPm(mc.pm_10[1])) {
    oss << std::round(mc.pm_10[1] * 10);
  }

  oss << ",";

  // TVOC
  if (utils::isValidVOC(mc.tvoc)) {
    oss << std::round(mc.tvoc);
  }

  oss << ",";

  // NOx
  if (utils::isValidNOx(mc.nox)) {
    oss << std::round(mc.nox);
  }

  oss << ",";

  /// PM 0.3 particle count
  if (utils::isValidPm03Count(mc.pm_03_pc[0]) && utils::isValidPm03Count(mc.pm_03_pc[1])) {
    oss << std::round((mc.pm_03_pc[0] + mc.pm_03_pc[1]) / 2.0f);
  } else if (utils::isValidPm03Count(mc.pm_03_pc[0])) {
    oss << std::round(mc.pm_03_pc[0]);
  } else if (utils::isValidPm03Count(mc.pm_03_pc[1])) {
    oss << std::round(mc.pm_03_pc[1]);
  }

  oss << ",";

  if (mc.signal < 0) {
    oss << mc.signal;
  }

  return oss.str();
}


String Measurements::toString(bool localServer, AgFirmwareMode fwMode, int rssi) {
  JSONVar root;

  if (ag->isOne() || (ag->isPro4_2()) || ag->isPro3_3() || ag->isBasic()) {
    root = buildIndoor(localServer);
  } else {
    root = buildOutdoor(localServer, fwMode);
  }

  // CO2
  if (config.hasSensorS8 && utils::isValidCO2(_co2.update.avg)) {
    root[json_prop_co2] = ag->round2(_co2.update.avg);
  }

  /// TVOx and NOx
  if (config.hasSensorSGP) {
    if (utils::isValidVOC(_tvoc.update.avg)) {
      root[json_prop_tvoc] = ag->round2(_tvoc.update.avg);
    }
    if (utils::isValidVOC(_tvoc_raw.update.avg)) {
      root[json_prop_tvocRaw] = ag->round2(_tvoc_raw.update.avg);
    }
    if (utils::isValidNOx(_nox.update.avg)) {
      root[json_prop_nox] = ag->round2(_nox.update.avg);
    }
    if (utils::isValidNOx(_nox_raw.update.avg)) {
      root[json_prop_noxRaw] = ag->round2(_nox_raw.update.avg);
    }
  }

  root["boot"] = _bootCount;
  root["bootCount"] = _bootCount;
  root["wifi"] = rssi;

  if (localServer) {
    if (ag->isOne()) {
      root["ledMode"] = config.getLedBarModeName();
    }
    root["serialno"] = ag->deviceId();
    root["firmware"] = ag->getVersion();
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

JSONVar Measurements::buildOutdoor(bool localServer, AgFirmwareMode fwMode) {
  JSONVar outdoor;
  if (fwMode == FW_MODE_O_1P || fwMode == FW_MODE_O_1PS || fwMode == FW_MODE_O_1PST) {
    // buildPMS params:
    /// Because only have 1 PMS, allCh is set to false
    /// But enable temp hum from PMS
    /// compensated values if requested by local server
    /// Set ch based on hasSensorPMSx
    if (config.hasSensorPMS1) {
      outdoor = buildPMS(1, false, true, localServer);
      if (!localServer) {
        outdoor[json_prop_pmFirmware] =
            pms5003TFirmwareVersion(ag->pms5003t_1.getFirmwareVersion());
      }
    } else {
      outdoor = buildPMS(2, false, true, localServer);
      if (!localServer) {
        outdoor[json_prop_pmFirmware] =
            pms5003TFirmwareVersion(ag->pms5003t_2.getFirmwareVersion());
      }
    }
  } else {
    // FW_MODE_O_1PPT && FW_MODE_O_1PP: Outdoor monitor that have 2 PMS sensor
    // buildPMS params:
    /// Have 2 PMS sensor, allCh is set to true (ch params ignored)
    /// Enable temp hum from PMS
    /// compensated values if requested by local server
    outdoor = buildPMS(1, true, true, localServer);
    // PMS5003T version
    if (!localServer) {
      outdoor["channels"]["1"][json_prop_pmFirmware] =
          pms5003TFirmwareVersion(ag->pms5003t_1.getFirmwareVersion());
      outdoor["channels"]["2"][json_prop_pmFirmware] =
          pms5003TFirmwareVersion(ag->pms5003t_2.getFirmwareVersion());
    }
  }

  return outdoor;
}

JSONVar Measurements::buildIndoor(bool localServer) {
  JSONVar indoor;

  if (config.hasSensorPMS1) {
    // buildPMS params:
    /// PMS channel 1 (indoor only have 1 PMS; hence allCh false)
    /// Not include temperature and humidity from PMS sensor
    /// Include compensated calculation
    indoor = buildPMS(1, false, false, true);
    if (!localServer) {
      // Indoor is using PMS5003
      indoor[json_prop_pmFirmware] = this->pms5003FirmwareVersion(ag->pms5003.getFirmwareVersion());
    }
  }

  if (config.hasSensorSHT) {
    // Add temperature
    if (utils::isValidTemperature(_temperature[0].update.avg)) {
      indoor[json_prop_temp] = ag->round2(_temperature[0].update.avg);
      if (localServer) {
        indoor[json_prop_tempCompensated] = ag->round2(getCorrectedTempHum(Temperature));
      }
    }
    // Add humidity
    if (utils::isValidHumidity(_humidity[0].update.avg)) {
      indoor[json_prop_rhum] = ag->round2(_humidity[0].update.avg);
      if (localServer) {
        indoor[json_prop_rhumCompensated] = ag->round2(getCorrectedTempHum(Humidity));
      }
    }
  }

  return indoor;
}

JSONVar Measurements::buildPMS(int ch, bool allCh, bool withTempHum, bool compensate) {
  JSONVar pms;

  // When only one of the channel
  if (allCh == false) {
    // Sanity check to validate channel, assert if invalid
    validateChannel(ch);

    // Follow array indexing just for get address of the value type
    int chIndex = ch - 1;

    if (utils::isValidPm(_pm_01[chIndex].update.avg)) {
      pms[json_prop_pm01Ae] = ag->round2(_pm_01[chIndex].update.avg);
    }
    if (utils::isValidPm(_pm_25[chIndex].update.avg)) {
      pms[json_prop_pm25Ae] = ag->round2(_pm_25[chIndex].update.avg);
    }
    if (utils::isValidPm(_pm_10[chIndex].update.avg)) {
      pms[json_prop_pm10Ae] = ag->round2(_pm_10[chIndex].update.avg);
    }
    if (utils::isValidPm(_pm_01_sp[chIndex].update.avg)) {
      pms[json_prop_pm01Sp] = ag->round2(_pm_01_sp[chIndex].update.avg);
    }
    if (utils::isValidPm(_pm_25_sp[chIndex].update.avg)) {
      pms[json_prop_pm25Sp] = ag->round2(_pm_25_sp[chIndex].update.avg);
    }
    if (utils::isValidPm(_pm_10_sp[chIndex].update.avg)) {
      pms[json_prop_pm10Sp] = ag->round2(_pm_10_sp[chIndex].update.avg);
    }
    if (utils::isValidPm03Count(_pm_03_pc[chIndex].update.avg)) {
      pms[json_prop_pm03Count] = ag->round2(_pm_03_pc[chIndex].update.avg);
    }
    if (utils::isValidPm03Count(_pm_05_pc[chIndex].update.avg)) {
      pms[json_prop_pm05Count] = ag->round2(_pm_05_pc[chIndex].update.avg);
    }
    if (utils::isValidPm03Count(_pm_01_pc[chIndex].update.avg)) {
      pms[json_prop_pm1Count] = ag->round2(_pm_01_pc[chIndex].update.avg);
    }
    if (utils::isValidPm03Count(_pm_25_pc[chIndex].update.avg)) {
      pms[json_prop_pm25Count] = ag->round2(_pm_25_pc[chIndex].update.avg);
    }
    if (_pm_5_pc[chIndex].listValues.empty() == false) {
      // Only include pm5.0 count when values available on its list
      // If not, means no pm5_pc available from the sensor
      if (utils::isValidPm03Count(_pm_5_pc[chIndex].update.avg)) {
        pms[json_prop_pm5Count] = ag->round2(_pm_5_pc[chIndex].update.avg);
      }
    }
    if (_pm_10_pc[chIndex].listValues.empty() == false) {
      // Only include pm10 count when values available on its list
      // If not, means no pm10_pc available from the sensor
      if (utils::isValidPm03Count(_pm_10_pc[chIndex].update.avg)) {
        pms[json_prop_pm10Count] = ag->round2(_pm_10_pc[chIndex].update.avg);
      }
    }

    if (withTempHum) {
      float _vc;
      // Set temperature if valid
      if (utils::isValidTemperature(_temperature[chIndex].update.avg)) {
        pms[json_prop_temp] = ag->round2(_temperature[chIndex].update.avg);
        // Compensate temperature when flag is set
        if (compensate) {
          _vc = getCorrectedTempHum(Temperature, ch, true);
          if (utils::isValidTemperature(_vc)) {
            pms[json_prop_tempCompensated] = ag->round2(_vc);
          }
        }
      }
      // Set humidity if valid
      if (utils::isValidHumidity(_humidity[chIndex].update.avg)) {
        pms[json_prop_rhum] = ag->round2(_humidity[chIndex].update.avg);
        // Compensate relative humidity when flag is set
        if (compensate) {
          _vc = getCorrectedTempHum(Humidity, ch, true);
          if (utils::isValidHumidity(_vc)) {
            pms[json_prop_rhumCompensated] = ag->round2(_vc);
          }
        }
      }

    }

    // Add pm25 compensated value only if PM2.5 and humidity value is valid
    if (compensate) {
      if (utils::isValidPm(_pm_25[chIndex].update.avg) &&
          utils::isValidHumidity(_humidity[chIndex].update.avg)) {
        float pm25 = getCorrectedPM25(true, ch, true);
        pms[json_prop_pm25Compensated] = ag->round2(pm25);
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
    pms[json_prop_pm01Ae] = ag->round2(avg);
    pms["channels"]["1"][json_prop_pm01Ae] = ag->round2(_pm_01[0].update.avg);
    pms["channels"]["2"][json_prop_pm01Ae] = ag->round2(_pm_01[1].update.avg);
  } else if (utils::isValidPm(_pm_01[0].update.avg)) {
    pms[json_prop_pm01Ae] = ag->round2(_pm_01[0].update.avg);
    pms["channels"]["1"][json_prop_pm01Ae] = ag->round2(_pm_01[0].update.avg);
  } else if (utils::isValidPm(_pm_01[1].update.avg)) {
    pms[json_prop_pm01Ae] = ag->round2(_pm_01[1].update.avg);
    pms["channels"]["2"][json_prop_pm01Ae] = ag->round2(_pm_01[1].update.avg);
  }

  /// PM2.5 atmospheric environment
  if (utils::isValidPm(_pm_25[0].update.avg) && utils::isValidPm(_pm_25[1].update.avg)) {
    float avg = (_pm_25[0].update.avg + _pm_25[1].update.avg) / 2.0f;
    pms[json_prop_pm25Ae] = ag->round2(avg);
    pms["channels"]["1"][json_prop_pm25Ae] = ag->round2(_pm_25[0].update.avg);
    pms["channels"]["2"][json_prop_pm25Ae] = ag->round2(_pm_25[1].update.avg);
  } else if (utils::isValidPm(_pm_25[0].update.avg)) {
    pms[json_prop_pm25Ae] = ag->round2(_pm_25[0].update.avg);
    pms["channels"]["1"][json_prop_pm25Ae] = ag->round2(_pm_25[0].update.avg);
  } else if (utils::isValidPm(_pm_25[1].update.avg)) {
    pms[json_prop_pm25Ae] = ag->round2(_pm_25[1].update.avg);
    pms["channels"]["2"][json_prop_pm25Ae] = ag->round2(_pm_25[1].update.avg);
  }

  /// PM10 atmospheric environment
  if (utils::isValidPm(_pm_10[0].update.avg) && utils::isValidPm(_pm_10[1].update.avg)) {
    float avg = (_pm_10[0].update.avg + _pm_10[1].update.avg) / 2.0f;
    pms[json_prop_pm10Ae] = ag->round2(avg);
    pms["channels"]["1"][json_prop_pm10Ae] = ag->round2(_pm_10[0].update.avg);
    pms["channels"]["2"][json_prop_pm10Ae] = ag->round2(_pm_10[1].update.avg);
  } else if (utils::isValidPm(_pm_10[0].update.avg)) {
    pms[json_prop_pm10Ae] = ag->round2(_pm_10[0].update.avg);
    pms["channels"]["1"][json_prop_pm10Ae] = ag->round2(_pm_10[0].update.avg);
  } else if (utils::isValidPm(_pm_10[1].update.avg)) {
    pms[json_prop_pm10Ae] = ag->round2(_pm_10[1].update.avg);
    pms["channels"]["2"][json_prop_pm10Ae] = ag->round2(_pm_10[1].update.avg);
  }

  /// PM1.0 standard particle
  if (utils::isValidPm(_pm_01_sp[0].update.avg) && utils::isValidPm(_pm_01_sp[1].update.avg)) {
    float avg = (_pm_01_sp[0].update.avg + _pm_01_sp[1].update.avg) / 2.0f;
    pms[json_prop_pm01Sp] = ag->round2(avg);
    pms["channels"]["1"][json_prop_pm01Sp] = ag->round2(_pm_01_sp[0].update.avg);
    pms["channels"]["2"][json_prop_pm01Sp] = ag->round2(_pm_01_sp[1].update.avg);
  } else if (utils::isValidPm(_pm_01_sp[0].update.avg)) {
    pms[json_prop_pm01Sp] = ag->round2(_pm_01_sp[0].update.avg);
    pms["channels"]["1"][json_prop_pm01Sp] = ag->round2(_pm_01_sp[0].update.avg);
  } else if (utils::isValidPm(_pm_01_sp[1].update.avg)) {
    pms[json_prop_pm01Sp] = ag->round2(_pm_01_sp[1].update.avg);
    pms["channels"]["2"][json_prop_pm01Sp] = ag->round2(_pm_01_sp[1].update.avg);
  }

  /// PM2.5 standard particle
  if (utils::isValidPm(_pm_25_sp[0].update.avg) && utils::isValidPm(_pm_25_sp[1].update.avg)) {
    float avg = (_pm_25_sp[0].update.avg + _pm_25_sp[1].update.avg) / 2.0f;
    pms[json_prop_pm25Sp] = ag->round2(avg);
    pms["channels"]["1"][json_prop_pm25Sp] = ag->round2(_pm_25_sp[0].update.avg);
    pms["channels"]["2"][json_prop_pm25Sp] = ag->round2(_pm_25_sp[1].update.avg);
  } else if (utils::isValidPm(_pm_25_sp[0].update.avg)) {
    pms[json_prop_pm25Sp] = ag->round2(_pm_25_sp[0].update.avg);
    pms["channels"]["1"][json_prop_pm25Sp] = ag->round2(_pm_25_sp[0].update.avg);
  } else if (utils::isValidPm(_pm_25_sp[1].update.avg)) {
    pms[json_prop_pm25Sp] = ag->round2(_pm_25_sp[1].update.avg);
    pms["channels"]["2"][json_prop_pm25Sp] = ag->round2(_pm_25_sp[1].update.avg);
  }

  /// PM10 standard particle
  if (utils::isValidPm(_pm_10_sp[0].update.avg) && utils::isValidPm(_pm_10_sp[1].update.avg)) {
    float avg = (_pm_10_sp[0].update.avg + _pm_10_sp[1].update.avg) / 2.0f;
    pms[json_prop_pm10Sp] = ag->round2(avg);
    pms["channels"]["1"][json_prop_pm10Sp] = ag->round2(_pm_10_sp[0].update.avg);
    pms["channels"]["2"][json_prop_pm10Sp] = ag->round2(_pm_10_sp[1].update.avg);
  } else if (utils::isValidPm(_pm_10_sp[0].update.avg)) {
    pms[json_prop_pm10Sp] = ag->round2(_pm_10_sp[0].update.avg);
    pms["channels"]["1"][json_prop_pm10Sp] = ag->round2(_pm_10_sp[0].update.avg);
  } else if (utils::isValidPm(_pm_10_sp[1].update.avg)) {
    pms[json_prop_pm10Sp] = ag->round2(_pm_10_sp[1].update.avg);
    pms["channels"]["2"][json_prop_pm10Sp] = ag->round2(_pm_10_sp[1].update.avg);
  }

  /// PM003 particle count
  if (utils::isValidPm03Count(_pm_03_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_03_pc[1].update.avg)) {
    float avg = (_pm_03_pc[0].update.avg + _pm_03_pc[1].update.avg) / 2.0f;
    pms[json_prop_pm03Count] = ag->round2(avg);
    pms["channels"]["1"][json_prop_pm03Count] = ag->round2(_pm_03_pc[0].update.avg);
    pms["channels"]["2"][json_prop_pm03Count] = ag->round2(_pm_03_pc[1].update.avg);
  } else if (utils::isValidPm03Count(_pm_03_pc[0].update.avg)) {
    pms[json_prop_pm03Count] = ag->round2(_pm_03_pc[0].update.avg);
    pms["channels"]["1"][json_prop_pm03Count] = ag->round2(_pm_03_pc[0].update.avg);
  } else if (utils::isValidPm03Count(_pm_03_pc[1].update.avg)) {
    pms[json_prop_pm03Count] = ag->round2(_pm_03_pc[1].update.avg);
    pms["channels"]["2"][json_prop_pm03Count] = ag->round2(_pm_03_pc[1].update.avg);
  }

  /// PM0.5 particle count
  if (utils::isValidPm03Count(_pm_05_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_05_pc[1].update.avg)) {
    float avg = (_pm_05_pc[0].update.avg + _pm_05_pc[1].update.avg) / 2.0f;
    pms[json_prop_pm05Count] = ag->round2(avg);
    pms["channels"]["1"][json_prop_pm05Count] = ag->round2(_pm_05_pc[0].update.avg);
    pms["channels"]["2"][json_prop_pm05Count] = ag->round2(_pm_05_pc[1].update.avg);
  } else if (utils::isValidPm03Count(_pm_05_pc[0].update.avg)) {
    pms[json_prop_pm05Count] = ag->round2(_pm_05_pc[0].update.avg);
    pms["channels"]["1"][json_prop_pm05Count] = ag->round2(_pm_05_pc[0].update.avg);
  } else if (utils::isValidPm03Count(_pm_05_pc[1].update.avg)) {
    pms[json_prop_pm05Count] = ag->round2(_pm_05_pc[1].update.avg);
    pms["channels"]["2"][json_prop_pm05Count] = ag->round2(_pm_05_pc[1].update.avg);
  }
  /// PM1.0 particle count
  if (utils::isValidPm03Count(_pm_01_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_01_pc[1].update.avg)) {
    float avg = (_pm_01_pc[0].update.avg + _pm_01_pc[1].update.avg) / 2.0f;
    pms[json_prop_pm1Count] = ag->round2(avg);
    pms["channels"]["1"][json_prop_pm1Count] = ag->round2(_pm_01_pc[0].update.avg);
    pms["channels"]["2"][json_prop_pm1Count] = ag->round2(_pm_01_pc[1].update.avg);
  } else if (utils::isValidPm03Count(_pm_01_pc[0].update.avg)) {
    pms[json_prop_pm1Count] = ag->round2(_pm_01_pc[0].update.avg);
    pms["channels"]["1"][json_prop_pm1Count] = ag->round2(_pm_01_pc[0].update.avg);
  } else if (utils::isValidPm03Count(_pm_01_pc[1].update.avg)) {
    pms[json_prop_pm1Count] = ag->round2(_pm_01_pc[1].update.avg);
    pms["channels"]["2"][json_prop_pm1Count] = ag->round2(_pm_01_pc[1].update.avg);
  }

  /// PM2.5 particle count
  if (utils::isValidPm03Count(_pm_25_pc[0].update.avg) &&
      utils::isValidPm03Count(_pm_25_pc[1].update.avg)) {
    float avg = (_pm_25_pc[0].update.avg + _pm_25_pc[1].update.avg) / 2.0f;
    pms[json_prop_pm25Count] = ag->round2(avg);
    pms["channels"]["1"][json_prop_pm25Count] = ag->round2(_pm_25_pc[0].update.avg);
    pms["channels"]["2"][json_prop_pm25Count] = ag->round2(_pm_25_pc[1].update.avg);
  } else if (utils::isValidPm03Count(_pm_25_pc[0].update.avg)) {
    pms[json_prop_pm25Count] = ag->round2(_pm_25_pc[0].update.avg);
    pms["channels"]["1"][json_prop_pm25Count] = ag->round2(_pm_25_pc[0].update.avg);
  } else if (utils::isValidPm03Count(_pm_25_pc[1].update.avg)) {
    pms[json_prop_pm25Count] = ag->round2(_pm_25_pc[1].update.avg);
    pms["channels"]["2"][json_prop_pm25Count] = ag->round2(_pm_25_pc[1].update.avg);
  }

  // NOTE: No need for particle count 5.0 and 10. When allCh is true, basically monitor using
  // PM5003T, which don't have PC 5.0 and 10

  if (withTempHum) {
    /// Temperature
    if (utils::isValidTemperature(_temperature[0].update.avg) &&
        utils::isValidTemperature(_temperature[1].update.avg)) {

      float temperature = (_temperature[0].update.avg + _temperature[1].update.avg) / 2.0f;
      pms[json_prop_temp] = ag->round2(temperature);
      pms["channels"]["1"][json_prop_temp] = ag->round2(_temperature[0].update.avg);
      pms["channels"]["2"][json_prop_temp] = ag->round2(_temperature[1].update.avg);

      if (compensate) {
        // Compensate both temperature channel
        float temp1 = getCorrectedTempHum(Temperature, 1, true);
        float temp2 = getCorrectedTempHum(Temperature, 2, true);
        float tempAverage = (temp1 + temp2) / 2.0f;
        pms[json_prop_tempCompensated] = ag->round2(tempAverage);
        pms["channels"]["1"][json_prop_tempCompensated] = ag->round2(temp1);
        pms["channels"]["2"][json_prop_tempCompensated] = ag->round2(temp2);
      }

    } else if (utils::isValidTemperature(_temperature[0].update.avg)) {
      pms[json_prop_temp] = ag->round2(_temperature[0].update.avg);
      pms["channels"]["1"][json_prop_temp] = ag->round2(_temperature[0].update.avg);

      if (compensate) {
        // Compensate channel 1
        float temp1 = getCorrectedTempHum(Temperature, 1, true);
        pms[json_prop_tempCompensated] = ag->round2(temp1);
        pms["channels"]["1"][json_prop_tempCompensated] = ag->round2(temp1);
      }

    } else if (utils::isValidTemperature(_temperature[1].update.avg)) {
      pms[json_prop_temp] = ag->round2(_temperature[1].update.avg);
      pms["channels"]["2"][json_prop_temp] = ag->round2(_temperature[1].update.avg);

      if (compensate) {
        // Compensate channel 2
        float temp2 = getCorrectedTempHum(Temperature, 2, true);
        pms[json_prop_tempCompensated] = ag->round2(temp2);
        pms["channels"]["2"][json_prop_tempCompensated] = ag->round2(temp2);
      }
    }

    /// Relative humidity
    if (utils::isValidHumidity(_humidity[0].update.avg) &&
        utils::isValidHumidity(_humidity[1].update.avg)) {
      float humidity = (_humidity[0].update.avg + _humidity[1].update.avg) / 2.0f;
      pms[json_prop_rhum] = ag->round2(humidity);
      pms["channels"]["1"][json_prop_rhum] = ag->round2(_humidity[0].update.avg);
      pms["channels"]["2"][json_prop_rhum] = ag->round2(_humidity[1].update.avg);

      if (compensate) {
        // Compensate both humidity channel
        float hum1 = getCorrectedTempHum(Humidity, 1, true);
        float hum2 = getCorrectedTempHum(Humidity, 2, true);
        float humAverage = (hum1 + hum2) / 2.0f;
        pms[json_prop_rhumCompensated] = ag->round2(humAverage);
        pms["channels"]["1"][json_prop_rhumCompensated] = ag->round2(hum1);
        pms["channels"]["2"][json_prop_rhumCompensated] = ag->round2(hum2);
      }

    } else if (utils::isValidHumidity(_humidity[0].update.avg)) {
      pms[json_prop_rhum] = ag->round2(_humidity[0].update.avg);
      pms["channels"]["1"][json_prop_rhum] = ag->round2(_humidity[0].update.avg);

      if (compensate) {
        // Compensate humidity channel 1
        float hum1 = getCorrectedTempHum(Humidity, 1, true);
        pms[json_prop_rhumCompensated] = ag->round2(hum1);
        pms["channels"]["1"][json_prop_rhumCompensated] = ag->round2(hum1);
      }

    } else if (utils::isValidHumidity(_humidity[1].update.avg)) {
      pms[json_prop_rhum] = ag->round2(_humidity[1].update.avg);
      pms["channels"]["2"][json_prop_rhum] = ag->round2(_humidity[1].update.avg);

      if (compensate) {
        // Compensate humidity channel 2
        float hum2 = getCorrectedTempHum(Humidity, 2, true);
        pms[json_prop_rhumCompensated] = ag->round2(hum2);
        pms["channels"]["2"][json_prop_rhumCompensated] = ag->round2(hum2);
      }
    }

    if (compensate) {
      // Add pm25 compensated value
      /// First get both channel compensated value
      float pm25_comp1 = utils::getInvalidPmValue();
      float pm25_comp2 = utils::getInvalidPmValue();
      if (utils::isValidPm(_pm_25[0].update.avg) &&
          utils::isValidHumidity(_humidity[0].update.avg)) {
        pm25_comp1 = getCorrectedPM25(true, 1, true);
        pms["channels"]["1"][json_prop_pm25Compensated] = ag->round2(pm25_comp1);
      }
      if (utils::isValidPm(_pm_25[1].update.avg) &&
          utils::isValidHumidity(_humidity[1].update.avg)) {
        pm25_comp2 = getCorrectedPM25(true, 2, true);
        pms["channels"]["2"][json_prop_pm25Compensated] = ag->round2(pm25_comp2);
      }

      /// Get average or one of the channel compensated value if only one channel is valid
      if (utils::isValidPm(pm25_comp1) && utils::isValidPm(pm25_comp2)) {
        pms[json_prop_pm25Compensated] = ag->round2((pm25_comp1 + pm25_comp2) / 2.0f);
      } else if (utils::isValidPm(pm25_comp1)) {
        pms[json_prop_pm25Compensated] = ag->round2(pm25_comp1);
      } else if (utils::isValidPm(pm25_comp2)) {
        pms[json_prop_pm25Compensated] = ag->round2(pm25_comp2);
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
