/*
This is the code for the AirGradient Open Air open-source hardware outdoor Air
Quality Monitor with an ESP32-C3 Microcontroller.

It is an air quality monitor for PM2.5, CO2, TVOCs, NOx, Temperature and
Humidity and can send data over Wifi.

Open source air quality monitors and kits are available:
Indoor Monitor: https://www.airgradient.com/indoor/
Outdoor Monitor: https://www.airgradient.com/outdoor/

Build Instructions:
https://www.airgradient.com/documentation/open-air-pst-kit-1-3/

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.16-rc.2
"Arduino_JSON" by Arduino Version 0.2.0

Please make sure you have esp32 board manager installed. Tested with
version 2.0.11.

Important flashing settings:
- Set board to "ESP32C3 Dev Module"
- Enable "USB CDC On Boot"
- Flash frequency "80Mhz"
- Flash mode "QIO"
- Flash size "4MB"
- Partition scheme "Default 4MB with spiffs (1.2MB APP/1,5MB SPIFFS)"
- JTAG adapter "Disabled"

If you have any questions please visit our forum at
https://forum.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include "EEPROM.h"
#include "mqtt_client.h"
#include <AirGradient.h>
#include <Arduino_JSON.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <Wire.h>

/**
 *
 * @brief Application state machine state
 *
 */
enum {
  APP_SM_WIFI_MANAGER_MODE,           /** In WiFi Manger Mode */
  APP_SM_WIFI_MANAGER_PORTAL_ACTIVE,  /** WiFi Manager has connected to mobile
                                         phone */
  APP_SM_WIFI_MANAGER_STA_CONNECTING, /** After SSID and PW entered and OK
                                        clicked, connection to WiFI network is
                                        attempted*/
  APP_SM_WIFI_MANAGER_STA_CONNECTED,  /** Connecting to WiFi worked */
  APP_SM_WIFI_OK_SERVER_CONNECTING,   /** Once connected to WiFi an attempt to
                                         reach the server is performed */
  APP_SM_WIFI_OK_SERVER_CONNECTED,    /** Server is reachable, all ﬁne */
  /** Exceptions during WIFi Setup */
  APP_SM_WIFI_MANAGER_CONNECT_FAILED,   /** Cannot connect to WiFi (e.g. wrong
                                            password, WPA Enterprise etc.) */
  APP_SM_WIFI_OK_SERVER_CONNECT_FAILED, /** Connected to WiFi but server not
                                          reachable, e.g. firewall block/
                                          whitelisting needed etc. */
  APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED, /** Server reachable but sensor
                                                   not configured correctly*/

  /** During Normal Operation */
  APP_SM_WIFI_LOST, /** Connection to WiFi network failed credentials incorrect
                       encryption not supported etc. */
  APP_SM_SERVER_LOST, /** Connected to WiFi network but the server cannot be
                         reached through the internet, e.g. blocked by firewall
                       */
  APP_SM_SENSOR_CONFIG_FAILED, /** Server is reachabFirmware nodele but there is
                                  some conﬁguration issue to be fixed on the
                                  server side */
  APP_SM_NORMAL,
};

#define LED_FAST_BLINK_DELAY 250             /** ms */
#define LED_SLOW_BLINK_DELAY 1000            /** ms */
#define LED_SHORT_BLINK_DELAY 500            /** ms */
#define LED_LONG_BLINK_DELAY 2000            /** ms */
#define WIFI_CONNECT_COUNTDOWN_MAX 180       /** sec */
#define WIFI_CONNECT_RETRY_MS 10000          /** ms */
#define LED_BAR_COUNT_INIT_VALUE (-1)        /** */
#define LED_BAR_ANIMATION_PERIOD 100         /** ms */
#define DISP_UPDATE_INTERVAL 5000            /** ms */
#define SERVER_CONFIG_UPDATE_INTERVAL 15000  /** ms */
#define SERVER_SYNC_INTERVAL 60000           /** ms */
#define MQTT_SYNC_INTERVAL 60000             /** ms */
#define SENSOR_CO2_CALIB_COUNTDOWN_MAX 5     /** sec */
#define SENSOR_TVOC_UPDATE_INTERVAL 1000     /** ms */
#define SENSOR_CO2_UPDATE_INTERVAL 4000      /** ms */
#define SENSOR_PM_UPDATE_INTERVAL 2000       /** ms */
#define SENSOR_TEMP_HUM_UPDATE_INTERVAL 5000 /** ms */
#define DISPLAY_DELAY_SHOW_CONTENT_MS 2000   /** ms */
#define WIFI_HOTSPOT_PASSWORD_DEFAULT                                          \
  "cleanair" /** default WiFi AP password                                      \
              */

/**
 * @brief Use use LED bar state
 */
typedef enum {
  UseLedBarOff, /** Don't use LED bar */
  UseLedBarPM,  /** Use LED bar for PMS */
  UseLedBarCO2, /** Use LED bar for CO2 */
} UseLedBar;

/**
 * @brief Schedule handle with timing period
 *
 */
class AgSchedule {
public:
  AgSchedule(int period, void (*handler)(void))
      : period(period), handler(handler) {}
  void run(void) {
    uint32_t ms = (uint32_t)(millis() - count);
    if (ms >= period) {
      /** Call handler */
      handler();

      // Serial.printf("[AgSchedule] handle 0x%08x, period: %d(ms)\r\n",
      //               (unsigned int)handler, period);

      /** Update period time */
      count = millis();
    }
  }
  void setPeriod(int period) { this->period = period; }

private:
  void (*handler)(void);
  int period;
  int count;
};

/**
 * @brief AirGradient server configuration and sync data
 *
 */
class AgServer {
public:
  void begin(void) {
    configFailed = false;
    serverFailed = false;
    loadConfig();
  }

  /**
   * @brief Reset local config into default value.
   *
   */
  void defaultReset(void) {
    config.inF = false;
    config.inUSAQI = false;
    memset(config.models, 0, sizeof(config.models));
    memset(config.mqttBrokers, 0, sizeof(config.mqttBrokers));
    config.useRGBLedBar = UseLedBarOff;
    saveConfig();
  }

  /**
   * @brief Get server configuration
   *
   * @param id Device ID
   * @return true Success
   * @return false Failure
   */
  bool fetchServerConfiguration(String id) {
    String uri =
        "http://hw.airgradient.com/sensors/airgradient:" + id + "/one/config";

    /** Init http client */
    HTTPClient client;
    if (client.begin(uri) == false) {
      configFailed = true;
      return false;
    }

    /** Get */
    int retCode = client.GET();
    if (retCode != 200) {
      client.end();
      configFailed = true;
      return false;
    }

    /** clear failed */
    configFailed = false;

    /** Get response string */
    String respContent = client.getString();
    client.end();
    Serial.println("Get server config: " + respContent);

    /** Parse JSON */
    JSONVar root = JSON.parse(respContent);
    if (JSON.typeof(root) == "undefined") {
      /** JSON invalid */
      return false;
    }

    /** Get "country" */
    bool inF = false;
    if (JSON.typeof_(root["country"]) == "string") {
      String _country = root["country"];
      country = _country;

      if (country == "US") {
        inF = true;
      } else {
        inF = false;
      }
    }

    /** Get "pmsStandard" */
    bool inUSAQI = false;
    if (JSON.typeof_(root["pmStandard"]) == "string") {
      String standard = root["pmStandard"];
      if (standard == "ugm3") {
        inUSAQI = false;
      } else {
        inUSAQI = true;
      }
    }

    /** Get "co2CalibrationRequested" */
    if (JSON.typeof_(root["co2CalibrationRequested"]) == "boolean") {
      co2Calib = root["co2CalibrationRequested"];
    } else {
      co2Calib = false;
    }

    /** Get "ledBarMode" */
    uint8_t ledBarMode = UseLedBarOff;
    if (JSON.typeof_(root["ledBarMode"]) == "string") {
      String mode = root["ledBarMode"];
      if (mode == "co2") {
        ledBarMode = UseLedBarCO2;
      } else if (mode == "pm") {
        ledBarMode = UseLedBarPM;
      } else if (mode == "off") {
        ledBarMode = UseLedBarOff;
      } else {
        ledBarMode = UseLedBarOff;
      }
    }

    /** Get model */
    bool _saveConfig = false;
    if (JSON.typeof_(root["model"]) == "string") {
      String model = root["model"];
      if (model.length()) {
        int len = model.length() < sizeof(config.models)
                      ? model.length()
                      : sizeof(config.models);
        if (model != String(config.models)) {
          memset(config.models, 0, sizeof(config.models));
          memcpy(config.models, model.c_str(), len);
          _saveConfig = true;
        }
      }
    }

    /** Get "mqttBrokerUrl" */
    if (JSON.typeof_(root["mqttBrokerUrl"]) == "string") {
      String mqtt = root["mqttBrokerUrl"];
      if (mqtt.length()) {
        int len = mqtt.length() < sizeof(config.mqttBrokers)
                      ? mqtt.length()
                      : sizeof(config.mqttBrokers);
        if (mqtt != String(config.mqttBrokers)) {
          memset(config.mqttBrokers, 0, sizeof(config.mqttBrokers));
          memcpy(config.mqttBrokers, mqtt.c_str(), len);
          _saveConfig = true;
        }
      }
    }

    /** Get 'abcDays' */
    if (JSON.typeof_(root["abcDays"]) == "number") {
      co2AbcCalib = root["abcDays"];
    } else {
      co2AbcCalib = -1;
    }

    /** Get "ledBarTestRequested" */
    if (JSON.typeof_(root["ledBarTestRequested"]) == "boolean") {
      ledBarTestRequested = root["ledBarTestRequested"];
    } else {
      ledBarTestRequested = false;
    }

    /** Show configuration */
    showServerConfig();
    if (_saveConfig || (inF != config.inF) || (inUSAQI != config.inUSAQI) ||
        (ledBarMode != config.useRGBLedBar)) {
      config.inF = inF;
      config.inUSAQI = inUSAQI;
      config.useRGBLedBar = ledBarMode;

      saveConfig();
    }

    return true;
  }

  bool postToServer(String id, String payload) {
    /**
     * @brief Only post data if WiFi is connected
     */
    if (WiFi.isConnected() == false) {
      return false;
    }

    Serial.printf("Post payload: %s\r\n", payload.c_str());

    String uri =
        "http://hw.airgradient.com/sensors/airgradient:" + id + "/measures";

    WiFiClient wifiClient;
    HTTPClient client;
    if (client.begin(wifiClient, uri.c_str()) == false) {
      return false;
    }
    client.addHeader("content-type", "application/json");
    int retCode = client.POST(payload);
    client.end();

    if ((retCode == 200) || (retCode == 429)) {
      serverFailed = false;
      return true;
    } else {
      Serial.printf("Post response failed code: %d\r\n", retCode);
    }
    serverFailed = true;
    return false;
  }

  /**
   * @brief Get temperature configuration unit
   *
   * @return true F unit
   * @return false C Unit
   */
  bool isTemperatureUnitF(void) { return config.inF; }

  /**
   * @brief Get PMS standard unit
   *
   * @return true USAQI
   * @return false ugm3
   */
  bool isPMSinUSAQI(void) { return config.inUSAQI; }

  /**
   * @brief Get status of get server configuration is failed
   *
   * @return true Failed
   * @return false Success
   */
  bool isConfigFailed(void) { return configFailed; }

  /**
   * @brief Get status of post server configuration is failed
   *
   * @return true Failed
   * @return false Success
   */
  bool isServerFailed(void) { return serverFailed; }

  /**
   * @brief Get request calibration CO2
   *
   * @return true Requested. If result = true, it's clear after function call
   * @return false Not-requested
   */
  bool isCo2Calib(void) {
    bool ret = co2Calib;
    if (ret) {
      co2Calib = false;
    }
    return ret;
  }

  /**
   * @brief Get request LedBar test
   *
   * @return true Requested. If result = true, it's clear after function call
   * @return false Not-requested
   */
  bool isLedBarTestRequested(void) {
    bool ret = ledBarTestRequested;
    if (ret) {
      ledBarTestRequested = false;
    }
    return ret;
  }

  /**
   * @brief Get the Co2 auto calib period
   *
   * @return int  days, -1 if invalid.
   */
  int getCo2AbcDaysConfig(void) { return co2AbcCalib; }

  /**
   * @brief Get device configuration model name
   *
   * @return String Model name, empty string if server failed
   */
  String getModelName(void) { return String(config.models); }

  /**
   * @brief Get mqttBroker url
   *
   * @return String Broker url, empty if server failed
   */
  String getMqttBroker(void) { return String(config.mqttBrokers); }

  /**
   * @brief Show server configuration parameter
   */
  void showServerConfig(void) {
    Serial.println("Server configuration: ");
    Serial.printf("inF: %s\r\n", config.inF ? "true" : "false");
    Serial.printf("inUSAQI: %s\r\n", config.inUSAQI ? "true" : "false");
    Serial.printf("useRGBLedBar: %d\r\n", (int)config.useRGBLedBar);
    Serial.printf("Model: %s\r\n", config.models);
    Serial.printf("MQTT Broker: %s\r\n", config.mqttBrokers);
    Serial.printf("S8 calibration period: %d\r\n", co2AbcCalib);
  }

  /**
   * @brief Get server config led bar mode
   *
   * @return UseLedBar
   */
  UseLedBar getLedBarMode(void) { return (UseLedBar)config.useRGBLedBar; }

  /**
   * @brief Get the Country
   *
   * @return String
   */
  String getCountry(void) { return country; }

private:
  bool configFailed;        /** Flag indicate get server configuration failed */
  bool serverFailed;        /** Flag indicate post data to server failed */
  bool co2Calib;            /** Is co2Ppmcalibration requset */
  bool ledBarTestRequested; /** */
  int co2AbcCalib = -1;     /** update auto calibration number of day */
  String country;           /***/

  struct config_s {
    bool inF;
    bool inUSAQI;
    uint8_t useRGBLedBar;
    char models[20];
    char mqttBrokers[256];
    uint32_t checksum;
  };
  struct config_s config;

  void defaultConfig(void) {
    config.inF = false;
    config.inUSAQI = false;
    memset(config.models, 0, sizeof(config.models));
    memset(config.mqttBrokers, 0, sizeof(config.mqttBrokers));

    Serial.println("Load config default");
    saveConfig();
  }

  void loadConfig(void) {
    if (EEPROM.readBytes(0, &config, sizeof(config)) != sizeof(config)) {
      config.inF = false;
      config.inUSAQI = false;
      memset(config.models, 0, sizeof(config.models));
      memset(config.mqttBrokers, 0, sizeof(config.mqttBrokers));

      Serial.println("Load configure failed");
    } else {
      uint32_t sum = 0;
      uint8_t *data = (uint8_t *)&config;
      for (int i = 0; i < sizeof(config) - 4; i++) {
        sum += data[i];
      }
      if (sum != config.checksum) {
        Serial.println("config checksum failed");
        defaultConfig();
      }
    }

    showServerConfig();
  }

  void saveConfig(void) {
    config.checksum = 0;
    uint8_t *data = (uint8_t *)&config;
    for (int i = 0; i < sizeof(config) - 4; i++) {
      config.checksum += data[i];
    }

    EEPROM.writeBytes(0, &config, sizeof(config));
    EEPROM.commit();
    Serial.println("Save config");
  }
};
AgServer agServer;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data);

class AgMqtt {
private:
  bool _isBegin = false;
  String uri;
  String hostname;
  String user;
  String pass;
  int port;
  esp_mqtt_client_handle_t client;
  bool clientConnected = false;
  int connectFailedCount = 0;

public:
  AgMqtt() {}
  ~AgMqtt() {}

  /**
   * @brief Initialize mqtt
   *
   * @param uri Complete mqtt uri, ex:
   * mqtts://username:password@my.broker.com:4711
   * @return true Success
   * @return false Failure
   */
  bool begin(String uri) {
    if (_isBegin) {
      Serial.println("Mqtt already begin, call 'end' and try again");
      return true;
    }

    if (uri.isEmpty()) {
      Serial.println("Mqtt uri is empty");
      return false;
    }

    this->uri = uri;
    Serial.printf("mqtt init '%s'\r\n", uri.c_str());

    /** config esp_mqtt client */
    esp_mqtt_client_config_t config = {
        .uri = this->uri.c_str(),
    };

    /** init client */
    client = esp_mqtt_client_init(&config);
    if (client == NULL) {
      Serial.println("MQTT client init failed");
      return false;
    }

    /** Register event */
    if (esp_mqtt_client_register_event(client, MQTT_EVENT_ANY,
                                       mqtt_event_handler, NULL) != ESP_OK) {
      Serial.println("MQTT client register event failed");
      return false;
    }

    if (esp_mqtt_client_start(client) != ESP_OK) {
      Serial.println("MQTT client start failed");
      return false;
    }

    _isBegin = true;
    return true;
  }

  /**
   * @brief Deinitialize mqtt
   *
   */
  void end(void) {
    if (_isBegin == false) {
      return;
    }

    esp_mqtt_client_disconnect(client);
    esp_mqtt_client_stop(client);
    esp_mqtt_client_destroy(client);
    _isBegin = false;

    Serial.println("mqtt de-init");
  }

  bool publish(const char *topic, const char *payload, int len) {
    if ((_isBegin == false) || (clientConnected == false)) {
      return false;
    }

    if (esp_mqtt_client_publish(client, topic, payload, len, 0, 0) == ESP_OK) {
      return true;
    }
    return false;
  }

  /**
   * @brief Get current complete mqtt uri
   *
   * @return String
   */
  String getUri(void) { return uri; }

  void _connectionHandler(bool connected) {
    clientConnected = connected;
    if (clientConnected == false) {
      connectFailedCount++;
    } else {
      connectFailedCount = 0;
    }
  }

  /**
   * @brief Mqtt client connect status
   *
   * @return true Connected
   * @return false Disconnected or Not initialize
   */
  bool isConnected(void) { return (_isBegin && clientConnected); }

  /**
   * @brief Get number of times connection failed
   *
   * @return int
   */
  int connectionFailedCount(void) { return connectFailedCount; }
};
AgMqtt agMqtt;
static TaskHandle_t mqttTask = NULL;

/** Create airgradient instance for 'OPEN_AIR_OUTDOOR' board */
AirGradient ag(OPEN_AIR_OUTDOOR);

static int ledSmState = APP_SM_NORMAL;

int loopCount = 0;

WiFiManager wifiManager; /** wifi manager instance */
static bool wifiHasConfig = false;
static String wifiSSID = "";

/** Web server instance */
WebServer webServer;

int tvocIndex = -1;
int tvocRawIndex = -1;
int noxIndex = -1;
int noxRawIndex = -1;
int co2Ppm = -1;

int pm25_1 = -1;
int pm01_1 = -1;
int pm10_1 = -1;
int pm03PCount_1 = -1;
float temp_1 = -1001;
int hum_1 = -1;

int pm25_2 = -1;
int pm01_2 = -1;
int pm10_2 = -1;
int pm03PCount_2 = -1;
float temp_2 = -1001;
int hum_2 = -1;

int pm1Value01;
int pm1Value25;
int pm1Value10;
int pm1PCount;
int pm1temp;
int pm1hum;
int pm2Value01;
int pm2Value25;
int pm2Value10;
int pm2PCount;
int pm2temp;
int pm2hum;
int countPosition;
const int targetCount = 20;

enum {
  FW_MODE_PST, /** PMS5003T, S8 and SGP41 */
  FW_MODE_PPT, /** PMS5003T_1, PMS5003T_2, SGP41 */
  FW_MODE_PP,  /** PMS5003T_1, PMS5003T_2 */
  FW_MDOE_PS   /** PMS5003T, S8 */
};
int fw_mode = FW_MODE_PST;

void boardInit(void);
void failedHandler(String msg);
void co2Calibration(void);
static String getDevId(void);
static void updateWiFiConnect(void);
static void tvocUpdate(void);
static void pmUpdate(void);
static void sendDataToServer(void);
static void co2Update(void);
static void updateServerConfiguration(void);
static const char *getFwMode(int mode);
static void showNr(void);
static void webServerInit(void);
static String getServerSyncData(bool localServer);
static void createMqttTask(void);
static void factoryConfigReset(void);

bool hasSensorS8 = true;
bool hasSensorPMS1 = true;
bool hasSensorPMS2 = true;
bool hasSensorSGP = true;
uint32_t factoryBtnPressTime = 0;
String mdnsModelName = "";
int getCO2FailCount = 0;
AgSchedule configSchedule(SERVER_CONFIG_UPDATE_INTERVAL,
                          updateServerConfiguration);
AgSchedule serverSchedule(SERVER_SYNC_INTERVAL, sendDataToServer);
AgSchedule co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Update);
AgSchedule pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, pmUpdate);
AgSchedule tvocSchedule(SENSOR_TVOC_UPDATE_INTERVAL, tvocUpdate);

void setup() {
  EEPROM.begin(512);

  Serial.begin(115200);
  delay(100); /** For bester show log */
  showNr();

  /** Board init */
  boardInit();

  /** Server init */
  agServer.begin();

  /** WiFi connect */
  connectToWifi();

  if (WiFi.isConnected()) {
    webServerInit();

    /** MQTT init */
    if (agServer.getMqttBroker().isEmpty() == false) {
      if (agMqtt.begin(agServer.getMqttBroker())) {
        createMqttTask();
        Serial.println("MQTT client init success");
      } else {
        Serial.println("MQTT client init failure");
      }
    }

    wifiHasConfig = true;
    sendPing();

    agServer.fetchServerConfiguration(getDevId());
    if (agServer.isConfigFailed()) {
      ledSmHandler(APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED);
      delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
    }
  }

  ledSmHandler(APP_SM_NORMAL);
}

void loop() {
  configSchedule.run();
  serverSchedule.run();

  if (fw_mode == FW_MODE_PST) {
    if (hasSensorS8) {
      co2Schedule.run();
    }
  }

  if (hasSensorPMS1 || hasSensorPMS2) {
    pmsSchedule.run();
  }

  if (fw_mode == FW_MODE_PST || fw_mode == FW_MODE_PPT) {
    if (hasSensorSGP) {
      tvocSchedule.run();
    }
  }
  updateWiFiConnect();

  factoryConfigReset();
}

void sendPing() {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  root["boot"] = loopCount;
  if (agServer.postToServer(getDevId(), JSON.stringify(root))) {
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNECTED);
  } else {
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNECT_FAILED);
  }
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
}

static void sendDataToServer(void) {
  String syncData = getServerSyncData(false);
  if (agServer.postToServer(getDevId(), syncData)) {
    resetWatchdog();
  }

  loopCount++;
}

void resetWatchdog() {
  Serial.println("Watchdog reset");
  ag.watchdog.reset();
}

bool wifiMangerClientConnected(void) {
  return WiFi.softAPgetStationNum() ? true : false;
}

// Wifi Manager
void connectToWifi() {
  wifiSSID = "airgradient-" + String(getNormalizedMac());

  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setTimeout(WIFI_CONNECT_COUNTDOWN_MAX);

  wifiManager.setAPCallback([](WiFiManager *obj) {
    /** This callback if wifi connnected failed and try to start configuration
     * portal */
    ledSmState = APP_SM_WIFI_MANAGER_MODE;
  });
  wifiManager.setSaveConfigCallback([]() {
    /** Wifi connected save the configuration */
    ledSmHandler(APP_SM_WIFI_MANAGER_STA_CONNECTED);
  });
  wifiManager.setSaveParamsCallback([]() {
    /** Wifi set connect: ssid, password */
    ledSmHandler(APP_SM_WIFI_MANAGER_STA_CONNECTING);
  });
  wifiManager.autoConnect(wifiSSID.c_str(), WIFI_HOTSPOT_PASSWORD_DEFAULT);

  xTaskCreate(
      [](void *obj) {
        while (wifiManager.getConfigPortalActive()) {
          wifiManager.process();
        }
        vTaskDelete(NULL);
      },
      "wifi_cfg", 4096, NULL, 10, NULL);

  uint32_t stimer = millis();
  bool clientConnectChanged = false;
  while (wifiManager.getConfigPortalActive()) {
    if (WiFi.isConnected() == false) {
      if (ledSmState == APP_SM_WIFI_MANAGER_MODE) {
        uint32_t ms = (uint32_t)(millis() - stimer);
        if (ms >= 100) {
          stimer = millis();
          ledSmHandler(ledSmState);
        }
      }

      /** Check for client connect to change led color */
      bool clientConnected = wifiMangerClientConnected();
      if (clientConnected != clientConnectChanged) {
        clientConnectChanged = clientConnected;
        if (clientConnectChanged) {
          ledSmHandler(APP_SM_WIFI_MANAGER_PORTAL_ACTIVE);
        } else {
          ledSmHandler(APP_SM_WIFI_MANAGER_MODE);
        }
      }
    }
  }

  /** Show display wifi connect result failed */
  ag.statusLed.setOff();
  delay(2000);
  if (WiFi.isConnected() == false) {
    ledSmHandler(APP_SM_WIFI_MANAGER_CONNECT_FAILED);
  }
}

String getNormalizedMac() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}

void boardInit(void) {
  if (Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin()) == false) {
    failedHandler("Init I2C failed");
  }

  Serial.println("Firmware Version: " + ag.getVersion());

  ag.watchdog.begin();
  ag.button.begin();
  ag.statusLed.begin();

  /** detect sensor: PMS5003, PMS5003T, SGP41 and S8 */
  /**
   * Serial1 and Serial0 is use for connect S8 and PM sensor or both PM
   */
  bool serial1Available = true;
  bool serial0Available = true;

  if (ag.s8.begin(Serial1) == false) {
    Serial1.end();
    delay(200);
    Serial.println("Can not detect S8 on Serial1, try on Serial0");
    /** Check on other port */
    if (ag.s8.begin(Serial0) == false) {
      hasSensorS8 = false;

      Serial.println("CO2 S8 sensor not found");
      Serial.println("Can not detect S8 run mode 'PPT'");
      fw_mode = FW_MODE_PPT;

      Serial0.end();
      delay(200);
    } else {
      Serial.println("Found S8 on Serial0");
      serial0Available = false;
    }
  } else {
    Serial.println("Found S8 on Serial1");
    serial1Available = false;
  }

  if (ag.sgp41.begin(Wire) == false) {
    hasSensorSGP = false;
    Serial.println("SGP sensor not found");

    if (hasSensorS8 == false) {
      fw_mode = FW_MODE_PP;
      Serial.println("Can not detect SGP run mode 'O-1PP'");
    } else {
      Serial.println("Can not detect SGP run mode 'O-1PS'");
      fw_mode = FW_MDOE_PS;
    }
  }

  /** Try to find the PMS on other difference port with S8 */
  if (fw_mode == FW_MODE_PST) {
    bool pmInitSuccess = false;
    if (serial0Available) {
      if (ag.pms5003t_1.begin(Serial0) == false) {
        hasSensorPMS1 = false;
        Serial.println("PMS1 sensor not found");
      } else {
        serial0Available = false;
        pmInitSuccess = true;
        Serial.println("Found PMS 1 on Serial0");
      }
    }
    if (pmInitSuccess == false) {
      if (serial1Available) {
        if (ag.pms5003t_1.begin(Serial1) == false) {
          hasSensorPMS1 = false;
          Serial.println("PMS1 sensor not found");
        } else {
          serial1Available = false;
          Serial.println("Found PMS 1 on Serial1");
        }
      }
    }
    hasSensorPMS2 = false; // Disable PM2
  } else {
    if (ag.pms5003t_1.begin(Serial0) == false) {
      hasSensorPMS1 = false;
      Serial.println("PMS1 sensor not found");
    } else {
      Serial.println("Found PMS 1 on Serial0");
    }
    if (ag.pms5003t_2.begin(Serial1) == false) {
      hasSensorPMS2 = false;
      Serial.println("PMS2 sensor not found");
    } else {
      Serial.println("Found PMS 2 on Serial1");
    }
  }

  /** update the PMS poll period base on fw mode and sensor available */
  if (fw_mode != FW_MODE_PST) {
    if (hasSensorPMS1 && hasSensorPMS2) {
      pmsSchedule.setPeriod(2000);
    }
  }

  Serial.printf("Firmware Mode: %s\r\n", getFwMode(fw_mode));
  switch (fw_mode) {
  case FW_MODE_PP:
    mdnsModelName = "O-1PP";
    break;
  case FW_MODE_PPT:
    mdnsModelName = "O-1PPT";
    break;
  case FW_MODE_PST:
    mdnsModelName = "O-1PST";
    break;
  case FW_MDOE_PS:
    mdnsModelName = "0-1PS";
  default:
    break;
  }
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    vTaskDelay(1000);
  }
}

void co2Calibration(void) {
  /** Count down for co2CalibCountdown secs */
  for (int i = 0; i < SENSOR_CO2_CALIB_COUNTDOWN_MAX; i++) {
    Serial.printf("Start CO2 calibration after %d sec\r\n",
                  SENSOR_CO2_CALIB_COUNTDOWN_MAX - i);
    delay(1000);
  }

  if (ag.s8.setBaselineCalibration()) {
    Serial.println("Calibration success");
    delay(1000);
    Serial.println("Wait for calibration to finish...");
    int count = 0;
    while (ag.s8.isBaseLineCalibrationDone() == false) {
      delay(1000);
      count++;
    }
    Serial.printf("Calibration finished after %d sec\r\n", count);
    delay(2000);
  } else {
    Serial.println("Calibration failure");
    delay(2000);
  }
}

/**
 * @brief WiFi reconnect handler
 */
static void updateWiFiConnect(void) {
  static uint32_t lastRetry;
  if (wifiHasConfig == false) {
    return;
  }
  if (WiFi.isConnected()) {
    lastRetry = millis();
    return;
  }
  uint32_t ms = (uint32_t)(millis() - lastRetry);
  if (ms >= WIFI_CONNECT_RETRY_MS) {
    lastRetry = millis();
    WiFi.reconnect();

    Serial.printf("Re-Connect to WiFi\r\n");
  }
}

/**
 * @brief Update tvocIndexindex
 *
 */
static void tvocUpdate(void) {
  tvocIndex = ag.sgp41.getTvocIndex();
  tvocRawIndex = ag.sgp41.getTvocRaw();
  noxIndex = ag.sgp41.getNoxIndex();
  noxRawIndex = ag.sgp41.getNoxRaw();

  Serial.println();
  Serial.printf("TVOC index: %d\r\n", tvocIndex);
  Serial.printf("TVOC raw: %d\r\n", tvocRawIndex);
  Serial.printf("NOx index: %d\r\n", noxIndex);
  Serial.printf("NOx raw: %d\r\n", noxRawIndex);
}

/**
 * @brief Update PMS data
 *
 */
static void pmUpdate(void) {
  bool pmsResult_1 = false;
  bool pmsResult_2 = false;
  if (hasSensorPMS1 && ag.pms5003t_1.readData()) {
    pm01_1 = ag.pms5003t_1.getPm01Ae();
    pm25_1 = ag.pms5003t_1.getPm25Ae();
    pm10_1 = ag.pms5003t_1.getPm10Ae();
    pm03PCount_1 = ag.pms5003t_1.getPm03ParticleCount();
    temp_1 = ag.pms5003t_1.getTemperature();
    hum_1 = ag.pms5003t_1.getRelativeHumidity();

    pmsResult_1 = true;

    Serial.println();
    Serial.printf("[1] PM1 ug/m3: %d\r\n", pm01_1);
    Serial.printf("[1] PM2.5 ug/m3: %d\r\n", pm25_1);
    Serial.printf("[1] PM10 ug/m3: %d\r\n", pm10_1);
    Serial.printf("[1] PM3.0 Count: %d\r\n", pm03PCount_1);
    Serial.printf("[1] Temperature in C: %0.2f\r\n", temp_1);
    Serial.printf("[1] Relative Humidity: %d\r\n", hum_1);
  } else {
    pm01_1 = -1;
    pm25_1 = -1;
    pm10_1 = -1;
    pm03PCount_1 = -1;
    temp_1 = -1001;
    hum_1 = -1;
  }

  if (hasSensorPMS2 && ag.pms5003t_2.readData()) {
    pm01_2 = ag.pms5003t_2.getPm01Ae();
    pm25_2 = ag.pms5003t_2.getPm25Ae();
    pm10_2 = ag.pms5003t_2.getPm10Ae();
    pm03PCount_2 = ag.pms5003t_2.getPm03ParticleCount();
    temp_2 = ag.pms5003t_2.getTemperature();
    hum_2 = ag.pms5003t_2.getRelativeHumidity();

    pmsResult_2 = true;

    Serial.println();
    Serial.printf("[2] PM1 ug/m3: %d\r\n", pm01_2);
    Serial.printf("[2] PM2.5 ug/m3: %d\r\n", pm25_2);
    Serial.printf("[2] PM10 ug/m3: %d\r\n", pm10_2);
    Serial.printf("[2] PM3.0 Count: %d\r\n", pm03PCount_2);
    Serial.printf("[2] Temperature in C: %0.2f\r\n", temp_2);
    Serial.printf("[2] Relative Humidity: %d\r\n", hum_2);
  } else {
    pm01_2 = -1;
    pm25_2 = -1;
    pm10_2 = -1;
    pm03PCount_2 = -1;
    temp_2 = -1001;
    hum_2 = -1;
  }

  if (hasSensorPMS1 && hasSensorPMS2 && pmsResult_1 && pmsResult_2) {
    /** Get total of PMS1*/
    pm1Value01 = pm1Value01 + pm01_1;
    pm1Value25 = pm1Value25 + pm25_1;
    pm1Value10 = pm1Value10 + pm10_1;
    pm1PCount = pm1PCount + pm03PCount_1;
    pm1temp = pm1temp + temp_1;
    pm1hum = pm1hum + hum_1;

    /** Get total of PMS2 */
    pm2Value01 = pm2Value01 + pm01_2;
    pm2Value25 = pm2Value25 + pm25_2;
    pm2Value10 = pm2Value10 + pm10_2;
    pm2PCount = pm2PCount + pm03PCount_2;
    pm2temp = pm2temp + temp_2;
    pm2hum = pm2hum + hum_2;

    countPosition++;

    /** Get average */
    if (countPosition == targetCount) {
      pm01_1 = pm1Value01 / targetCount;
      pm25_1 = pm1Value25 / targetCount;
      pm10_1 = pm1Value10 / targetCount;
      pm03PCount_1 = pm1PCount / targetCount;
      temp_1 = pm1temp / targetCount;
      hum_1 = pm1hum / targetCount;

      pm01_2 = pm2Value01 / targetCount;
      pm25_2 = pm2Value25 / targetCount;
      pm10_2 = pm2Value10 / targetCount;
      pm03PCount_2 = pm2PCount / targetCount;
      temp_2 = pm2temp / targetCount;
      hum_2 = pm2hum / targetCount;

      countPosition = 0;

      pm1Value01 = 0;
      pm1Value25 = 0;
      pm1Value10 = 0;
      pm1PCount = 0;
      pm1temp = 0;
      pm1hum = 0;
      pm2Value01 = 0;
      pm2Value25 = 0;
      pm2Value10 = 0;
      pm2PCount = 0;
      pm2temp = 0;
      pm2hum = 0;
    }
  }

  if (hasSensorSGP) {
    float temp;
    float hum;
    if (pmsResult_1 && pmsResult_2) {
      temp = (temp_1 + temp_2) / 2.0f;
      hum = (hum_1 + hum_2) / 2.0f;
    } else {
      if (pmsResult_1) {
        temp = temp_1;
        hum = hum_1;
      }
      if (pmsResult_2) {
        temp = temp_2;
        hum = hum_2;
      }
    }
    ag.sgp41.setCompensationTemperatureHumidity(temp, hum);
  }
}

static void co2Update(void) {
  int value = ag.s8.getCo2();
  if (value >= 0) {
    co2Ppm = value;
    getCO2FailCount = 0;
    Serial.printf("CO2 ppm: %d\r\n", co2Ppm);
  } else {
    getCO2FailCount++;
    Serial.printf("Get CO2 failed: %d\r\n", getCO2FailCount);
    if (getCO2FailCount >= 3) {
      co2Ppm = -1;
    }
  }
}

static void updateServerConfiguration(void) {
  if (agServer.fetchServerConfiguration(getDevId())) {
    /** Only support CO2 S8 sensor on FW_MODE_PST */
    if (fw_mode == FW_MODE_PST) {
      if (agServer.isCo2Calib()) {
        if (hasSensorS8) {
          co2Calibration();
        } else {
          Serial.println("CO2 S8 not available, calibration ignored");
        }
      }

      if (agServer.getCo2AbcDaysConfig() > 0) {
        if (hasSensorS8) {
          int newHour = agServer.getCo2AbcDaysConfig() * 24;
          Serial.printf("Requested abcDays setting: %d days (%d hours)\r\n",
                        agServer.getCo2AbcDaysConfig(), newHour);
          int curHour = ag.s8.getAbcPeriod();
          Serial.printf("Current S8 abcDays setting: %d (hours)\r\n", curHour);
          if (curHour == newHour) {
            Serial.println("'abcDays' unchanged");
          } else {
            if (ag.s8.setAbcPeriod(agServer.getCo2AbcDaysConfig() * 24) ==
                false) {
              Serial.println("Set S8 abcDays period failed");
            } else {
              Serial.println("Set S8 abcDays period success");
            }
          }
        }
      } else {
        Serial.println("CO2 S8 not available, set 'abcDays' ignored");
      }
    }

    String mqttUri = agServer.getMqttBroker();
    if (mqttUri != agMqtt.getUri()) {
      agMqtt.end();
      if (mqttTask != NULL) {
        vTaskDelete(mqttTask);
        mqttTask = NULL;
      }
      if (agMqtt.begin(mqttUri)) {
        Serial.println("Connect to MQTT broker successful");
        createMqttTask();
      } else {
        Serial.println("Connect to MQTT broker failed");
      }
    }
  }
}

static String getDevId(void) { return getNormalizedMac(); }

void ledBlinkDelay(uint32_t tdelay) {
  ag.statusLed.setOn();
  delay(tdelay);
  ag.statusLed.setOff();
  delay(tdelay);
}

void ledSmHandler(int sm) {
  if (sm > APP_SM_NORMAL) {
    return;
  }

  ledSmState = sm;
  switch (sm) {
  case APP_SM_WIFI_MANAGER_MODE: {
    ag.statusLed.setToggle();
    break;
  }
  case APP_SM_WIFI_MANAGER_PORTAL_ACTIVE: {
    ag.statusLed.setOn();
    break;
  }
  case APP_SM_WIFI_MANAGER_STA_CONNECTING: {
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_WIFI_MANAGER_STA_CONNECTED: {
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECTING: {
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECTED: {
    ag.statusLed.setOff();

    /** two time slow blink, then off */
    for (int i = 0; i < 2; i++) {
      ledBlinkDelay(LED_SLOW_BLINK_DELAY);
    }

    ag.statusLed.setOff();
    break;
  }
  case APP_SM_WIFI_MANAGER_CONNECT_FAILED: {
    /** Three time fast blink then 2 sec pause. Repeat 3 times */
    ag.statusLed.setOff();

    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 3; i++) {
        ledBlinkDelay(LED_FAST_BLINK_DELAY);
      }
      delay(2000);
    }
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECT_FAILED: {
    ag.statusLed.setOff();
    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 4; i++) {
        ledBlinkDelay(LED_FAST_BLINK_DELAY);
      }
      delay(2000);
    }
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED: {
    ag.statusLed.setOff();
    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 5; i++) {
        ledBlinkDelay(LED_FAST_BLINK_DELAY);
      }
      delay(2000);
    }
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_WIFI_LOST: {
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_SERVER_LOST: {
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_SENSOR_CONFIG_FAILED: {
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_NORMAL: {
    ag.statusLed.setOff();
    break;
  }
  default:
    break;
  }
}

static const char *getFwMode(int mode) {
  switch (mode) {
  case FW_MODE_PST:
    return "FW_MODE_PST";
  case FW_MODE_PPT:
    return "FW_MODE_PPT";
  case FW_MODE_PP:
    return "FW_MODE_PP";
  case FW_MDOE_PS:
    return "FW_MODE_PS";
  default:
    break;
  }
  return "FW_MODE_UNKNOW";
}

static void showNr(void) { Serial.println("Serial nr: " + getDevId()); }

void webServerMeasureCurrentGet(void) {
  webServer.send(200, "application/json", getServerSyncData(true));
}

/**
 * Sends metrics in Prometheus/OpenMetrics format to the currently connected
 * webServer client.
 *
 * For background, see:
 * https://prometheus.io/docs/instrumenting/exposition_formats/
 */
void webServerMetricsGet(void) {
  String response;
  String current_metric_name;
  const auto add_metric = [&](const String &name, const String &help,
                              const String &type, const String &unit = "") {
    current_metric_name = "airgradient_" + name;
    if (!unit.isEmpty())
      current_metric_name += "_" + unit;
    response += "# HELP " + current_metric_name + " " + help + "\n";
    response += "# TYPE " + current_metric_name + " " + type + "\n";
    if (!unit.isEmpty())
      response += "# UNIT " + current_metric_name + " " + unit + "\n";
  };
  const auto add_metric_point = [&](const String &labels, const String &value) {
    response += current_metric_name + "{" + labels + "} " + value + "\n";
  };

  add_metric("info", "AirGradient device information", "info");
  add_metric_point("airgradient_serial_number=\"" + getDevId() +
                       "\",airgradient_device_type=\"" + ag.getBoardName() +
                       "\",airgradient_library_version=\"" + ag.getVersion() +
                       "\"",
                   "1");

  add_metric("config_ok",
             "1 if the AirGradient device was able to successfully fetch its "
             "configuration from the server",
             "gauge");
  add_metric_point("", agServer.isConfigFailed() ? "0" : "1");

  add_metric(
      "post_ok",
      "1 if the AirGradient device was able to successfully send to the server",
      "gauge");
  add_metric_point("", agServer.isServerFailed() ? "0" : "1");

  add_metric(
      "wifi_rssi",
      "WiFi signal strength from the AirGradient device perspective, in dBm",
      "gauge", "dbm");
  add_metric_point("", String(WiFi.RSSI()));

  if (hasSensorS8 && co2Ppm >= 0) {
    add_metric("co2",
               "Carbon dioxide concentration as measured by the AirGradient S8 "
               "sensor, in parts per million",
               "gauge", "ppm");
    add_metric_point("", String(co2Ppm));
  }

  float temp = -1001;
  float hum = -1;
  int pm01 = -1;
  int pm25 = -1;
  int pm10 = -1;
  int pm03PCount = -1;
  if (hasSensorPMS1 && hasSensorPMS2) {
    temp = (temp_1 + temp_2) / 2.0f;
    hum = (hum_1 + hum_2) / 2.0f;
    pm01 = (pm01_1 + pm01_2) / 2;
    pm25 = (pm25_1 + pm25_2) / 2;
    pm10 = (pm10_1 + pm10_2) / 2;
    pm03PCount = (pm03PCount_1 + pm03PCount_2) / 2;
  } else {
    if (hasSensorPMS1) {
      temp = temp_1;
      hum = hum_1;
      pm01 = pm01_1;
      pm25 = pm25_1;
      pm10 = pm10_1;
      pm03PCount = pm03PCount_1;
    }
    if (hasSensorPMS2) {
      temp = temp_2;
      hum = hum_2;
      pm01 = pm01_2;
      pm25 = pm25_2;
      pm10 = pm10_2;
      pm03PCount = pm03PCount_2;
    }
  }

  if (hasSensorPMS1 || hasSensorPMS2) {
    if (pm01 >= 0) {
      add_metric("pm1",
                 "PM1.0 concentration as measured by the AirGradient PMS "
                 "sensor, in micrograms per cubic meter",
                 "gauge", "ugm3");
      add_metric_point("", String(pm01));
    }
    if (pm25 >= 0) {
      add_metric("pm2d5",
                 "PM2.5 concentration as measured by the AirGradient PMS "
                 "sensor, in micrograms per cubic meter",
                 "gauge", "ugm3");
      add_metric_point("", String(pm25));
    }
    if (pm10 >= 0) {
      add_metric("pm10",
                 "PM10 concentration as measured by the AirGradient PMS "
                 "sensor, in micrograms per cubic meter",
                 "gauge", "ugm3");
      add_metric_point("", String(pm10));
    }
    if (pm03PCount >= 0) {
      add_metric("pm0d3",
                 "PM0.3 concentration as measured by the AirGradient PMS "
                 "sensor, in number of particules per 100 milliliters",
                 "gauge", "p100ml");
      add_metric_point("", String(pm03PCount));
    }
  }

  if (hasSensorSGP) {
    if (tvocIndex >= 0) {
      add_metric("tvoc_index",
                 "The processed Total Volatile Organic Compounds (TVOC) index "
                 "as measured by the AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(tvocIndex));
    }
    if (tvocRawIndex >= 0) {
      add_metric("tvoc_raw",
                 "The raw input value to the Total Volatile Organic Compounds "
                 "(TVOC) index as measured by the AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(tvocRawIndex));
    }
    if (noxIndex >= 0) {
      add_metric("nox_index",
                 "The processed Nitrous Oxide (NOx) index as measured by the "
                 "AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(noxIndex));
    }
    if (noxRawIndex >= 0) {
      add_metric("nox_raw",
                 "The raw input value to the Nitrous Oxide (NOx) index as "
                 "measured by the AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(noxRawIndex));
    }
  }

  if (hasSensorPMS1 || hasSensorPMS2) {
    if (temp > -1001) {
      add_metric("temperature",
                 "The ambient temperature as measured by the AirGradient SHT "
                 "sensor, in degrees Celsius",
                 "gauge", "celcius");
      add_metric_point("", String(temp));
    }
    if (hum >= 0) {
      add_metric(
          "humidity",
          "The relative humidity as measured by the AirGradient SHT sensor",
          "gauge", "percent");
      add_metric_point("", String(hum));
    }
  }

  response += "# EOF\n";
  webServer.send(200,
                 "application/openmetrics-text; version=1.0.0; charset=utf-8",
                 response);
}

void webServerHandler(void *param) {
  for (;;) {
    webServer.handleClient();
  }
}

static void webServerInit(void) {
  String host = "airgradient_" + getDevId();
  if (!MDNS.begin(host)) {
    Serial.println("Init MDNS failed");
    return;
  }

  webServer.on("/measures/current", HTTP_GET, webServerMeasureCurrentGet);
  // Make it possible to query this device from Prometheus/OpenMetrics.
  webServer.on("/metrics", HTTP_GET, webServerMetricsGet);
  webServer.begin();
  MDNS.addService("_airgradient", "tcp", 80);
  MDNS.addServiceTxt("airgradient", "_tcp", "model", mdnsModelName);
  MDNS.addServiceTxt("airgradient", "_tcp", "serialno", getDevId());
  MDNS.addServiceTxt("airgradient", "_tcp", "fw_ver", ag.getVersion());
  MDNS.addServiceTxt("airgradient", "_tcp", "vendor", "AirGradient");

  if (xTaskCreate(webServerHandler, "webserver", 1024 * 4, NULL, 5, NULL) !=
      pdTRUE) {
    Serial.println("Create task handle webserver failed");
  }
  Serial.printf("Webserver init: %s.local\r\n", host.c_str());
}

static String getServerSyncData(bool localServer) {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  root["boot"] = loopCount;
  if (localServer) {
    root["serialno"] = getDevId();
  }

  if (fw_mode == FW_MODE_PST) {
    if (hasSensorS8) {
      if (co2Ppm >= 0) {
        root["rco2"] = co2Ppm;
      }
    }

    if (hasSensorPMS1) {
      if (pm01_1 >= 0) {
        root["pm01"] = pm01_1;
      }
      if (pm25_1 >= 0) {
        root["pm02"] = pm25_1;
      }
      if (pm10_1 >= 0) {
        root["pm10"] = pm10_1;
      }
      if (pm03PCount_1 >= 0) {
        root["pm003_count"] = pm03PCount_1;
      }
      if (temp_1 > -1001) {
        root["atmp"] = ag.round2(temp_1);
      }
      if (hum_1 >= 0) {
        root["rhum"] = hum_1;
      }
    } else if (hasSensorPMS2) {
      if (pm01_2 >= 0) {
        root["pm01"] = pm01_2;
      }
      if (pm25_2 >= 0) {
        root["pm02"] = pm25_2;
      }
      if (pm10_2 >= 0) {
        root["pm10"] = pm10_2;
      }
      if (pm03PCount_2 >= 0) {
        root["pm003_count"] = pm03PCount_2;
      }
      if (temp_2 > -1001) {
        root["atmp"] = ag.round2(temp_2);
      }
      if (hum_2 >= 0) {
        root["rhum"] = hum_2;
      }
    }
  }

  if ((fw_mode == FW_MODE_PPT) || (fw_mode == FW_MODE_PST)) {
    if (hasSensorSGP) {
      if (tvocIndex >= 0) {
        root["tvoc_index"] = tvocIndex;
      }
      if (tvocRawIndex >= 0) {
        root["tvoc_raw"] = tvocRawIndex;
      }
      if (noxIndex >= 0) {
        root["nox_index"] = noxIndex;
      }
      if (noxRawIndex >= 0) {
        root["nox_raw"] = noxRawIndex;
      }
    }
  }

  if ((fw_mode == FW_MODE_PP) || (fw_mode == FW_MODE_PPT)) {
    if (hasSensorPMS1 && hasSensorPMS2) {
      root["pm01"] = ag.round2((pm01_1 + pm01_2) / 2.0);
      root["pm02"] = ag.round2((pm25_1 + pm25_2) / 2.0);
      root["pm10"] = ag.round2((pm10_1 + pm10_2) / 2.0);
      root["pm003_count"] = ag.round2((pm03PCount_1 + pm03PCount_2) / 2.0);
      root["atmp"] = ag.round2((temp_1 + temp_2) / 2.0);
      root["rhum"] = ag.round2((hum_1 + hum_2) / 2.0);
    }
    if (hasSensorPMS1) {
      root["channels"]["1"]["pm01"] = pm01_1;
      root["channels"]["1"]["pm02"] = pm25_1;
      root["channels"]["1"]["pm10"] = pm10_1;
      root["channels"]["1"]["pm003_count"] = pm03PCount_1;
      root["channels"]["1"]["atmp"] = ag.round2(temp_1);
      root["channels"]["1"]["rhum"] = hum_1;
    }
    if (hasSensorPMS2) {
      root["channels"]["2"]["pm01"] = pm01_2;
      root["channels"]["2"]["pm02"] = pm25_2;
      root["channels"]["2"]["pm10"] = pm10_2;
      root["channels"]["2"]["pm003_count"] = pm03PCount_2;
      root["channels"]["2"]["atmp"] = ag.round2(temp_2);
      root["channels"]["2"]["rhum"] = hum_2;
    }
  }

  return JSON.stringify(root);
}

static void createMqttTask(void) {
  if (mqttTask) {
    vTaskDelete(mqttTask);
    mqttTask = NULL;
  }

  xTaskCreate(
      [](void *param) {
        for (;;) {
          delay(MQTT_SYNC_INTERVAL);

          /** Send data */
          if (agMqtt.isConnected()) {
            String syncData = getServerSyncData(false);
            String topic = "airgradient/readings/" + getDevId();
            if (agMqtt.publish(topic.c_str(), syncData.c_str(),
                               syncData.length())) {
              Serial.println("Mqtt sync success");
            } else {
              Serial.println("Mqtt sync failure");
            }
          }
        }
      },
      "mqtt-task", 1024 * 3, NULL, 6, &mqttTask);

  if (mqttTask == NULL) {
    Serial.println("Creat mqttTask failed");
  }
}

static void factoryConfigReset(void) {
  if (ag.button.getState() == ag.button.BUTTON_PRESSED) {
    if (factoryBtnPressTime == 0) {
      factoryBtnPressTime = millis();
    } else {
      uint32_t ms = (uint32_t)(millis() - factoryBtnPressTime);
      if (ms >= 2000) {
        Serial.println("Factory reset keep presssed for 8 sec");

        uint32_t ledTime = millis();
        bool ledOn = true;
        ag.statusLed.setOn();
        while (ag.button.getState() == ag.button.BUTTON_PRESSED) {
          ms = (uint32_t)(millis() - ledTime);
          if (ms >= LED_SHORT_BLINK_DELAY) {
            ledTime = millis();
            ag.statusLed.setToggle();
          }

          ms = (uint32_t)(millis() - factoryBtnPressTime);
          if (ms > 10000) {
            ag.statusLed.setOff();

            /** Stop MQTT task first */
            if (mqttTask) {
              vTaskDelete(mqttTask);
              mqttTask = NULL;
            }

            /** Disconnect WIFI */
            WiFi.disconnect();
            wifiManager.resetSettings();

            /** Reset local config */
            agServer.defaultReset();

            Serial.println("Factory successful");
            ledBlinkDelay(LED_LONG_BLINK_DELAY);
            ledBlinkDelay(LED_LONG_BLINK_DELAY);
            ledBlinkDelay(LED_LONG_BLINK_DELAY);
            ESP.restart();
          }
        }

        /** Show current content cause reset ignore */
        factoryBtnPressTime = 0;
        ag.statusLed.setOff();
      }
    }
  } else {
    if (factoryBtnPressTime != 0) {
      ag.statusLed.setOff();
    }
    factoryBtnPressTime = 0;
  }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  esp_mqtt_client_handle_t client = event->client;
  int msg_id;
  switch ((esp_mqtt_event_id_t)event_id) {
  case MQTT_EVENT_CONNECTED:
    Serial.println("MQTT_EVENT_CONNECTED");
    // msg_id = esp_mqtt_client_subscribe(client, "helloworld", 0);
    // Serial.printf("sent subscribe successful, msg_id=%d\r\n", msg_id);
    agMqtt._connectionHandler(true);
    break;
  case MQTT_EVENT_DISCONNECTED:
    Serial.println("MQTT_EVENT_DISCONNECTED");
    agMqtt._connectionHandler(false);
    break;
  case MQTT_EVENT_SUBSCRIBED:
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    Serial.printf("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d\r\n", event->msg_id);
    break;
  case MQTT_EVENT_PUBLISHED:
    Serial.printf("MQTT_EVENT_PUBLISHED, msg_id=%d\r\n", event->msg_id);
    break;
  case MQTT_EVENT_DATA:
    Serial.println("MQTT_EVENT_DATA");
    // add null terminal to data
    // event->data[event->data_len] = 0;
    // rpc_attritbutes_handler(event->data, event->data_len);
    break;
  case MQTT_EVENT_ERROR:
    Serial.println("MQTT_EVENT_ERROR");
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
      Serial.printf("reported from esp-tls: %d",
                    event->error_handle->esp_tls_last_esp_err);
      Serial.printf("reported from tls stack: %d",
                    event->error_handle->esp_tls_stack_err);
      Serial.printf("captured as transport's socket errno: %d",
                    event->error_handle->esp_transport_sock_errno);
    }
    break;
  default:
    Serial.printf("Other event id:%d\r\n", event->event_id);
    break;
  }
}
