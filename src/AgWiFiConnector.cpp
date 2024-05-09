#include "AgWiFiConnector.h"
#include "Libraries/WiFiManager/WiFiManager.h"
#include <time.h>

#define WIFI_CONNECT_COUNTDOWN_MAX 180
#define WIFI_HOTSPOT_PASSWORD_DEFAULT "cleanair"

#define WIFI() ((WiFiManager *)(this->wifi))

/**
 * @brief Set reference AirGradient instance
 *
 * @param ag Point to AirGradient instance
 */
void WifiConnector::setAirGradient(AirGradient *ag) { this->ag = ag; }

#ifdef ESP32
/**
 * @brief Construct a new Ag Wi Fi Connector:: Ag Wi Fi Connector object
 *
 * @param disp OledDisplay
 * @param log Stream
 * @param sm StateMachine
 */
WifiConnector::WifiConnector(OledDisplay &disp, Stream &log, StateMachine &sm,
                             Configuration &config)
    : PrintLog(log, "WifiConnector"), disp(disp), sm(sm), config(config) {}
#else
WifiConnector::WifiConnector(Stream &log) : PrintLog(log, "WiFiConnector") {}
#endif

WifiConnector::~WifiConnector() {}

/**
 * @brief Connection to WIFI AP process. Just call one times
 *
 * @return true Success
 * @return false Failure
 */
bool WifiConnector::connect(void) {
  if (wifi == NULL) {
    wifi = new WiFiManager();
    if (wifi == NULL) {
      logError("Create 'WiFiManger' failed");
      return false;
    }
  }

  WIFI()->setConfigPortalBlocking(false);
  WIFI()->setConnectTimeout(15);
  WIFI()->setTimeout(WIFI_CONNECT_COUNTDOWN_MAX);

#ifdef ESP32
  WIFI()->setAPCallback([this](WiFiManager *obj) { _wifiApCallback(); });
  WIFI()->setSaveConfigCallback([this]() { _wifiSaveConfig(); });
  WIFI()->setSaveParamsCallback([this]() { _wifiSaveParamCallback(); });
  if (ag->isOne()) {
    disp.setText("Connecting to", "WiFi", "...");
  } else {
    logInfo("Connecting to WiFi...");
  }
  ssid = "airgradient-" + ag->deviceId();
#else
  ssid = "AG-" + String(ESP.getChipId(), HEX);
#endif
  WIFI()->setConfigPortalTimeout(WIFI_CONNECT_COUNTDOWN_MAX);

  WiFiManagerParameter postToAg("chbPostToAg",
                                "Prevent Connection to AirGradient Server", "T",
                                2, "type=\"checkbox\" ", WFM_LABEL_AFTER);
  WIFI()->addParameter(&postToAg);
  WiFiManagerParameter postToAgInfo(
      "<p>Prevent connection to the AirGradient Server. Important: Only enable "
      "it if you are sure you don't want to use any AirGradient cloud "
      "features. As a result you will not receive automatic firmware updates "
      "and your data will not reach the AirGradient dashboard.</p>");
  WIFI()->addParameter(&postToAgInfo);

  WIFI()->autoConnect(ssid.c_str(), WIFI_HOTSPOT_PASSWORD_DEFAULT);

#ifdef ESP32
  // Task handle WiFi connection.
  xTaskCreate(
      [](void *obj) {
        WifiConnector *connector = (WifiConnector *)obj;
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
      if (ag->isOne()) {
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
        sm.handleLeds();
      }

      /** Check for client connect to change led color */
      bool clientConnected = wifiClientConnected();
      if (clientConnected != clientConnectChanged) {
        clientConnectChanged = clientConnected;
        if (clientConnectChanged) {
          sm.handleLeds(AgStateMachineWiFiManagerPortalActive);
        } else {
          sm.ledAnimationInit();
          sm.handleLeds(AgStateMachineWiFiManagerMode);
          if (ag->isOne()) {
            sm.displayHandle(AgStateMachineWiFiManagerMode);
          }
        }
      }
    }

    delay(1); // avoid watchdog timer reset.
  }
  /** Show display wifi connect result failed */
  if (WiFi.isConnected() == false) {
    sm.handleLeds(AgStateMachineWiFiManagerConnectFailed);
    if (ag->isOne()) {
      sm.displayHandle(AgStateMachineWiFiManagerConnectFailed);
    }
    delay(6000);
  } else {
    hasConfig = true;
    logInfo("WiFi Connected: " + WiFi.SSID() + " IP: " + localIpStr());

    if (hasPortalConfig) {
      String result = String(postToAg.getValue());
      logInfo("Setting postToAirGradient set from " +
              String(config.isPostDataToAirGradient() ? "True" : "False") +
              String(" to ") + String(result != "T" ? "True" : "False") +
              String(" successful"));
      config.setPostToAirGradient(result != "T");
    }
    hasPortalConfig = false;

    /** Configure internet time */
    const char *ntp_server = "pool.ntp.org";
    configTime(0, 0, ntp_server);
    logInfo("Set internet time server: " + String(ntp_server));
  }
#else
  _wifiProcess();
#endif
  return true;
}

/**
 * @brief Disconnect to current connected WiFi AP
 *
 */
void WifiConnector::disconnect(void) {
  if (WiFi.isConnected()) {
    logInfo("Disconnect");
    WiFi.disconnect();
  }
}

#ifdef ESP32
#else
void WifiConnector::displayShowText(String ln1, String ln2, String ln3) {
  char buf[9];
  ag->display.clear();

  ag->display.setCursor(1, 1);
  ag->display.setText(ln1);
  ag->display.setCursor(1, 19);
  ag->display.setText(ln2);
  ag->display.setCursor(1, 37);
  ag->display.setText(ln3);

  ag->display.show();
  delay(100);
}
#endif

/**
 * @brief Has wifi STA connected to WIFI softAP (this device)
 *
 * @return true Connected
 * @return false Not connected
 */
bool WifiConnector::wifiClientConnected(void) {
  return WiFi.softAPgetStationNum() ? true : false;
}

#ifdef ESP32
/**
 * @brief Handle WiFiManage softAP setup completed callback
 *
 */
void WifiConnector::_wifiApCallback(void) {
  sm.displayWiFiConnectCountDown(WIFI_CONNECT_COUNTDOWN_MAX);
  sm.setDisplayState(AgStateMachineWiFiManagerMode);
  sm.ledAnimationInit();
  sm.handleLeds(AgStateMachineWiFiManagerMode);
}

/**
 * @brief Handle WiFiManager save configuration callback
 *
 */
void WifiConnector::_wifiSaveConfig(void) {
  sm.setDisplayState(AgStateMachineWiFiManagerStaConnected);
  sm.handleLeds(AgStateMachineWiFiManagerStaConnected);
}

/**
 * @brief Handle WiFiManager save parameter callback
 *
 */
void WifiConnector::_wifiSaveParamCallback(void) {
  sm.ledAnimationInit();
  sm.handleLeds(AgStateMachineWiFiManagerStaConnecting);
  sm.setDisplayState(AgStateMachineWiFiManagerStaConnecting);
  hasPortalConfig = true;
}

/**
 * @brief Check that WiFiManager Configure portal active
 *
 * @return true Active
 * @return false Not-Active
 */
bool WifiConnector::_wifiConfigPortalActive(void) {
  return WIFI()->getConfigPortalActive();
}
#endif
/**
 * @brief Process WiFiManager connection
 *
 */
void WifiConnector::_wifiProcess() {
#ifdef ESP32
  WIFI()->process();
#else
  int count = WIFI_CONNECT_COUNTDOWN_MAX;
  displayShowText(String(WIFI_CONNECT_COUNTDOWN_MAX) + " sec", "SSID:", ssid);
  while (WIFI()->getConfigPortalActive()) {
    WIFI()->process();

    uint32_t lastTime = millis();
    uint32_t ms = (uint32_t)(millis() - lastTime);
    if (ms >= 1000) {
      lastTime = millis();

      displayShowText(String(count) + " sec", "SSID:", ssid);

      count--;

      // Timeout
      if (count == 0) {
        break;
      }
    }
  }

  if (!WiFi.isConnected()) {
    displayShowText("Booting", "offline", "mode");
    Serial.println("failed to connect and hit timeout");
    delay(2500);
  } else {
    hasConfig = true;
  }
#endif
}

/**
 * @brief Handle and reconnect WiFi
 *
 */
void WifiConnector::handle(void) {
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

/**
 * @brief Is WiFi connected
 *
 * @return true  Connected
 * @return false Disconnected
 */
bool WifiConnector::isConnected(void) { return WiFi.isConnected(); }

/**
 * @brief Reset WiFi configuretion and connection, disconnect wifi before call
 * this method
 *
 */
void WifiConnector::reset(void) { WIFI()->resetSettings(); }

/**
 * @brief Get wifi RSSI
 *
 * @return int
 */
int WifiConnector::RSSI(void) { return WiFi.RSSI(); }

/**
 * @brief Get WIFI IP as string format ex: 192.168.1.1
 *
 * @return String
 */
String WifiConnector::localIpStr(void) { return WiFi.localIP().toString(); }
