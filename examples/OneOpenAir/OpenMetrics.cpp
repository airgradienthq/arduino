#include "OpenMetrics.h"

OpenMetrics::OpenMetrics(Measurements &measure, Configuration &config,
                         WifiConnector &wifiConnector)
    : measure(measure), config(config), wifiConnector(wifiConnector) {}

OpenMetrics::~OpenMetrics() {}

void OpenMetrics::setAirGradient(AirGradient *ag) {
  this->ag = ag;
}

void OpenMetrics::setAirgradientClient(AirgradientClient *client) {
  this->agClient = client;
}

const char *OpenMetrics::getApiContentType(void) {
  return "application/openmetrics-text; version=1.0.0; charset=utf-8";
}

const char *OpenMetrics::getApi(void) { return "/metrics"; }

String OpenMetrics::getPayload(void) {
  String response;
  String current_metric_name;
  const auto add_metric = [&](const String &name, const String &help,
                              const String &type, const String &unit = "") {
    current_metric_name = "airgradient_" + name;
    if (!unit.isEmpty())
      current_metric_name += "_" + unit;
    response += "# HELP " + current_metric_name + " " + help + "\n";
    response += "# TYPE " + current_metric_name + " " + type + "\n";
    if (!unit.isEmpty())
      response += "# UNIT " + current_metric_name + " " + unit + "\n";
  };
  const auto add_metric_point = [&](const String &labels, const String &value) {
    response += current_metric_name + "{" + labels + "} " + value + "\n";
  };

  add_metric("info", "AirGradient device information", "info");
  add_metric_point("airgradient_serial_number=\"" + ag->deviceId() +
                       "\",airgradient_device_type=\"" + ag->getBoardName() +
                       "\",airgradient_library_version=\"" + ag->getVersion() +
                       "\"",
                   "1");

  add_metric("config_ok",
             "1 if the AirGradient device was able to successfully fetch its "
             "configuration from the server",
             "gauge");
  add_metric_point("", agClient->isLastFetchConfigSucceed() ? "1" : "0");

  add_metric(
      "post_ok",
      "1 if the AirGradient device was able to successfully send to the server",
      "gauge");
  add_metric_point("", agClient->isLastPostMeasureSucceed() ? "1" : "0");

  add_metric(
      "wifi_rssi",
      "WiFi signal strength from the AirGradient device perspective, in dBm",
      "gauge", "dbm");
  add_metric_point("", String(wifiConnector.RSSI()));

  // Initialize default invalid value for each measurements
  float _temp = utils::getInvalidTemperature();
  float _hum = utils::getInvalidHumidity();
  int pm01 = utils::getInvalidPmValue();
  int pm25 = utils::getInvalidPmValue();
  int pm10 = utils::getInvalidPmValue();
  int pm03PCount = utils::getInvalidPmValue();
  int co2 = utils::getInvalidCO2();
  int atmpCompensated = utils::getInvalidTemperature();
  int rhumCompensated = utils::getInvalidHumidity();
  int tvoc = utils::getInvalidVOC();
  int tvocRaw = utils::getInvalidVOC();
  int nox = utils::getInvalidNOx();
  int noxRaw = utils::getInvalidNOx();

  // Get values
  if (config.hasSensorPMS1 && config.hasSensorPMS2) {
    _temp = (measure.getFloat(Measurements::Temperature, 1) +
             measure.getFloat(Measurements::Temperature, 2)) /
            2.0f;
    _hum = (measure.getFloat(Measurements::Humidity, 1) +
            measure.getFloat(Measurements::Humidity, 2)) /
           2.0f;
    pm01 = (measure.get(Measurements::PM01, 1) + measure.get(Measurements::PM01, 2)) / 2.0f;
    float correctedPm25_1 = measure.getCorrectedPM25(false, 1);
    float correctedPm25_2 = measure.getCorrectedPM25(false, 2);
    float correctedPm25 = (correctedPm25_1 + correctedPm25_2) / 2.0f;
    pm25 = round(correctedPm25);
    pm10 = (measure.get(Measurements::PM10, 1) + measure.get(Measurements::PM10, 2)) / 2.0f;
    pm03PCount =
        (measure.get(Measurements::PM03_PC, 1) + measure.get(Measurements::PM03_PC, 2)) / 2.0f;
  } else {
    if (ag->isOne()) {
      if (config.hasSensorSHT) {
        _temp = measure.getFloat(Measurements::Temperature);
        _hum = measure.getFloat(Measurements::Humidity);
      }

      if (config.hasSensorPMS1) {
        pm01 = measure.get(Measurements::PM01);
        float correctedPm = measure.getCorrectedPM25(false, 1);
        pm25 = round(correctedPm);
        pm10 = measure.get(Measurements::PM10);
        pm03PCount = measure.get(Measurements::PM03_PC);
      }
    } else {
      if (config.hasSensorPMS1) {
        _temp = measure.getFloat(Measurements::Temperature, 1);
        _hum = measure.getFloat(Measurements::Humidity, 1);
        pm01 = measure.get(Measurements::PM01, 1);
        float correctedPm = measure.getCorrectedPM25(false, 1);
        pm25 = round(correctedPm);
        pm10 = measure.get(Measurements::PM10, 1);
        pm03PCount = measure.get(Measurements::PM03_PC, 1);
      }
      if (config.hasSensorPMS2) {
        _temp = measure.getFloat(Measurements::Temperature, 2);
        _hum = measure.getFloat(Measurements::Humidity, 2);
        pm01 = measure.get(Measurements::PM01, 2);
        float correctedPm = measure.getCorrectedPM25(false, 2);
        pm25 = round(correctedPm);
        pm10 = measure.get(Measurements::PM10, 2);
        pm03PCount = measure.get(Measurements::PM03_PC, 2);
      }
    }
  }

  if (config.hasSensorSGP) {
    tvoc = measure.get(Measurements::TVOC);
    tvocRaw = measure.get(Measurements::TVOCRaw);
    nox = measure.get(Measurements::NOx);
    noxRaw = measure.get(Measurements::NOxRaw);
  }

  if (config.hasSensorS8) {
    co2 = measure.get(Measurements::CO2);
  }

  /** Get temperature and humidity compensated */
  if (ag->isOne()) {
    atmpCompensated = round(measure.getCorrectedTempHum(Measurements::Temperature));
    rhumCompensated = round(measure.getCorrectedTempHum(Measurements::Humidity));
  } else {
    atmpCompensated = round((measure.getCorrectedTempHum(Measurements::Temperature, 1) +
                             measure.getCorrectedTempHum(Measurements::Temperature, 2)) /
                            2.0f);
    rhumCompensated = round((measure.getCorrectedTempHum(Measurements::Humidity, 1) +
                             measure.getCorrectedTempHum(Measurements::Humidity, 2)) /
                            2.0f);
  }

  // Add measurements that valid to the metrics
  if (config.hasSensorPMS1 || config.hasSensorPMS2) {
    if (utils::isValidPm(pm01)) {
      add_metric("pm1",
                 "PM1.0 concentration as measured by the AirGradient PMS "
                 "sensor, in micrograms per cubic meter",
                 "gauge", "ugm3");
      add_metric_point("", String(pm01));
    }
    if (utils::isValidPm(pm25)) {
      add_metric("pm2d5",
                 "PM2.5 concentration as measured by the AirGradient PMS "
                 "sensor, in micrograms per cubic meter",
                 "gauge", "ugm3");
      add_metric_point("", String(pm25));
    }
    if (utils::isValidPm(pm10)) {
      add_metric("pm10",
                 "PM10 concentration as measured by the AirGradient PMS "
                 "sensor, in micrograms per cubic meter",
                 "gauge", "ugm3");
      add_metric_point("", String(pm10));
    }
    if (utils::isValidPm03Count(pm03PCount)) {
      add_metric("pm0d3",
                 "PM0.3 concentration as measured by the AirGradient PMS "
                 "sensor, in number of particules per 100 milliliters",
                 "gauge", "p100ml");
      add_metric_point("", String(pm03PCount));
    }
  }

  if (config.hasSensorSGP) {
    if (utils::isValidVOC(tvoc)) {
      add_metric("tvoc_index",
                 "The processed Total Volatile Organic Compounds (TVOC) index "
                 "as measured by the AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(tvoc));
    }
    if (utils::isValidVOC(tvocRaw)) {
      add_metric("tvoc_raw",
                 "The raw input value to the Total Volatile Organic Compounds "
                 "(TVOC) index as measured by the AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(tvocRaw));
    }
    if (utils::isValidNOx(nox)) {
      add_metric("nox_index",
                 "The processed Nitrogen Oxide (NOx) index as measured by the "
                 "AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(nox));
    }
    if (utils::isValidNOx(noxRaw)) {
      add_metric("nox_raw",
                 "The raw input value to the Nitrogen Oxide (NOx) index as "
                 "measured by the AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(noxRaw));
    }
  }

  if (utils::isValidCO2(co2)) {
    add_metric("co2",
               "Carbon dioxide concentration as measured by the AirGradient S8 "
               "sensor, in parts per million",
               "gauge", "ppm");
    add_metric_point("", String(co2));
  }

  if (utils::isValidTemperature(_temp)) {
    add_metric("temperature",
               "The ambient temperature as measured by the AirGradient SHT / PMS "
               "sensor, in degrees Celsius",
               "gauge", "celsius");
    add_metric_point("", String(_temp));
  }
  if (utils::isValidTemperature(atmpCompensated)) {
    add_metric("temperature_compensated",
               "The compensated ambient temperature as measured by the AirGradient SHT / PMS "
               "sensor, in degrees Celsius",
               "gauge", "celsius");
    add_metric_point("", String(atmpCompensated));
  }
  if (utils::isValidHumidity(_hum)) {
    add_metric("humidity", "The relative humidity as measured by the AirGradient SHT sensor",
               "gauge", "percent");
    add_metric_point("", String(_hum));
  }
  if (utils::isValidHumidity(rhumCompensated)) {
    add_metric("humidity_compensated",
               "The compensated relative humidity as measured by the AirGradient SHT / PMS sensor",
               "gauge", "percent");
    add_metric_point("", String(rhumCompensated));
  }

  response += "# EOF\n";
  return response;
}
