#include "OpenMetrics.h"

OpenMetrics::OpenMetrics(Measurements &measure, Configuration &config,
                         WifiConnector &wifiConnector, AgApiClient &apiClient)
    : measure(measure), config(config), wifiConnector(wifiConnector),
      apiClient(apiClient) {}

OpenMetrics::~OpenMetrics() {}

void OpenMetrics::setAirGradient(AirGradient *ag) { this->ag = ag; }

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
  add_metric_point("", apiClient.isFetchConfigureFailed() ? "0" : "1");

  add_metric(
      "post_ok",
      "1 if the AirGradient device was able to successfully send to the server",
      "gauge");
  add_metric_point("", apiClient.isPostToServerFailed() ? "0" : "1");

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
  int ahumCompensated = utils::getInvalidHumidity();
  int tvoc = utils::getInvalidVOC();
  int tvocRaw = utils::getInvalidVOC();
  int nox = utils::getInvalidNOx();
  int noxRaw = utils::getInvalidNOx();

  if (config.hasSensorSHT) {
    _temp = measure.getFloat(Measurements::Temperature);
    _hum = measure.getFloat(Measurements::Humidity);
    atmpCompensated = _temp;
    ahumCompensated = _hum;
  }

  if (config.hasSensorPMS1) {
    pm01 = measure.get(Measurements::PM01);
    float correctedPm = measure.getCorrectedPM25(*ag, config, false, 1);
    pm25 = round(correctedPm);
    pm10 = measure.get(Measurements::PM10);
    pm03PCount = measure.get(Measurements::PM03_PC);
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

  if (config.hasSensorPMS1) {
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
                 "The processed Nitrous Oxide (NOx) index as measured by the "
                 "AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(nox));
    }
    if (utils::isValidNOx(noxRaw)) {
      add_metric("nox_raw",
                 "The raw input value to the Nitrous Oxide (NOx) index as "
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
    add_metric(
        "temperature",
        "The ambient temperature as measured by the AirGradient SHT / PMS "
        "sensor, in degrees Celsius",
        "gauge", "celsius");
    add_metric_point("", String(_temp));
  }
  if (utils::isValidTemperature(atmpCompensated)) {
    add_metric("temperature_compensated",
               "The compensated ambient temperature as measured by the "
               "AirGradient SHT / PMS "
               "sensor, in degrees Celsius",
               "gauge", "celsius");
    add_metric_point("", String(atmpCompensated));
  }
  if (utils::isValidHumidity(_hum)) {
    add_metric(
        "humidity",
        "The relative humidity as measured by the AirGradient SHT sensor",
        "gauge", "percent");
    add_metric_point("", String(_hum));
  }
  if (utils::isValidHumidity(ahumCompensated)) {
    add_metric("humidity_compensated",
               "The compensated relative humidity as measured by the "
               "AirGradient SHT / PMS sensor",
               "gauge", "percent");
    add_metric_point("", String(ahumCompensated));
  }

  response += "# EOF\n";
  return response;
}
