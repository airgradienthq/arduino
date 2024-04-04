#include "AgWiFiConnector.h"
#include <WiFiManager.h>

#define WIFI_CONNECT_COUNTDOWN_MAX 180
#define WIFI_HOTSPOT_PASSWORD_DEFAULT "cleanair"

#define WIFI() ((WiFiManager *)(this->wifi))

AgWiFiConnector::AgWiFiConnector(AgOledDisplay &disp, Stream &log,
                                 AgStateMachine &sm)
    : PrintLog(log, "AgWiFiConnector"), disp(disp), sm(sm) {}

AgWiFiConnector::~AgWiFiConnector() {}

void AgWiFiConnector::setAirGradient(AirGradient *ag) { this->ag = ag; }

bool AgWiFiConnector::connect(void) {
  if (wifi == NULL) {
    wifi = new WiFiManager();
    if (wifi == NULL) {
      logError("Create 'WiFiManger' failed");
      return false;
    }
  }

  WIFI()->setConfigPortalBlocking(false);
  WIFI()->setTimeout(WIFI_CONNECT_COUNTDOWN_MAX);

  WIFI()->setAPCallback([this](WiFiManager *obj) { _wifiApCallback(); });
  WIFI()->setSaveConfigCallback([this]() { _wifiSaveConfig(); });
  WIFI()->setSaveParamsCallback([this]() { _wifiSaveParamCallback(); });

  if (ag->isOneIndoor()) {
    disp.setText("Connecting to", "WiFi", "...");
  } else {
    logInfo("Connecting to WiFi...");
  }

  WIFI()->autoConnect(ssid.c_str(), WIFI_HOTSPOT_PASSWORD_DEFAULT);

  // Task handle WiFi connection.
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
    /** LED animatoin and display update content */
    if (WiFi.isConnected() == false) {
      /** Display countdown */
      uint32_t ms;
      if (ag->isOneIndoor()) {
        ms = (uint32_t)(millis() - dispPeriod);
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
        } else {
          sm.ledAnimationInit();
          sm.ledHandle(AgStateMachineWiFiManagerMode);
          if (ag->isOneIndoor()) {
            sm.displayHandle(AgStateMachineWiFiManagerMode);
          }
        }
      }
    }

    delay(1); // avoid watchdog timer reset.
  }

  /** Show display wifi connect result failed */
  if (WiFi.isConnected() == false) {
    sm.ledHandle(AgStateMachineWiFiManagerConnectFailed);
    if (ag->isOneIndoor()) {
      sm.displayHandle(AgStateMachineWiFiManagerConnectFailed);
    }
    delay(6000);
  } else {
    hasConfig = true;
  }

  return true;
}

bool AgWiFiConnector::wifiClientConnected(void) {
  return WiFi.softAPgetStationNum() ? true : false;
}

void AgWiFiConnector::_wifiApCallback(void) {
  sm.displayWiFiConnectCountDown(WIFI_CONNECT_COUNTDOWN_MAX);
  sm.setDisplayState(AgStateMachineWiFiManagerMode);
  sm.ledAnimationInit();
  sm.ledHandle(AgStateMachineWiFiManagerMode);
}

void AgWiFiConnector::_wifiSaveConfig(void) {
  sm.setDisplayState(AgStateMachineWiFiManagerStaConnected);
  sm.ledHandle(AgStateMachineWiFiManagerStaConnected);
}

void AgWiFiConnector::_wifiSaveParamCallback(void) {
  sm.ledAnimationInit();
  sm.ledHandle(AgStateMachineWiFiManagerStaConnecting);
  sm.setDisplayState(AgStateMachineWiFiManagerStaConnecting);
}

bool AgWiFiConnector::_wifiConfigPortalActive(void) {
  return WIFI()->getConfigPortalActive();
}

void AgWiFiConnector::_wifiProcess() { WIFI()->process(); }

void AgWiFiConnector::handle(void) {
  // Ignore if WiFi is not configured
  if (hasConfig == false) {
    return;
  }

  if (WiFi.isConnected()) {
    lastRetry = millis();
    return;
  }

  /** Retry connect WiFi each 10sec */
  uint32_t ms = (uint32_t)(millis() - lastRetry);
  if (ms >= 10000) {
    lastRetry = millis();
    WiFi.reconnect();

    // Serial.printf("Re-Connect WiFi\r\n");
    logInfo("Re-Connect WiFi");
  }
}

bool AgWiFiConnector::isConnected(void) { return WiFi.isConnected(); }
