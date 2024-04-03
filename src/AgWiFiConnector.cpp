#include "AgWiFiConnector.h"
// #include "Libraries/WiFiManager/WiFiManager.h"
#include <WiFiManager.h>

#define WIFI_CONNECT_COUNTDOWN_MAX 180
#define WIFI_HOTSPOT_PASSWORD_DEFAULT "cleanair"

#define WIFI() ((WiFiManager *)(this->wifi))

AgWiFiConnector::AgWiFiConnector(AirGradient &ag, AgOledDisplay &disp,
                                 String ssid, Stream &log, AgStateMachine &sm)
    : PrintLog(log, "AgWiFiConnector"), ag(ag), disp(disp), ssid(ssid), sm(sm) {
}

AgWiFiConnector::~AgWiFiConnector() {}

void AgWiFiConnector::setHotspotSSID(String ssid) { this->ssid = ssid; }

bool AgWiFiConnector::connect(uint32_t timeout) {
  if (wifi == NULL) {
    wifi = new WiFiManager();
    if (wifi == NULL) {
      logError("Create 'WiFiManger' failed");
      return false;
    }
  }

  WIFI()->setConfigPortalBlocking(false);
  WIFI()->setTimeout(timeout);

  WIFI()->setAPCallback([this](WiFiManager *obj) { _wifiApCallback(); });
  WIFI()->setSaveConfigCallback([this]() { _wifiSaveConfig(); });
  WIFI()->setSaveParamsCallback([this]() { _wifiSaveParamCallback(); });

  if (ag.isOneIndoor()) {
    disp.setText("Connecting to", "WiFi", "...");
  } else {
    logInfo("Connecting to WiFi...");
  }

  WIFI()->autoConnect(ssid.c_str(), WIFI_HOTSPOT_PASSWORD_DEFAULT);
  xTaskCreate(
      [](void *obj) {
        AgWiFiConnector *connector = (AgWiFiConnector *)obj;
        while (connector->_wifiConfigPortalActive()) {
          connector->_wifiProcess();
        }
        vTaskDelete(NULL);
      },
      "wifi_cfg", 4096, this, 10, NULL);

  /** Wait for WiFi connect and show LED, display status */
  uint32_t dispPeriod = millis();
  uint32_t ledPeriod = millis();
  bool clientConnectChanged = false;

  AgStateMachineState stateOld = sm.getDisplayState();
  while (WIFI()->getConfigPortalActive()) {
    /** LED and display animation */
    if (WiFi.isConnected() == false) {
      /** Display countdown */
      uint32_t ms = (uint32_t)(millis() - dispPeriod);
      if (ag.isOneIndoor()) {
        if (ms >= 1000) {
          dispPeriod = millis();
          sm.displayHandle();
        } else {
          if (stateOld != sm.getDisplayState()) {
            stateOld = sm.getDisplayState();
            sm.displayHandle();
          }
        }
      }

      /** LED animations */
      ms = (uint32_t)(millis() - ledPeriod);
      if (ms >= 100) {
        ledPeriod = millis();
        sm.ledHandle();
      }

      /** Check for client connect to change led color */
      bool clientConnected = wifiClientConnected();
      if (clientConnected != clientConnectChanged) {
        clientConnectChanged = clientConnected;
        if (clientConnectChanged) {
          sm.ledHandle(AgStateMachineWiFiManagerPortalActive);
          // ledSmHandler(AgStateMachineWiFiManagerPortalActive);
        } else {
          // ledCount = LED_BAR_COUNT_INIT_VALUE;
          sm.ledAnimationInit();
          // ledSmHandler(AgStateMachineWiFiManagerMode);
          sm.ledHandle(AgStateMachineWiFiManagerMode);
          if (ag.isOneIndoor()) {
            // dispSmHandler(AgStateMachineWiFiManagerMode);
            sm.displayHandle(AgStateMachineWiFiManagerMode);
          }
        }
      }
    }
  }

  /** Show display wifi connect result failed */
  if (WiFi.isConnected() == false) {
    sm.ledHandle(AgStateMachineWiFiManagerConnectFailed);
    // ledSmHandler(AgStateMachineWiFiManagerConnectFailed);
    if (ag.isOneIndoor()) {
      // dispSmHandler(AgStateMachineWiFiManagerConnectFailed);
      sm.displayHandle(AgStateMachineWiFiManagerConnectFailed);
    }
    delay(6000);
  } else {
    wifiHasConfig = true;
  }

  /** Update LED bar color */
  appLedHandler();
}

bool AgWiFiConnector::wifiClientConnected(void) {
  return WiFi.softAPgetStationNum() ? true : false;
}

void AgWiFiConnector::_wifiApCallback(void) {
  sm.displayWiFiConnectCountDown(WIFI_CONNECT_COUNTDOWN_MAX);
  sm.ledAnimationInit();
  sm.setDisplayState(AgStateMachineWiFiManagerMode);
}

void AgWiFiConnector::_wifiSaveConfig(void) {
  sm.setDisplayState(AgStateMachineWiFiManagerStaConnected);
  sm.ledHandle();
  // sm.displayHandle();
}

void AgWiFiConnector::_wifiSaveParamCallback(void) {
  sm.ledAnimationInit();
  sm.setDisplayState(AgStateMachineWiFiManagerStaConnecting);
}

bool AgWiFiConnector::_wifiConfigPortalActive(void) {
  return WIFI()->getConfigPortalActive();
}

void AgWiFiConnector::_wifiProcess() { WIFI()->process(); }
