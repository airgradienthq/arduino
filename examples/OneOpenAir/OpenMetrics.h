#ifndef _OPEN_METRICS_H_
#define _OPEN_METRICS_H_

#include "AgConfigure.h"
#include "AgValue.h"
#include "AgWiFiConnector.h"
#include "AirGradient.h"
#include "Libraries/airgradient-client/src/airgradientClient.h"

class OpenMetrics {
private:
  AirGradient *ag;
  AirgradientClient *agClient;
  Measurements &measure;
  Configuration &config;
  WifiConnector &wifiConnector;

public:
  OpenMetrics(Measurements &measure, Configuration &config,
              WifiConnector &wifiConnector);
  ~OpenMetrics();
  void setAirGradient(AirGradient *ag, AirgradientClient *client);
  const char *getApiContentType(void);
  const char* getApi(void);
  String getPayload(void);
};

#endif /** _OPEN_METRICS_H_ */
