#include "LocalServer.h"

LocalServer::LocalServer(Stream &log, OpenMetrics &openMetrics,
                         Measurements &measure, Configuration &config,
                         WifiConnector &wifiConnector)
    : PrintLog(log, "LocalServer"), openMetrics(openMetrics), measure(measure),
      config(config), wifiConnector(wifiConnector) {}

LocalServer::~LocalServer() {}

bool LocalServer::begin(void) {
  server.on("/measures/current", HTTP_GET, [this]() { _GET_measure(); });
  server.on(openMetrics.getApi(), HTTP_GET, [this]() { _GET_metrics(); });
  server.on("/config", HTTP_GET, [this]() { _GET_config(); });
  server.on("/config", HTTP_PUT, [this]() { _PUT_config(); });
  server.on("/storage", HTTP_GET, [this]() {
    Serial.println(server.arg("time"));
    _GET_storage();
  });
  server.begin();

  if (xTaskCreate(
          [](void *param) {
            LocalServer *localServer = (LocalServer *)param;
            for (;;) {
              localServer->_handle();
            }
          },
          "webserver", 1024 * 4, this, 5, NULL) != pdTRUE) {
    Serial.println("Create task handle webserver failed");
  }
  logInfo("Init: " + getHostname() + ".local");

  return true;
}

void LocalServer::setAirGraident(AirGradient *ag) { this->ag = ag; }

String LocalServer::getHostname(void) {
  return "airgradient_" + ag->deviceId();
}

void LocalServer::_handle(void) { server.handleClient(); }

void LocalServer::_GET_config(void) {
  if(ag->isOne()) {
    server.send(200, "application/json", config.toString());
  } else {
    server.send(200, "application/json", config.toString(fwMode));
  }
}

void LocalServer::_PUT_config(void) {
  String data = server.arg(0);
  String response = "";
  int statusCode = 400; // Status code for data invalid
  if (config.parse(data, true)) {
    statusCode = 200;
    response = "Success";
  } else {
    response = config.getFailedMesage();
  }
  server.send(statusCode, "text/plain", response);
}

void LocalServer::_GET_metrics(void) {
  server.send(200, openMetrics.getApiContentType(), openMetrics.getPayload());
}

void LocalServer::_GET_measure(void) {
  String toSend = measure.toString(true, fwMode, wifiConnector.RSSI(), *ag, config);
  server.send(200, "application/json", toSend);
}

void LocalServer::_GET_storage(void) {
  char *data = measure.getLocalStorage();
  if (data != nullptr) {
    server.sendHeader("Content-Disposition", "attachment; filename=\"measurements.csv\"");
    server.send(200, "text/plain", data);
    free(data);
  } else {
    server.send(200, "text/plain", "No data");
  }
}

void LocalServer::setFwMode(AgFirmwareMode fwMode) { this->fwMode = fwMode; }
