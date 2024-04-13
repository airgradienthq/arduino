#include "AgValue.h"
#include "AgConfigure.h"
#include "AirGradient.h"
#include "Libraries/Arduino_JSON/src/Arduino_JSON.h"

String Measurements::toString(bool localServer, AgFirmwareMode fwMode, int rssi,
                              void *_ag, void *_config) {
  AirGradient *ag = (AirGradient *)_ag;
  Configuration *config = (Configuration *)_config;

  JSONVar root;
  root["wifi"] = rssi;
  if (localServer) {
    root["serialno"] = ag->deviceId();
  }
  if (config->hasSensorS8) {
    if (this->CO2 >= 0) {
      root["rco2"] = this->CO2;
    }
  }

  if (ag->isOne()) {
    if (config->hasSensorPMS1) {
      if (this->pm01_1 >= 0) {
        root["pm01"] = this->pm01_1;
      }
      if (this->pm25_1 >= 0) {
        root["pm02"] = this->pm25_1;
      }
      if (this->pm10_1 >= 0) {
        root["pm10"] = this->pm10_1;
      }
      if (this->pm03PCount_1 >= 0) {
        if (localServer) {
          root["pm003Count"] = this->pm03PCount_1;
        } else {
          root["pm003_count"] = this->pm03PCount_1;
        }
      }
    }

    if (config->hasSensorSHT) {
      if (this->Temperature > -1001) {
        root["atmp"] = ag->round2(this->Temperature);
        root["atmp_compensated"] = ag->round2(this->Temperature);
      }
      if (this->Humidity >= 0) {
        root["rhum"] = this->Humidity;
        root["rhum_compensated"] = this->Humidity;
      }
    }

  } else {
    if (config->hasSensorPMS1 && config->hasSensorPMS2) {
      root["pm01"] = ag->round2((this->pm01_1 + this->pm01_2) / 2.0);
      root["pm02"] = ag->round2((this->pm25_1 + this->pm25_2) / 2.0);
      root["pm10"] = ag->round2((this->pm10_1 + this->pm10_2) / 2.0);
      if (localServer) {
        root["pm003Count"] =
            ag->round2((this->pm03PCount_1 + this->pm03PCount_2) / 2.0);
      } else {
        root["pm003_count"] =
            ag->round2((this->pm03PCount_1 + this->pm03PCount_2) / 2.0);
      }
      root["atmp"] = ag->round2((this->temp_1 + this->temp_2) / 2.0f);
      root["rhum"] = ag->round2((this->hum_1 + this->hum_2) / 2.0f);
      root["atmp_compensated"] =
          ag->round2(ag->pms5003t_2.temperatureCompensated(
              (this->temp_1 + this->temp_2) / 2.0f));
      root["rhum_compensated"] = (int)ag->pms5003t_2.humidityCompensated(
          (this->hum_1 + this->hum_2) / 2.0f);
    }

    if (fwMode == FW_MODE_O_1PS || fwMode == FW_MODE_O_1PST) {
      if (config->hasSensorPMS1) {
        root["pm01"] = this->pm01_1;
        root["pm02"] = this->pm25_1;
        root["pm10"] = this->pm10_1;
        if (localServer) {
          root["pm003Count"] = this->pm03PCount_1;
        } else {
          root["pm003_count"] = this->pm03PCount_1;
        }
        root["atmp"] = ag->round2(this->temp_1);
        root["rhum"] = this->hum_1;
        root["atmp_compensated"] =
            ag->round2(ag->pms5003t_1.temperatureCompensated(this->temp_1));
        root["rhum_compensated"] =
            (int)ag->pms5003t_1.humidityCompensated(this->hum_1);
      }
      if (config->hasSensorPMS2) {
        root["pm01"] = this->pm01_2;
        root["pm02"] = this->pm25_2;
        root["pm10"] = this->pm10_2;
        if (localServer) {
          root["pm003Count"] = this->pm03PCount_2;
        } else {
          root["pm003_count"] = this->pm03PCount_2;
        }
        root["atmp"] = ag->round2(this->temp_2);
        root["rhum"] = this->hum_2;
        root["atmp_compensated"] =
            ag->round2(ag->pms5003t_2.temperatureCompensated(this->temp_2));
        root["rhum_compensated"] =
            (int)ag->pms5003t_2.humidityCompensated(this->hum_2);
      }
    } else {
      if (config->hasSensorPMS1) {
        root["channels"]["1"]["pm01"] = this->pm01_1;
        root["channels"]["1"]["pm02"] = this->pm25_1;
        root["channels"]["1"]["pm10"] = this->pm10_1;
        if (localServer) {
          root["channels"]["1"]["pm003Count"] = this->pm03PCount_1;
        } else {
          root["channels"]["1"]["pm003_count"] = this->pm03PCount_1;
        }
        root["channels"]["1"]["atmp"] = ag->round2(this->temp_1);
        root["channels"]["1"]["rhum"] = this->hum_1;
        root["channels"]["1"]["atmp_compensated"] =
            ag->round2(ag->pms5003t_1.temperatureCompensated(this->temp_1));
        root["channels"]["1"]["rhum_compensated"] =
            (int)ag->pms5003t_1.humidityCompensated(this->hum_1);
      }
      if (config->hasSensorPMS2) {
        root["channels"]["2"]["pm01"] = this->pm01_2;
        root["channels"]["2"]["pm02"] = this->pm25_2;
        root["channels"]["2"]["pm10"] = this->pm10_2;
        if (localServer) {
          root["channels"]["2"]["pm003Count"] = this->pm03PCount_2;
        } else {
          root["channels"]["2"]["pm003_count"] = this->pm03PCount_2;
        }
        root["channels"]["2"]["atmp"] = ag->round2(this->temp_2);
        root["channels"]["2"]["rhum"] = this->hum_2;
        root["channels"]["2"]["atmp_compensated"] =
            ag->round2(ag->pms5003t_1.temperatureCompensated(this->temp_2));
        root["channels"]["2"]["rhum_compensated"] =
            (int)ag->pms5003t_1.humidityCompensated(this->hum_2);
      }
    }
  }

  if (config->hasSensorSGP) {
    if (this->TVOC >= 0) {
      if (localServer) {
        root["tvocIndex"] = this->TVOC;
      } else {
        root["tvoc_index"] = this->TVOC;
      }
    }
    if (this->TVOCRaw >= 0) {
      root["tvoc_raw"] = this->TVOCRaw;
    }
    if (this->NOx >= 0) {
      if (localServer) {
        root["noxIndex"] = this->NOx;
      } else {
        root["nox_index"] = this->NOx;
      }
    }
    if (this->NOxRaw >= 0) {
      root["nox_raw"] = this->NOxRaw;
    }
  }
  root["boot"] = bootCount;

  if (localServer) {
    root["ledMode"] = config->getLedBarModeName();
    root["firmwareVersion"] = ag->getVersion();
    root["fwMode"] = AgFirmwareModeName(fwMode);
  }

  return JSON.stringify(root);
}
