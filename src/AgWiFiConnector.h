#ifndef _AG_WIFI_CONNECTOR_H_
#define _AG_WIFI_CONNECTOR_H_

#include "AirGradient.h"
#include "AgOledDisplay.h"

class AgWiFiConnector
{
private:
  AirGradient &ag;
public:
  AgWiFiConnector(AirGradient& ag, AgOledDisplay &disp, String ssid);
  ~AgWiFiConnector();
};




#endif /** _AG_WIFI_CONNECTOR_H_ */
