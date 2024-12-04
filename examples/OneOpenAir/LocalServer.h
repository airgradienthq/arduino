#ifndef _LOCAL_SERVER_H_
#define _LOCAL_SERVER_H_

#include "AgConfigure.h"
#include "AgValue.h"
#include "AirGradient.h"
#include "OpenMetrics.h"
#include "AgWiFiConnector.h"
#include <Arduino.h>
#include <WebServer.h>

class LocalServer : public PrintLog {
private:
  AirGradient *ag;
  OpenMetrics &openMetrics;
  Measurements &measure;
  Configuration &config;
  WifiConnector &wifiConnector;
  WebServer server;
  AgFirmwareMode fwMode;

public:
  LocalServer(Stream &log, OpenMetrics &openMetrics, Measurements &measure,
              Configuration &config, WifiConnector& wifiConnector);
  ~LocalServer();

  bool begin(void);
  void setAirGraident(AirGradient *ag);
  String getHostname(void);
  void setFwMode(AgFirmwareMode fwMode);
  void _handle(void);
  void _GET_config(void);
  void _PUT_config(void);
  void _GET_metrics(void);
  void _GET_measure(void);
  void _GET_storage(void);
};

#endif /** _LOCAL_SERVER_H_ */
