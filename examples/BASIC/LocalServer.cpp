#include "LocalServer.h"

LocalServer::LocalServer(Stream &log, OpenMetrics &openMetrics,
                         Measurements &measure, Configuration &config,
                         WifiConnector &wifiConnector)
    : PrintLog(log, "LocalServer"), openMetrics(openMetrics), measure(measure),
      config(config), wifiConnector(wifiConnector), server(80) {}

LocalServer::~LocalServer() {}

bool LocalServer::begin(void) {
  server.on("/measures/current", HTTP_GET, [this]() { _GET_measure(); });
  server.on(openMetrics.getApi(), HTTP_GET, [this]() { _GET_metrics(); });
  server.on("/config", HTTP_GET, [this]() { _GET_config(); });
  server.on("/config", HTTP_PUT, [this]() { _PUT_config(); });
  server.begin();
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

void LocalServer::setFwMode(AgFirmwareMode fwMode) { this->fwMode = fwMode; }
