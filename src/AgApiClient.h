/**
 * @file AgApiClient.h
 * @brief HTTP client connect post data to Aigradient cloud.
 * @version 0.1
 * @date 2024-Apr-02
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef _AG_API_CLIENT_H_
#define _AG_API_CLIENT_H_

#include "AgConfigure.h"
#include "AirGradient.h"
#include "Main/PrintLog.h"
#include <Arduino.h>

#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#else
#include <HTTPClient.h>
#endif

class AgApiClient : public PrintLog {
private:
  Configuration &config;
  AirGradient *ag;
  String apiRoot = "http://hw.airgradient.com";

  bool getConfigFailed;
  bool postToServerFailed;
  bool notAvailableOnDashboard = false; // Device not setup on Airgradient cloud dashboard.
  uint16_t timeoutMs = 10000;           // Default set to 10s

  void _setHttpClientTimeout(HTTPClient *httpClient);

public:
  AgApiClient(Stream &stream, Configuration &config);
  ~AgApiClient();

  void begin(void);
  bool fetchServerConfiguration(void);
  bool postToServer(String data);
  bool isFetchConfigureFailed(void);
  bool isPostToServerFailed(void);
  bool isNotAvailableOnDashboard(void);
  void setAirGradient(AirGradient *ag);
  bool sendPing(int rssi, int bootCount);
  String getApiRoot() const;
  void setApiRoot(const String &apiRoot);
  void setTimeout(uint16_t timeoutMs);
};

#endif /** _AG_API_CLIENT_H_ */
