#include "AgValue.h"
#include "AgConfigure.h"
#include "AirGradient.h"
#include "Main/utils.h"
#include "Libraries/Arduino_JSON/src/Arduino_JSON.h"

#define json_prop_pmFirmware     "firmware"

/**
 * @brief Get PMS5003 firmware version string
 * 
 * @param fwCode 
 * @return String
 */
String Measurements::pms5003FirmwareVersion(int fwCode) {
  return pms5003FirmwareVersionBase("PMS5003x", fwCode);
}

/**
 * @brief Get PMS5003T firmware version string
 * 
 * @param fwCode 
 * @return String
 */
String Measurements::pms5003TFirmwareVersion(int fwCode) {
  return pms5003FirmwareVersionBase("PMS5003x", fwCode);
}

/**
 * @brief Get firmware version string
 * 
 * @param prefix Prefix firmware string
 * @param fwCode Version code
 * @return string 
 */
String Measurements::pms5003FirmwareVersionBase(String prefix, int fwCode) {
  return prefix + String("-") + String(fwCode);
}

String Measurements::toString(bool localServer, AgFirmwareMode fwMode, int rssi,
                              void *_ag, void *_config) {
  AirGradient *ag = (AirGradient *)_ag;
  Configuration *config = (Configuration *)_config;

  JSONVar root;
  root["wifi"] = rssi;
  if (localServer) {
    root["serialno"] = ag->deviceId();
  }

  if (config->hasSensorS8 && utils::isValidCO2(this->CO2)) {
    root["rco2"] = this->CO2;
  }

  if (ag->isOne() || (ag->isPro4_2()) || ag->isPro3_3() || ag->isBasic()) {
    if (config->hasSensorPMS1) {
      if (utils::isValidPm(this->pm01_1)) {
        root["pm01"] = this->pm01_1;
      }
      if (utils::isValidPm(this->pm25_1)) {
        root["pm02"] = this->pm25_1;
      }
      if (utils::isValidPm(this->pm10_1)) {
        root["pm10"] = this->pm10_1;
      }
      if (utils::isValidPm03Count(this->pm03PCount_1)) {
        root["pm003Count"] = this->pm03PCount_1;
      }
      if (!localServer) {

        root[json_prop_pmFirmware] =
            this->pms5003FirmwareVersion(ag->pms5003.getFirmwareVersion());
      }
    }

    if (config->hasSensorSHT) {
      if (utils::isValidTemperature(this->Temperature)) {
        root["atmp"] = ag->round2(this->Temperature);
        if (localServer) {
          root["atmpCompensated"] = ag->round2(this->Temperature);
        }
      }
      if (utils::isValidHumidity(this->Humidity)) {
        root["rhum"] = this->Humidity;
        if (localServer) {
          root["rhumCompensated"] = this->Humidity;
        }
      }
    }

    if (config->hasSensorSHT && config->hasSensorPMS1) {
      int pm25 = ag->pms5003.compensate(this->pm25_1, this->Humidity);
      if (pm25 >= 0) {
        root["pm02Compensated"] = pm25;
      }
    }

  } else {
    if (config->hasSensorPMS1 && config->hasSensorPMS2) {
      if (utils::isValidPm(this->pm01_1) && utils::isValidPm(this->pm01_2)) {
        root["pm01"] = ag->round2((this->pm01_1 + this->pm01_2) / 2.0f);
      }
      if (utils::isValidPm(this->pm25_1) && utils::isValidPm(this->pm25_2)) {
        root["pm02"] = ag->round2((this->pm25_1 + this->pm25_2) / 2.0f);
      }
      if (utils::isValidPm(this->pm10_1) && utils::isValidPm(this->pm10_2)) {
        root["pm10"] = ag->round2((this->pm10_1 + this->pm10_2) / 2.0f);
      }
      if (utils::isValidPm(this->pm03PCount_1) && utils::isValidPm(this->pm03PCount_2)) {
        root["pm003Count"] = ag->round2((this->pm03PCount_1 + this->pm03PCount_2) / 2.0f);
      }

      float val;
      if (utils::isValidTemperature(this->temp_1) && utils::isValidTemperature(this->temp_1)) {
        root["atmp"] = ag->round2((this->temp_1 + this->temp_2) / 2.0f);
        if (localServer) {
          val = ag->pms5003t_2.compensateTemp((this->temp_1 + this->temp_2) / 2.0f);
          if (utils::isValidTemperature(val)) {
            root["atmpCompensated"] = ag->round2(val);
          }
        }
      }
      if (utils::isValidHumidity(this->hum_1) && utils::isValidHumidity(this->hum_1)) {
        root["rhum"] = ag->round2((this->hum_1 + this->hum_2) / 2.0f);
        if (localServer) {
          val = ag->pms5003t_2.compensateHum((this->hum_1 + this->hum_2) / 2.0f);
          if (utils::isValidHumidity(val)) {
            root["rhumCompensated"] = (int)val;
          }
        }
      }

      int pm25 = (ag->pms5003t_1.compensate(this->pm25_1, this->temp_1) +
                  ag->pms5003t_2.compensate(this->pm25_2, this->temp_2)) /
                 2;
      root["pm02Compensated"] = pm25;
    }

    if (fwMode == FW_MODE_O_1PS || fwMode == FW_MODE_O_1PST) {
      float val;
      if (config->hasSensorPMS1) {
        if (utils::isValidPm(this->pm01_1)) {
          root["pm01"] = this->pm01_1;
        }
        if (utils::isValidPm(this->pm25_1)) {
          root["pm02"] = this->pm25_1;
        }
        if (utils::isValidPm(this->pm10_1)) {
          root["pm10"] = this->pm10_1;
        }
        if (utils::isValidPm03Count(this->pm03PCount_1)) {
          root["pm003Count"] = this->pm03PCount_1;
        }
        if (utils::isValidTemperature(this->temp_1)) {
          root["atmp"] = ag->round2(this->temp_1);

          if (localServer) {
            val = ag->pms5003t_1.compensateTemp(this->temp_1);
            if (utils::isValidTemperature(val)) {
              root["atmpCompensated"] = ag->round2(val);
            }
          }
        }
        if (utils::isValidHumidity(this->hum_1)) {
          root["rhum"] = this->hum_1;

          if (localServer) {
            val = ag->pms5003t_1.compensateHum(this->hum_1);
            if (utils::isValidHumidity(val)) {
              root["rhumCompensated"] = (int)val;
            }
          }
        }
        root["pm02Compensated"] = ag->pms5003t_1.compensate(this->pm25_1, this->temp_1);
        if (!localServer) {
          root[json_prop_pmFirmware] =
              pms5003TFirmwareVersion(ag->pms5003t_1.getFirmwareVersion());
        }
      }
      if (config->hasSensorPMS2) {
        if(utils::isValidPm(this->pm01_2)) {
          root["pm01"] = this->pm01_2;
        }
        if(utils::isValidPm(this->pm25_2)) {
          root["pm02"] = this->pm25_2;
        }
        if(utils::isValidPm(this->pm10_2)) {
          root["pm10"] = this->pm10_2;
        }
        if(utils::isValidPm03Count(this->pm03PCount_2)) {
          root["pm003Count"] = this->pm03PCount_2;
        }

        float val;
        if (utils::isValidTemperature(this->temp_2)) {
          root["atmp"] = ag->round2(this->temp_2);

          if (localServer) {
            val = ag->pms5003t_2.compensateTemp(this->temp_2);
            if (utils::isValidTemperature(val)) {
              root["atmpCompensated"] = ag->round2(val);
            }
          }
        }
        if(utils::isValidHumidity(this->hum_2)) {
          root["rhum"] = this->hum_2;

          if (localServer) {
            val = ag->pms5003t_2.compensateHum(this->hum_2);
            if (utils::isValidHumidity(val)) {
              root["rhumCompensated"] = (int)val;
            }
          }
        }
        root["pm02Compensated"] = ag->pms5003t_2.compensate(this->pm25_2, this->temp_2);
        if(!localServer) {
          root[json_prop_pmFirmware] =
              pms5003TFirmwareVersion(ag->pms5003t_1.getFirmwareVersion());
        }
      }
    } else {
      if (fwMode == FW_MODE_O_1P) {
        float val;
        if (config->hasSensorPMS1) {
          if (utils::isValidPm(this->pm01_1)) {
            root["pm01"] = this->pm01_1;
          }
          if (utils::isValidPm(this->pm25_1)) {
            root["pm02"] = this->pm25_1;
          }
          if (utils::isValidPm(this->pm10_1)) {
            root["pm10"] = this->pm10_1;
          }
          if (utils::isValidPm03Count(this->pm03PCount_1)) {
            root["pm003Count"] = this->pm03PCount_1;
          }
          if (utils::isValidTemperature(this->temp_1)) {
            root["atmp"] = ag->round2(this->temp_1);

            if (localServer) {
              val = ag->pms5003t_1.compensateTemp(this->temp_1);
              if (utils::isValidTemperature(val)) {
                root["atmpCompensated"] = ag->round2(val);
              }
            }
          }
          if (utils::isValidHumidity(this->hum_1)) {
            root["rhum"] = this->hum_1;
            if(localServer) {
              val = ag->pms5003t_1.compensateHum(this->hum_1);
              if(utils::isValidHumidity(val)) {
                root["rhumCompensated"] = (int)val;
              }
            }
          }
          root["pm02Compensated"] = ag->pms5003t_1.compensate(this->pm25_1, this->temp_1);
          if(!localServer) {
            root[json_prop_pmFirmware] =
                pms5003TFirmwareVersion(ag->pms5003t_1.getFirmwareVersion());
          }
        } else if (config->hasSensorPMS2) {
          if(utils::isValidPm(this->pm01_2)) {
            root["pm01"] = this->pm01_2;
          }
          if(utils::isValidPm(this->pm25_2)) {
            root["pm02"] = this->pm25_2;
          }
          if(utils::isValidPm(this->pm10_2)) {
            root["pm10"] = this->pm10_2;
          }
          if(utils::isValidPm03Count(this->pm03PCount_2)) {
            root["pm003Count"] = this->pm03PCount_2;
          }
          if (utils::isValidTemperature(this->temp_2)) {
            root["atmp"] = ag->round2(this->temp_2);
            if (localServer) {

              val = ag->pms5003t_1.compensateTemp(this->temp_2);
              if (utils::isValidTemperature(val)) {
                root["atmpCompensated"] = ag->round2(val);
              }
            }
          }
          if (utils::isValidHumidity(this->hum_2)) {
            root["rhum"] = this->hum_2;

            if(localServer) {
              val = ag->pms5003t_1.compensateHum(this->hum_2);
              if(utils::isValidHumidity(val)) {
                root["rhumCompensated"] = (int)val;
              }
            }
          }
          root["pm02Compensated"] = ag->pms5003t_1.compensate(this->pm25_1, this->temp_1);
          if(!localServer) {
            root[json_prop_pmFirmware] =
                pms5003TFirmwareVersion(ag->pms5003t_2.getFirmwareVersion());
          }
        }
      } else {
        float val;
        if (config->hasSensorPMS1) {
          if(utils::isValidPm(this->pm01_1)) {
            root["channels"]["1"]["pm01"] = this->pm01_1;
          }
          if(utils::isValidPm(this->pm25_1)) {
            root["channels"]["1"]["pm02"] = this->pm25_1;
          }
          if(utils::isValidPm(this->pm10_1)) {
            root["channels"]["1"]["pm10"] = this->pm10_1;
          }
          if (utils::isValidPm03Count(this->pm03PCount_1)) {
            root["channels"]["1"]["pm003Count"] = this->pm03PCount_1;
          }
          if(utils::isValidTemperature(this->temp_1)) {
            root["channels"]["1"]["atmp"] = ag->round2(this->temp_1);

            if (localServer) {
              val = ag->pms5003t_1.compensateTemp(this->temp_1);
              if (utils::isValidTemperature(val)) {
                root["channels"]["1"]["atmpCompensated"] = ag->round2(val);
              }
            }
          }
          if (utils::isValidHumidity(this->hum_1)) {
            root["channels"]["1"]["rhum"] = this->hum_1;

            if (localServer) {
              val = ag->pms5003t_1.compensateHum(this->hum_1);
              if (utils::isValidHumidity(val)) {
                root["channels"]["1"]["rhumCompensated"] = (int)val;
              }
            }
          }
          root["channels"]["1"]["pm02Compensated"] = ag->pms5003t_1.compensate(this->pm25_1, this->temp_1);
        
          // PMS5003T version
          if(!localServer) {
            root["channels"]["1"][json_prop_pmFirmware] =
                pms5003TFirmwareVersion(ag->pms5003t_1.getFirmwareVersion());
          }
        }
        if (config->hasSensorPMS2) {
          float val;
          if (utils::isValidPm(this->pm01_2)) {
            root["channels"]["2"]["pm01"] = this->pm01_2;
          }
          if (utils::isValidPm(this->pm25_2)) {
            root["channels"]["2"]["pm02"] = this->pm25_2;
          }
          if (utils::isValidPm(this->pm10_2)) {
            root["channels"]["2"]["pm10"] = this->pm10_2;
          }
          if (utils::isValidPm03Count(this->pm03PCount_2)) {
            root["channels"]["2"]["pm003Count"] = this->pm03PCount_2;
          }
          if (utils::isValidTemperature(this->temp_2)) {
            root["channels"]["2"]["atmp"] = ag->round2(this->temp_2);

            if (localServer) {
              val = ag->pms5003t_1.compensateTemp(this->temp_2);
              if (utils::isValidTemperature(val)) {
                root["channels"]["2"]["atmpCompensated"] = ag->round2(val);
              }
            }
          }
          if (utils::isValidHumidity(this->hum_2)) {
            root["channels"]["2"]["rhum"] = this->hum_2;

            if (localServer) {
              val = ag->pms5003t_1.compensateHum(this->hum_2);
              if (utils::isValidHumidity(val)) {
                root["channels"]["2"]["rhumCompensated"] = (int)val;
              }
            }
          }
          root["channels"]["2"]["pm02Compensated"] = ag->pms5003t_2.compensate(this->pm25_2, this->temp_2);
          // PMS5003T version
          if(!localServer) {
            root["channels"]["2"][json_prop_pmFirmware] =
                pms5003TFirmwareVersion(ag->pms5003t_2.getFirmwareVersion());
          }
        }
      }
    }
  }

  if (config->hasSensorSGP) {
    if (utils::isValidVOC(this->TVOC)) {
      root["tvocIndex"] = this->TVOC;
    }
    if (utils::isValidVOC(this->TVOCRaw)) {
      root["tvocRaw"] = this->TVOCRaw;
    }
    if (utils::isValidNOx(this->NOx)) {
      root["noxIndex"] = this->NOx;
    }
    if (utils::isValidNOx(this->NOxRaw)) {
      root["noxRaw"] = this->NOxRaw;
    }
  }
  root["boot"] = bootCount;
  root["bootCount"] = bootCount;

  if (localServer) {
    if (ag->isOne()) {
      root["ledMode"] = config->getLedBarModeName();
    }
    root["firmware"] = ag->getVersion();
    root["model"] = AgFirmwareModeName(fwMode);
  }

  return JSON.stringify(root);
}
