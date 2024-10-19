#ifndef _AG_WIFI_CONNECTOR_H_
#define _AG_WIFI_CONNECTOR_H_

#include "AgOledDisplay.h"
#include "AgStateMachine.h"
#include "AirGradient.h"
#include "AgConfigure.h"
#include "Main/PrintLog.h"

#include <Arduino.h>

class WifiConnector : public PrintLog {
private:
  AirGradient *ag;
  OledDisplay &disp;
  StateMachine &sm;
  Configuration &config;

  String ssid;
  void *wifi = NULL;
  bool hasConfig;
  uint32_t lastRetry;
  bool hasPortalConfig = false;
  bool connectorTimeout = false;

  bool wifiClientConnected(void);

public:
  void setAirGradient(AirGradient *ag);

  WifiConnector(OledDisplay &disp, Stream &log, StateMachine &sm, Configuration& config);
  ~WifiConnector();

  bool connect(void);
  void disconnect(void);
  void handle(void);
  void _wifiApCallback(void);
  void _wifiSaveConfig(void);
  void _wifiSaveParamCallback(void);
  bool _wifiConfigPortalActive(void);
  void _wifiTimeoutCallback(void);
  void _wifiProcess();
  bool isConnected(void);
  void reset(void);
  int RSSI(void);
  String localIpStr(void);
  bool hasConfigurated(void);
  bool isConfigurePorttalTimeout(void);

  const char* defaultSsid = "airgradient";
  const char* defaultPassword = "cleanair";
  void setDefault(void);
};

#endif /** _AG_WIFI_CONNECTOR_H_ */
