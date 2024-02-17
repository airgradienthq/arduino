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

#include <AirGradient.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <WiFiManager.h>
#include <Wire.h>

/**
 *
 * @brief Application state machine state
 *
 */
enum {
  APP_SM_WIFI_MANAGER_MODE,           /** In WiFi Manger Mode */
  APP_SM_WIFI_MAMAGER_PORTAL_ACTIVE,  /** WiFi Manager has connected to mobile
                                         phone */
  APP_SM_WIFI_MANAGER_STA_CONNECTING, /** After SSID and PW entered and OK
                                        clicked, connection to WiFI network is
                                        attempted*/
  APP_SM_WIFI_MANAGER_STA_CONNECTED,  /** Connecting to WiFi worked */
  APP_SM_WIFI_OK_SERVER_CONNECTING,   /** Once connected to WiFi an attempt to
                                         reach the server is performed */
  APP_SM_WIFI_OK_SERVER_CONNNECTED,   /** Server is reachable, all ﬁne */
  /** Exceptions during WIFi Setup */
  APP_SM_WIFI_MANAGER_CONNECT_FAILED,   /** Cannot connect to WiFi (e.g. wrong
                                            password, WPA Enterprise etc.) */
  APP_SM_WIFI_OK_SERVER_CONNECT_FAILED, /** Connected to WiFi but server not
                                          reachable, e.g. ﬁrewall block/
                                          whitelisting needed etc. */
  APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED, /** Server reachable but sensor
                                                   not conﬁgured correctly*/

  /** During Normal Operation */
  APP_SM_WIFI_LOST, /** Connection to WiFi network failed credentials incorrect
                       encryption not supported etc. */
  APP_SM_SERVER_LOST, /** Connected to WiFi network but the server cannot be
                         reached through the internet, e.g. blocked by ﬁrewall
                       */
  APP_SM_SENSOR_CONFIG_FAILED, /** Server is reachable but there is some
                                  conﬁguration issue to be ﬁxed on the server
                                  side */
  APP_SM_NORMAL,
};

#define LED_FAST_BLINK_DELAY 250             /** ms */
#define LED_SLOW_BLINK_DELAY 1000            /** ms */
#define WIFI_CONNECT_COUNTDOWN_MAX 180       /** sec */
#define WIFI_CONNECT_RETRY_MS 10000          /** ms */
#define LED_BAR_COUNT_INIT_VALUE (-1)        /** */
#define LED_BAR_ANIMATION_PERIOD 100         /** ms */
#define DISP_UPDATE_INTERVAL 5000            /** ms */
#define SERVER_CONFIG_UPDATE_INTERVAL 30000  /** ms */
#define SERVER_SYNC_INTERVAL 60000           /** ms */
#define SENSOR_CO2_CALIB_COUNTDOWN_MAX 5     /** sec */
#define SENSOR_TVOC_UPDATE_INTERVAL 1000     /** ms */
#define SENSOR_CO2_UPDATE_INTERVAL 5000      /** ms */
#define SENSOR_PM_UPDATE_INTERVAL 5000       /** ms */
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

      Serial.printf("[AgSchedule] handle 0x%08x, period: %d(ms)\r\n",
                    (unsigned int)handler, period);

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
    inF = false;
    inUSAQI = false;
    configFailed = false;
    serverFailed = false;
    memset(models, 0, sizeof(models));
    memset(mqttBroker, 0, sizeof(mqttBroker));
  }

  /**
   * @brief Get server configuration
   *
   * @param id Device ID
   * @return true Success
   * @return false Failure
   */
  bool pollServerConfig(String id) {
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
    if (JSON.typeof_(root["country"]) == "string") {
      String country = root["country"];
      if (country == "US") {
        inF = true;
      } else {
        inF = false;
      }
    }

    /** Get "pmStandard" */
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
    }

    /** Get "ledBarMode" */
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
    if (JSON.typeof_(root["model"]) == "string") {
      String model = root["model"];
      if (model.length()) {
        int len =
            model.length() < sizeof(models) ? model.length() : sizeof(models);
        memset(models, 0, sizeof(models));
        memcpy(models, model.c_str(), len);
      }
    }

    /** Get "mqttBrokerUrl" */
    if (JSON.typeof_(root["mqttBrokerUrl"]) == "string") {
      String mqtt = root["mqttBrokerUrl"];
      if (mqtt.length()) {
        int len = mqtt.length() < sizeof(mqttBroker) ? mqtt.length()
                                                     : sizeof(mqttBroker);
        memset(mqttBroker, 0, sizeof(mqttBroker));
        memcpy(mqttBroker, mqtt.c_str(), len);
      }
    }

    /** Get 'abcDays' */
    if (JSON.typeof_(root["abcDays"]) == "number") {
      co2AbcCalib = root["abcDays"];
    } else {
      co2AbcCalib = -1;
    }

    /** Show configuration */
    showServerConfig();

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
  bool isTemperatureUnitF(void) { return inF; }

  /**
   * @brief Get PMS standard unit
   *
   * @return true USAQI
   * @return false ugm3
   */
  bool isPMSinUSAQI(void) { return inUSAQI; }

  /**
   * @brief Get status of get server coniguration is failed
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
  String getModelName(void) { return String(models); }

  /**
   * @brief Get mqttBroker url
   *
   * @return String Broker url, empty if server failed
   */
  String getMqttBroker(void) { return String(mqttBroker); }

  /**
   * @brief Show server configuration parameter
   */
  void showServerConfig(void) {
    Serial.println("Server configuration: ");
    Serial.printf("             inF: %s\r\n", inF ? "true" : "false");
    Serial.printf("         inUSAQI: %s\r\n", inUSAQI ? "true" : "false");
    Serial.printf("    useRGBLedBar: %d\r\n", (int)ledBarMode);
    Serial.printf("           Model: %s\r\n", models);
    Serial.printf("     Mqtt Broker: %s\r\n", mqttBroker);
    Serial.printf("  abcDays period: %d\r\n", co2AbcCalib);
  }

  /**
   * @brief Get server config led bar mode
   *
   * @return UseLedBar
   */
  UseLedBar getLedBarMode(void) { return ledBarMode; }

private:
  bool inF;             /** Temperature unit, true: F, false: C */
  bool inUSAQI;         /** PMS unit, true: USAQI, false: ugm3 */
  bool configFailed;    /** Flag indicate get server configuration failed */
  bool serverFailed;    /** Flag indicate post data to server failed */
  bool co2Calib;        /** Is co2Ppmcalibration requset */
  int co2AbcCalib = -1; /** update auto calibration number of day */
  UseLedBar ledBarMode = UseLedBarCO2; /** */
  char models[20];                     /** */
  char mqttBroker[256];                /** */
};
AgServer agServer;

/** Create airgradient instance for 'OPEN_AIR_OUTDOOR' board */
AirGradient ag(OPEN_AIR_OUTDOOR);

static int ledSmState = APP_SM_NORMAL;

int loopCount = 0;

WiFiManager wifiManager; /** wifi manager instance */
static bool wifiHasConfig = false;
static String wifiSSID = "";

int tvocIndex = -1;
int noxIndex = -1;
int co2Ppm = 0;

int pm25_1 = -1;
int pm01_1 = -1;
int pm10_1 = -1;
int pm03PCount_1 = -1;
float temp_1;
int hum_1;

int pm25_2 = -1;
int pm01_2 = -1;
int pm10_2 = -1;
int pm03PCount_2 = -1;
float temp_2;
int hum_2;

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
  FW_MODE_PP   /** PMS5003T_1, PMS5003T_2 */
};
int fw_mode = FW_MODE_PST;

void boardInit(void);
void failedHandler(String msg);
void co2Calibration(void);
static String getDevId(void);
static void updateWiFiConnect(void);
static void tvocPoll(void);
static void pmPoll(void);
static void sendDataToServer(void);
static void co2Poll(void);
static void serverConfigPoll(void);
static const char *getFwMode(int mode);

AgSchedule configSchedule(SERVER_CONFIG_UPDATE_INTERVAL, serverConfigPoll);
AgSchedule serverSchedule(SERVER_SYNC_INTERVAL, sendDataToServer);
AgSchedule co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Poll);
AgSchedule pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, pmPoll);
AgSchedule tvocSchedule(SENSOR_TVOC_UPDATE_INTERVAL, tvocPoll);

void setup() {
  Serial.begin(115200);

  /** Board init */
  boardInit();

  /** Server init */
  agServer.begin();

  /** WiFi connect */
  connectToWifi();

  if (WiFi.isConnected()) {
    wifiHasConfig = true;
    sendPing();

    agServer.pollServerConfig(getDevId());
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
    co2Schedule.run();
  }
  pmsSchedule.run();
  if (fw_mode == FW_MODE_PST || fw_mode == FW_MODE_PPT) {
    tvocSchedule.run();
  }
  updateWiFiConnect();
}

void sendPing() {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  root["boot"] = loopCount;
  if (agServer.postToServer(getDevId(), JSON.stringify(root))) {
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNNECTED);
  } else {
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNECT_FAILED);
  }
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
}

static void sendDataToServer(void) {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  root["boot"] = loopCount;
  if (fw_mode == FW_MODE_PST) {
    if (co2Ppm >= 0) {
      root["rco2"] = co2Ppm;
    }
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
    if (tvocIndex >= 0) {
      root["tvoc_index"] = tvocIndex;
    }
    if (noxIndex >= 0) {
      root["noxIndex"] = noxIndex;
    }
    if (temp_1 >= 0) {
      root["atmp"] = temp_1;
    }
    if (hum_1 >= 0) {
      root["rhum"] = hum_1;
    }
  } else if (fw_mode == FW_MODE_PPT) {
    if (tvocIndex > 0) {
      root["tvoc_index"] = loopCount;
    }
    if (noxIndex > 0) {
      root["nox_index"] = loopCount;
    }
  }

  if ((fw_mode == FW_MODE_PP) || (fw_mode == FW_MODE_PPT)) {
    root["pm01"] = (int)((pm01_1 + pm01_2) / 2);
    root["pm02"] = (int)((pm25_1 + pm25_2) / 2);
    root["pm003_count"] = (int)((pm03PCount_1 + pm03PCount_2) / 2);
    root["atmp"] = (int)((temp_1 + temp_2) / 2);
    root["rhum"] = (int)((hum_1 + hum_2) / 2);
    root["channels"]["1"]["pm01"] = pm01_1;
    root["channels"]["1"]["pm02"] = pm25_1;
    root["channels"]["1"]["pm10"] = pm10_1;
    root["channels"]["1"]["pm003_count"] = pm03PCount_1;
    root["channels"]["1"]["atmp"] = temp_1;
    root["channels"]["1"]["rhum"] = hum_1;
    root["channels"]["2"]["pm01"] = pm01_2;
    root["channels"]["2"]["pm02"] = pm25_2;
    root["channels"]["2"]["pm10"] = pm10_2;
    root["channels"]["2"]["pm003_count"] = pm03PCount_2;
    root["channels"]["2"]["atmp"] = temp_2;
    root["channels"]["2"]["rhum"] = hum_2;
  }

  /** Send data to sensor */
  if (agServer.postToServer(getDevId(), JSON.stringify(root))) {
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
          ledSmHandler(APP_SM_WIFI_MAMAGER_PORTAL_ACTIVE);
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

  ag.watchdog.begin();

  ag.button.begin();

  ag.statusLed.begin();

  /** detect sensor: PMS5003, PMS5003T, SGP41 and S8 */
  if (ag.s8.begin(Serial1) == false) {
    Serial.println("S8 not detect run mode 'PPT'");
    fw_mode = FW_MODE_PPT;

    /** De-initialize Serial1 */
    Serial1.end();
  }
  if (ag.sgp41.begin(Wire) == false) {
    if (fw_mode == FW_MODE_PST) {
      failedHandler("Init SGP41 failed");
    } else {
      Serial.println("SGP41 not detect run mode 'PP'");
      fw_mode = FW_MODE_PP;
    }
  }

  if (ag.pms5003t_1.begin(Serial0) == false) {
    failedHandler("Init PMS5003T_1 failed");
  }
  if (fw_mode != FW_MODE_PST) {
    if (ag.pms5003t_2.begin(Serial1) == false) {
      failedHandler("Init PMS5003T_2 failed");
    }
  }

  if (fw_mode != FW_MODE_PST) {
    pmsSchedule.setPeriod(2000);
  }

  Serial.printf("Firmware node: %s\r\n", getFwMode(fw_mode));
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
    Serial.printf("Start CO2 calib after %d sec\r\n",
                  SENSOR_CO2_CALIB_COUNTDOWN_MAX - i);
    delay(1000);
  }

  if (ag.s8.setBaselineCalibration()) {
    Serial.println("Calibration success");
    delay(1000);
    Serial.println("Wait for calib finish...");
    int count = 0;
    while (ag.s8.isBaseLineCalibrationDone() == false) {
      delay(1000);
      count++;
    }
    Serial.printf("Calib finish after %d sec\r\n", count);
    delay(2000);
  } else {
    Serial.println("Calibration failure!!!");
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

    Serial.printf("Re-Connect WiFi\r\n");
  }
}

/**
 * @brief Update tvocIndexindex
 *
 */
static void tvocPoll(void) {
  tvocIndex = ag.sgp41.getTvocIndex();
  noxIndex = ag.sgp41.getNoxIndex();

  Serial.printf("tvocIndexindex: %d\r\n", tvocIndex);
  Serial.printf(" NOx index: %d\r\n", noxIndex);
}

/**
 * @brief Update PMS data
 *
 */
static void pmPoll(void) {
  if (fw_mode == FW_MODE_PST) {
    if (ag.pms5003t_1.readData()) {
      pm01_1 = ag.pms5003t_1.getPm01Ae();
      pm25_1 = ag.pms5003t_1.getPm25Ae();
      pm25_1 = ag.pms5003t_1.getPm10Ae();
      pm03PCount_1 = ag.pms5003t_1.getPm03ParticleCount();
      temp_1 = ag.pms5003t_1.getTemperature();
      hum_1 = ag.pms5003t_1.getRelativeHumidity();
    }
  } else {
    if (ag.pms5003t_1.readData() && ag.pms5003t_2.readData()) {
      pm1Value01 = pm1Value01 + ag.pms5003t_1.getPm01Ae();
      pm1Value25 = pm1Value25 + ag.pms5003t_1.getPm25Ae();
      pm1Value10 = pm1Value10 + ag.pms5003t_1.getPm10Ae();
      pm1PCount = pm1PCount + ag.pms5003t_1.getPm03ParticleCount();
      pm1temp = pm1temp + ag.pms5003t_1.getTemperature();
      pm1hum = pm1hum + ag.pms5003t_1.getRelativeHumidity();
      pm2Value01 = pm2Value01 + ag.pms5003t_2.getPm01Ae();
      pm2Value25 = pm2Value25 + ag.pms5003t_2.getPm25Ae();
      pm2Value10 = pm2Value10 + ag.pms5003t_2.getPm10Ae();
      pm2PCount = pm2PCount + ag.pms5003t_2.getPm03ParticleCount();
      pm2temp = pm2temp + ag.pms5003t_2.getTemperature();
      pm2hum = pm2hum + ag.pms5003t_2.getRelativeHumidity();
      countPosition++;
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
  }
}

static void co2Poll(void) {
  co2Ppm = ag.s8.getCo2();
  Serial.printf("CO2 index: %d\r\n", co2Ppm);
}

static void serverConfigPoll(void) {
  if (agServer.pollServerConfig(getDevId())) {
    /** Only support CO2 S8 sensor on FW_MODE_PST */
    if (fw_mode == FW_MODE_PST) {
      if (agServer.isCo2Calib()) {
        co2Calibration();
      }
      if (agServer.getCo2AbcDaysConfig() > 0) {
        Serial.printf("abcDays config: %d days(%d hours)\r\n",
                      agServer.getCo2AbcDaysConfig(),
                      agServer.getCo2AbcDaysConfig() * 24);
        Serial.printf("Current config: %d (hours)\r\n", ag.s8.getAbcPeriod());
        if (ag.s8.setAbcPeriod(agServer.getCo2AbcDaysConfig() * 24) == false) {
          Serial.println("Set S8 abcDays period calib failed");
        } else {
          Serial.println("Set S8 abcDays period calib success");
        }
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
  case APP_SM_WIFI_MAMAGER_PORTAL_ACTIVE: {
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
  case APP_SM_WIFI_OK_SERVER_CONNNECTED: {
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
  default:
    break;
  }
  return "FW_MODE_UNKNOW";
}
