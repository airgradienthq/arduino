#include "AgWiFiConnector.h"
#include "Arduino.h"
#include "Libraries/WiFiManager/WiFiManager.h"
#include "Libraries/Arduino_JSON/src/Arduino_JSON.h"

#define WIFI_CONNECT_COUNTDOWN_MAX 180
#define WIFI_HOTSPOT_PASSWORD_DEFAULT "cleanair"


#define BLE_SERVICE_UUID "acbcfea8-e541-4c40-9bfd-17820f16c95c"
#define BLE_CRED_CHAR_UUID "703fa252-3d2a-4da9-a05c-83b0d9cacb8e"
#define BLE_SCAN_CHAR_UUID "467a080f-e50f-42c9-b9b2-a2ab14d82725"

#define BLE_CRED_BIT (1 << 0)
#define BLE_SCAN_BIT (1 << 1)

#define WIFI() ((WiFiManager *)(this->wifi))

/**
 * @brief Set reference AirGradient instance
 *
 * @param ag Point to AirGradient instance
 */
void WifiConnector::setAirGradient(AirGradient *ag) { this->ag = ag; }

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

  WiFi.begin();
  String wifiSSID = WIFI()->getWiFiSSID(true);
  if (wifiSSID.isEmpty()) {
    logInfo("Connected WiFi is empty, connect to default wifi \"" +
            String(this->defaultSsid) + String("\""));

    /** Set wifi connect */
    WiFi.begin(this->defaultSsid, this->defaultPassword);

    /** Wait for wifi connect to AP */
    int count = 0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      count++;
      if (count >= 15) {
        logError("Try connect to default wifi \"" + String(this->defaultSsid) +
                 String("\" failed"));
        break;
      }
    }
  }

  WIFI()->setConfigPortalBlocking(false);
  WIFI()->setConnectTimeout(15);
  WIFI()->setTimeout(WIFI_CONNECT_COUNTDOWN_MAX);

  WIFI()->setAPCallback([this](WiFiManager *obj) { _wifiApCallback(); });
  WIFI()->setSaveConfigCallback([this]() { _wifiSaveConfig(); });
  WIFI()->setSaveParamsCallback([this]() { _wifiSaveParamCallback(); });
  WIFI()->setConfigPortalTimeoutCallback([this]() {_wifiTimeoutCallback();});
  if (ag->isOne() || (ag->isPro4_2()) || ag->isPro3_3() || ag->isBasic()) {
    disp.setText("Connecting to", "WiFi", "...");
  } else {
    logInfo("Connecting to WiFi...");
  }
  ssid = "airgradient-" + ag->deviceId();

  // ssid = "AG-" + String(ESP.getChipId(), HEX);
  WIFI()->setConfigPortalTimeout(WIFI_CONNECT_COUNTDOWN_MAX);

  WiFiManagerParameter disableCloud("chbPostToAg", "Prevent Connection to AirGradient Server", "T",
                                    2, "type=\"checkbox\" ", WFM_LABEL_AFTER);
  WIFI()->addParameter(&disableCloud);
  WiFiManagerParameter disableCloudInfo(
      "<p>Prevent connection to the AirGradient Server. Important: Only enable "
      "it if you are sure you don't want to use any AirGradient cloud "
      "features. As a result you will not receive automatic firmware updates, "
      "configuration settings from cloud and the measure data will not reach the AirGradient dashboard.</p>");
  WIFI()->addParameter(&disableCloudInfo);

  WIFI()->autoConnect(ssid.c_str(), WIFI_HOTSPOT_PASSWORD_DEFAULT);

  logInfo("Wait for configure portal");

#ifdef ESP32
  // Task handle WiFi connection.
  xTaskCreate(
      [](void *obj) {
        WifiConnector *connector = (WifiConnector *)obj;
        while (connector->_wifiConfigPortalActive()) {
          if (connector->isBleClientConnected()) {
            Serial.println("Stopping portal because BLE connected");
            connector->_wifiStop();
            connector->provisionMethod = ProvisionMethod::BLE;
            break;
          }
          connector->_wifiProcess();
          vTaskDelay(1);
        }
        vTaskDelete(NULL);
      },
      "wifi_cfg", 4096, this, 10, NULL);

  /** Wait for WiFi connect and show LED, display status */
  uint32_t dispPeriod = millis();
  uint32_t ledPeriod = millis();
  bool clientConnectChanged = false;

  setupBLE();

  AgStateMachineState stateOld = sm.getDisplayState();
  while (WIFI()->getConfigPortalActive()) {
    /** LED animation and display update content */
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
          if (bleServerRunning) {
            Serial.println("Stopping BLE since wifi is connected");
            stopBLE();
            provisionMethod = ProvisionMethod::WiFi;
          }
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
#else
  _wifiProcess();
#endif

  if (provisionMethod == ProvisionMethod::BLE) {
    disp.setText("Provision by", "BLE", "");

    int count = 0;

    // Loop until the BLE client disconnected or WiFi connected
    while (isBleClientConnected() && !WiFi.isConnected()) {
      Serial.println("Wait for BLE provision command");
      EventBits_t bits = xEventGroupWaitBits(
        bleEventGroup,
        BLE_SCAN_BIT | BLE_CRED_BIT,
        pdTRUE,
        pdFALSE,
        portMAX_DELAY
      );

      if (bits & BLE_CRED_BIT) {
        count = 0;
        wifiConnecting = true;
        Serial.printf("Connecting to %s...\n", ssid.c_str());
        while (WiFi.status() != WL_CONNECTED) {
          delay(1000);
          Serial.print(".");
          count++;
          if (count >= 15) {
            WiFi.disconnect();
            wifiConnecting = false;
            bleNotifyStatus(10);
            break;
          }
        }
      }
      else if (bits & BLE_SCAN_BIT) {
        String result = scanFilteredWiFiJSON();
        NimBLEService* pSvc = pServer->getServiceByUUID(BLE_SERVICE_UUID);
        if (pSvc) {
          NimBLECharacteristic* pChr = pSvc->getCharacteristic(BLE_SCAN_CHAR_UUID);
          if (pChr) {
            pChr->setValue(result);
            pChr->notify();
            Serial.println("List of scanned networks sent through BLE notify");
          }
        }
      }
    }

    Serial.println("Exit provision by BLE");
  }

  /** Show display wifi connect result failed */
  if (WiFi.isConnected() == false) {
    sm.handleLeds(AgStateMachineWiFiManagerConnectFailed);
    if (ag->isOne() || ag->isPro4_2() || ag->isPro3_3() || ag->isBasic()) {
      sm.displayHandle(AgStateMachineWiFiManagerConnectFailed);
    }
    delay(6000);
  } else {
    hasConfig = true;
    logInfo("WiFi Connected: " + WiFi.SSID() + " IP: " + localIpStr());

    if (hasPortalConfig) {
      String result = String(disableCloud.getValue());
      logInfo("Setting disableCloudConnection set from " +
              String(config.isCloudConnectionDisabled() ? "True" : "False") + String(" to ") +
              String(result == "T" ? "True" : "False") + String(" successful"));
      config.setDisableCloudConnection(result == "T");
    }
    hasPortalConfig = false;
    bleNotifyStatus(0);
  }

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

/**
 * @brief Has wifi STA connected to WIFI softAP (this device)
 *
 * @return true Connected
 * @return false Not connected
 */
bool WifiConnector::wifiClientConnected(void) {
  return WiFi.softAPgetStationNum() ? true : false;
}


bool WifiConnector::isBleClientConnected() {
  return bleClientConnected;
}

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
void WifiConnector::_wifiTimeoutCallback(void) { connectorTimeout = true; }

void WifiConnector::_wifiStop() {
  WIFI()->stopConfigPortal();
}

/**
 * @brief Process WiFiManager connection
 *
 */
void WifiConnector::_wifiProcess() {
#ifdef ESP32
  WIFI()->process();
#else
  /** Wait for WiFi connect and show LED, display status */
  uint32_t dispPeriod = millis();
  uint32_t ledPeriod = millis();
  bool clientConnectChanged = false;
  AgStateMachineState stateOld = sm.getDisplayState();

  while (WIFI()->getConfigPortalActive()) {
    WIFI()->process();

    if (WiFi.isConnected() == false) {
      /** Display countdown */
      uint32_t ms;
      if (ag->isOne() || (ag->isPro4_2()) || ag->isPro3_3() || ag->isBasic()) {
        ms = (uint32_t)(millis() - dispPeriod);
        if (ms >= 1000) {
          dispPeriod = millis();
          sm.displayHandle();
          logInfo("displayHandle state: " + String(sm.getDisplayState()));
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

    delay(1);
  }

  // TODO This is for basic
  if (ag->getBoardType() == DIY_BASIC) {
    if (!WiFi.isConnected()) {
      // disp.setText("Booting", "offline", "mode");
      Serial.println("failed to connect and hit timeout");
      delay(2500);
    } else {
      hasConfig = true;
    }
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
void WifiConnector::reset(void) {
  if(this->wifi == NULL) {
    this->wifi = new WiFiManager();
    if(this->wifi == NULL){
      logInfo("reset failed");
      return;
    }
  }
  WIFI()->resetSettings();
}

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

/**
 * @brief Get status that wifi has configurated
 *
 * @return true Configurated
 * @return false Not Configurated
 */
bool WifiConnector::hasConfigurated(void) {
  if (WiFi.SSID().isEmpty()) {
    return false;
  }
  return true;
}

/**
 * @brief Get WiFi connection porttal timeout.
 *
 * @return true
 * @return false
 */
bool WifiConnector::isConfigurePorttalTimeout(void) { return connectorTimeout; }


void WifiConnector::bleNotifyStatus(int status) {
  if (!bleServerRunning) {
    return;
  }

  if (pServer->getConnectedCount()) {
    NimBLEService* pSvc = pServer->getServiceByUUID(BLE_SERVICE_UUID);
    if (pSvc) {
      NimBLECharacteristic* pChr = pSvc->getCharacteristic(BLE_CRED_CHAR_UUID);
      if (pChr) {
        char tosend[50];
        memset(tosend, 0, 50);
        sprintf(tosend, "{\"status\":%d}", status);
        Serial.printf("BLE Notify >> %s \n", tosend);
        pChr->setValue(String(tosend));
        pChr->notify();
      }
    }
  }
}

/**
 * @brief Set wifi connect to default WiFi
 *
 */
void WifiConnector::setDefault(void) {
  WiFi.begin("airgradient", "cleanair");
}

String WifiConnector::scanFilteredWiFiJSON() {
  Serial.println("Scanning for Wi-Fi networks...");
  int n = WiFi.scanNetworks(false, true);  // async=false, show_hidden=true
  Serial.printf("Found %d networks\n", n);

  const int MAX_NETWORKS = 50;
  const int MAX_RESULTS = 15;

  if (n <= 0) {
    Serial.println("No networks found");
    return "[]";
  }

  WiFiNetwork allNetworks[MAX_NETWORKS];
  int allCount = 0;

  // Collect valid networks (filter weak or empty SSID)
  for (int i = 0; i < n && allCount < MAX_NETWORKS; ++i) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);

    if (ssid.length() == 0 || rssi < -70) continue;

    allNetworks[allCount++] = {ssid, rssi, open};
  }

  // Remove duplicates (keep the strongest)
  WiFiNetwork uniqueNetworks[MAX_NETWORKS];
  int uniqueCount = 0;

  for (int i = 0; i < allCount; i++) {
    bool exists = false;
    for (int j = 0; j < uniqueCount; j++) {
      if (uniqueNetworks[j].ssid == allNetworks[i].ssid) {
        exists = true;
        if (allNetworks[i].rssi > uniqueNetworks[j].rssi)
          uniqueNetworks[j] = allNetworks[i]; // keep stronger one
        break;
      }
    }
    if (!exists && uniqueCount < MAX_NETWORKS) {
      uniqueNetworks[uniqueCount++] = allNetworks[i];
    }
  }

  // Sort by RSSI descending (simple bubble sort for small lists)
  for (int i = 0; i < uniqueCount - 1; i++) {
    for (int j = i + 1; j < uniqueCount; j++) {
      if (uniqueNetworks[j].rssi > uniqueNetworks[i].rssi) {
        WiFiNetwork temp = uniqueNetworks[i];
        uniqueNetworks[i] = uniqueNetworks[j];
        uniqueNetworks[j] = temp;
      }
    }
  }

  // Limit to top X
  if (uniqueCount > MAX_RESULTS)
    uniqueCount = MAX_RESULTS;

  // Build JSON array
  JSONVar jsonArray;
  for (int i = 0; i < uniqueCount; i++) {
    JSONVar obj;
    obj["ssid"] = uniqueNetworks[i].ssid;
    obj["rssi"] = uniqueNetworks[i].rssi;
    obj["open"] = uniqueNetworks[i].open;
    jsonArray[i] = obj;
  }

  String jsonString = JSON.stringify(jsonArray);

  Serial.println("Filtered Wi-Fi Networks (JSON):");
  Serial.println(jsonString);

  return jsonString;
}


void WifiConnector::setupBLE() {
  Serial.printf("Setup BLE with device name %s\n", ssid.c_str());
  NimBLEDevice::init(ssid.c_str());
  NimBLEDevice::setPower(3); /** +3db */

  /** bonding, MITM, don't need BLE secure connections as we are using passkey pairing */
  NimBLEDevice::setSecurityAuth(false, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks(this));


  auto characteristicCallback = new CharacteristicCallbacks(this);

  NimBLEService *pService = pServer->createService(BLE_SERVICE_UUID);
  NimBLECharacteristic *pCredentialCharacteristic =
      pService->createCharacteristic(BLE_CRED_CHAR_UUID,
                                     NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC |
                                         NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::NOTIFY);
  pCredentialCharacteristic->setCallbacks(characteristicCallback);

  NimBLECharacteristic *pScanCharacteristic =
      pService->createCharacteristic(BLE_SCAN_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::NOTIFY);
  pScanCharacteristic->setCallbacks(characteristicCallback);


  pService->start();

  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->start();
  bleServerRunning = true;

  // Create event group
  bleEventGroup = xEventGroupCreate();
  if (bleEventGroup == NULL) {
    Serial.println("Failed to create BLE event group!");
    // This case is very unlikely
  }

  Serial.println("Provision by BLE ready");
}

void WifiConnector::stopBLE() {
  if (bleServerRunning) {
    Serial.println("Stopping BLE");
    NimBLEDevice::deinit();
  } 
  bleServerRunning = false;
}

//
// BLE innerclass implementation
//

WifiConnector::ServerCallbacks::ServerCallbacks(WifiConnector* parent)
    : parent(parent) {}

void WifiConnector::ServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
  Serial.printf("Client address: %s\n", connInfo.getAddress().toString().c_str());
  parent->bleClientConnected = true;
  NimBLEDevice::stopAdvertising();
}

void WifiConnector::ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
  Serial.printf("Client disconnected - start advertising\n");
  NimBLEDevice::startAdvertising();
  parent->bleClientConnected = false;
}

void WifiConnector::ServerCallbacks::onAuthenticationComplete(NimBLEConnInfo& connInfo) {
  Serial.println("\n========== PAIRING COMPLETE ==========");
  Serial.printf("Peer Address: %s\n", connInfo.getAddress().toString().c_str());
  Serial.printf("Encrypted: %s\n", connInfo.isEncrypted() ? "YES" : "NO");
  Serial.printf("Authenticated: %s\n", connInfo.isAuthenticated() ? "YES" : "NO");
  Serial.printf("Key Size: %d bits\n", connInfo.getSecKeySize() * 8);
  Serial.println("======================================\n");
}

WifiConnector::CharacteristicCallbacks::CharacteristicCallbacks(WifiConnector* parent)
    : parent(parent) {}

void WifiConnector::CharacteristicCallbacks::onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) {
  Serial.printf("%s : onRead(), value: %s\n", pCharacteristic->getUUID().toString().c_str(),
                pCharacteristic->getValue().c_str());
}

void WifiConnector::CharacteristicCallbacks::onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) {
  Serial.printf("%s : onWrite(), value: %s\n", pCharacteristic->getUUID().toString().c_str(),
                pCharacteristic->getValue().c_str());

  auto bleCred = NimBLEUUID(BLE_CRED_CHAR_UUID);
  if (pCharacteristic->getUUID().equals(bleCred)) {
    if (!parent->wifiConnecting) {
      JSONVar root = JSON.parse(pCharacteristic->getValue().c_str());

      String ssid = root["ssid"];
      String pass = root["password"];

      WiFi.begin(ssid.c_str(), pass.c_str());
      xEventGroupSetBits(parent->bleEventGroup, BLE_CRED_BIT);
    }
  } else {
    xEventGroupSetBits(parent->bleEventGroup, BLE_SCAN_BIT);
  }

}

