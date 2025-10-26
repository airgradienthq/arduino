#ifndef _AG_WIFI_CONNECTOR_H_
#define _AG_WIFI_CONNECTOR_H_

#include "AgOledDisplay.h"
#include "AgStateMachine.h"
#include "AirGradient.h"
#include "AgConfigure.h"
#include "Main/PrintLog.h"
#include "NimBLECharacteristic.h"
#include "NimBLEService.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

class WifiConnector : public PrintLog {
public:
  enum class ProvisionMethod {
    Unknown = 0,
    WiFi,
    BLE
  };

private:
  AirGradient *ag;
  OledDisplay &disp;
  StateMachine &sm;
  Configuration &config;
  NimBLEServer *pServer;

  String ssid;
  void *wifi = NULL;
  bool hasConfig;
  uint32_t lastRetry;
  bool hasPortalConfig = false;
  bool connectorTimeout = false;
  bool bleServerRunning = false;
  bool bleClientConnected = false;
  bool wifiConnecting = false;
  ProvisionMethod provisionMethod = ProvisionMethod::Unknown;

  bool wifiClientConnected(void);
  bool isBleClientConnected();

  // BLE server handler
  class ServerCallbacks : public NimBLEServerCallbacks {
  public:
    explicit ServerCallbacks(WifiConnector *parent);
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override;
    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override;
    void onAuthenticationComplete(NimBLEConnInfo &connInfo) override;

  private:
    WifiConnector *parent;
  };

  // BLE Characteristics handler
  class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  public:
    explicit CharacteristicCallbacks(WifiConnector *parent);
    void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override;
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override;
  private:
    WifiConnector *parent;
  };


public:
  void setAirGradient(AirGradient *ag);

  WifiConnector(OledDisplay &disp, Stream &log, StateMachine &sm, Configuration &config);
  ~WifiConnector();

  void setupBLE(String bleName);
  void stopBLE();
  bool connect(void);
  void disconnect(void);
  void handle(void);
  void _wifiApCallback(void);
  void _wifiSaveConfig(void);
  void _wifiSaveParamCallback(void);
  bool _wifiConfigPortalActive(void);
  void _wifiTimeoutCallback(void);
  void _wifiStop();
  void _wifiProcess();
  bool isConnected(void);
  void reset(void);
  int RSSI(void);
  String localIpStr(void);
  bool hasConfigurated(void);
  bool isConfigurePorttalTimeout(void);


  void bleNotifyStatus(int status);

  const char *defaultSsid = "airgradient";
  const char *defaultPassword = "cleanair";
  void setDefault(void);
};

#endif /** _AG_WIFI_CONNECTOR_H_ */
