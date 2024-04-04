#ifdef ESP32

#ifndef _AG_WIFI_CONNECTOR_H_
#define _AG_WIFI_CONNECTOR_H_

#include "AgOledDisplay.h"
#include "AgStateMachine.h"
#include "AirGradient.h"
#include "Main/PrintLog.h"

#include <Arduino.h>

class AgWiFiConnector : public PrintLog {
private:
  AirGradient *ag;
  AgOledDisplay &disp;
  AgStateMachine &sm;
  String ssid;
  void *wifi = NULL;
  bool hasConfig;
  uint32_t lastRetry;

  bool wifiClientConnected(void);

public:
  AgWiFiConnector(AgOledDisplay &disp, Stream &log, AgStateMachine &sm);
  ~AgWiFiConnector();

  void setAirGradient(AirGradient *ag);
  bool connect(void);
  void disconnect(void);
  void handle(void);
  void _wifiApCallback(void);
  void _wifiSaveConfig(void);
  void _wifiSaveParamCallback(void);
  bool _wifiConfigPortalActive(void);
  void _wifiProcess();
  bool isConnected(void);
  void reset(void);
  int RSSI(void);
  String localIpStr(void);
};

#endif /** _AG_WIFI_CONNECTOR_H_ */


#endif
