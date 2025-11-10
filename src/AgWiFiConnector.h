#ifndef _AG_WIFI_CONNECTOR_H_
#define _AG_WIFI_CONNECTOR_H_

#include "AgOledDisplay.h"
#include "AgStateMachine.h"
#include "AirGradient.h"
#include "AgConfigure.h"
#include "Libraries/WiFiManager/WiFiManager.h"
#include "Main/PrintLog.h"
#include "NimBLECharacteristic.h"
#include "NimBLEService.h"
#include "esp32-hal.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

// Provisioning Status Codes
#define PROV_WIFI_CONNECT                         0   // WiFi Connect
#define PROV_CONNECTING_TO_SERVER                 1   // Connecting to server
#define PROV_SERVER_REACHABLE                     2   // Server reachable
#define PROV_MONITOR_CONFIGURED                   3   // Monitor configured properly on dashboard

// Provisioning Error Codes
#define PROV_ERR_WIFI_CONNECT_FAILED             10   // Failed to connect to WiFi
#define PROV_ERR_SERVER_UNREACHABLE              11   // Server unreachable
#define PROV_ERR_GET_MONITOR_CONFIG_FAILED       12   // Failed to get monitor configuration from dashboard
#define PROV_ERR_MONITOR_NOT_REGISTERED          13   // Monitor is not registered on dashboard

class WifiConnector : public PrintLog {
public:
  enum class ProvisionMethod {
    Unknown = 0,
    WiFi,
    BLE
  };

  struct WiFiNetwork {
    String ssid;
    int32_t rssi;
    bool open;
  };

private:
  AirGradient *ag;
  OledDisplay &disp;
  StateMachine &sm;
  Configuration &config;
  NimBLEServer *pServer;

  EventGroupHandle_t bleEventGroup;

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
  String scanFilteredWiFiJSON();

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

  void setupProvisionByPortal(WiFiManagerParameter *disableCloudParam, WiFiManagerParameter *disableCloudInfo);
  void setupProvisionByBLE(const char *modelName);
  void stopBLE();
  bool connect(String modelName = "");
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
