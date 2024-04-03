#ifndef _AG_WIFI_CONNECTOR_H_
#define _AG_WIFI_CONNECTOR_H_

#include "AgOledDisplay.h"
#include "AirGradient.h"
#include "AgStateMachine.h"
#include "Main/PrintLog.h"

#include <Arduino.h>

class AgWiFiConnector : public PrintLog {
private:
  AirGradient &ag;
  AgOledDisplay &disp;
  AgStateMachine &sm;
  String ssid;
  void *wifi = NULL;

  bool wifiClientConnected(void);
public:
  AgWiFiConnector(AirGradient &ag, AgOledDisplay &disp, String ssid,
                  Stream &log, AgStateMachine &sm);
  ~AgWiFiConnector();

  void setHotspotSSID(String ssid);
  bool connect(uint32_t timeout);
  void _wifiApCallback(void);
  void _wifiSaveConfig(void);
  void _wifiSaveParamCallback(void);
  bool _wifiConfigPortalActive(void);
  void _wifiProcess();
};

#endif /** _AG_WIFI_CONNECTOR_H_ */
