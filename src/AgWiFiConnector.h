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
#ifdef ESP32
  OledDisplay &disp;
  StateMachine &sm;
  Configuration &config;
#else
  void displayShowText(String ln1, String ln2, String ln3);
#endif
  String ssid;
  void *wifi = NULL;
  bool hasConfig;
  uint32_t lastRetry;
  bool hasPortalConfig = false;
  bool connectorTimeout = false;

  bool wifiClientConnected(void);

public:
  void setAirGradient(AirGradient *ag);
#ifdef ESP32
  WifiConnector(OledDisplay &disp, Stream &log, StateMachine &sm, Configuration& config);
#else
  WifiConnector(Stream &log);
#endif
  ~WifiConnector();

  bool connect(void);
  void disconnect(void);
  void handle(void);
#ifdef ESP32
  void _wifiApCallback(void);
  void _wifiSaveConfig(void);
  void _wifiSaveParamCallback(void);
  bool _wifiConfigPortalActive(void);
  void _wifiTimeoutCallback(void);
#endif
  void _wifiProcess();
  bool isConnected(void);
  void reset(void);
  int RSSI(void);
  String localIpStr(void);
  bool hasConfigurated(void);
  bool isConfigurePorttalTimeout(void);
};

#endif /** _AG_WIFI_CONNECTOR_H_ */
