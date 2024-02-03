/*
This is the code for the AirGradient Open Air open-source hardware outdoor Air Quality Monitor with an ESP32-C3 Microcontroller.

It is an air quality monitor for PM2.5, CO2, TVOCs, NOx, Temperature and Humidity and can send data over Wifi.

Open source air quality monitors and kits are available:
Indoor Monitor: https://www.airgradient.com/indoor/
Outdoor Monitor: https://www.airgradient.com/outdoor/

Build Instructions: https://www.airgradient.com/documentation/open-air-pst-kit-1-3/

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.16-rc.2
"Arduino_JSON" by Arduino Version 0.2.0

Please make sure you have esp32 board manager installed. Tested with version 2.0.11.

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

#define DEBUG true

#define WIFI_CONNECT_COUNTDOWN_MAX 180 /** sec */
#define WIFI_HOTSPOT_PASSWORD_DEFAULT "cleanair"

AirGradient ag(BOARD_OUTDOOR_MONITOR_V1_3);

// time in seconds needed for NOx conditioning
uint16_t conditioning_s = 10;

String APIROOT = "http://hw.airgradient.com/";

typedef struct {
  bool inF;             /** Temperature unit */
  bool inUSAQI;         /** PMS standard */
  uint8_t ledBarMode;   /** @ref UseLedBar*/
  char model[16];       /** Model string value, Just define, don't know how much
                        memory usage */
  char mqttBroker[128]; /** Mqtt broker link */
  uint32_t _check;      /** Checksum configuration data */
} ServerConfig_t;
static ServerConfig_t serverConfig;

// set to true if you want to connect to wifi. You have 60 seconds to connect.
// Then it will go into an offline mode.
boolean connectWIFI = true;

static int ledSmState = APP_SM_NORMAL;
static bool serverFailed = false;
static bool configFailed = false;
static bool wifiHasConfig = false;

int loopCount = 0;

WiFiManager wifiManager; /** wifi manager instance */

unsigned long currentMillis = 0;

const int oledInterval = 5000;
unsigned long previousOled = 0;

const int sendToServerInterval = 60000;
const int pollServerConfigInterval = 30000;
const int co2CalibCountdown = 5; /** Seconds */
unsigned long previoussendToServer = 0;

const int tvocInterval = 1000;
unsigned long previousTVOC = 0;
int TVOC = -1;
int NOX = -1;

const int co2Interval = 5000;
unsigned long previousCo2 = 0;
int Co2 = 0;

const int pmInterval = 5000;
unsigned long previousPm = 0;
int pm25 = -1;
int pm01 = -1;
int pm10 = -1;
int pm03PCount = -1;
float temp;
int hum;

bool co2CalibrationRequest = false;
uint32_t serverConfigLoadTime = 0;
String HOTSPOT = "";

// const int tempHumInterval = 2500;
// unsigned long previousTempHum = 0;

void boardInit(void);
void failedHandler(String msg);
void getServerConfig(void);
void co2Calibration(void);

void setup() {
  if (DEBUG) {
    Serial.begin(115200);
  }

  /** Board init */
  boardInit();

  delay(500);

  countdown(3);

  if (connectWIFI) {
    connectToWifi();
  }

  if (WiFi.status() == WL_CONNECTED) {
    sendPing();
    Serial.println(F("WiFi connected!"));
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  getServerConfig();
  if (configFailed) {
    ledSmHandler(APP_SM_SENSOR_CONFIG_FAILED);
    delay(5000);
  }

  ledSmHandler(APP_SM_NORMAL);
}

void loop() {
  currentMillis = millis();
  updateTVOC();
  updateCo2();
  updatePm();
  sendToServer();
  getServerConfig();
}

void updateTVOC() {
  delay(1000);

  if (currentMillis - previousTVOC >= tvocInterval) {
    previousTVOC += tvocInterval;
    TVOC = ag.sgp41.getTvocIndex();
    NOX = ag.sgp41.getNoxIndex();
  }
}

void updateCo2() {
  if (currentMillis - previousCo2 >= co2Interval) {
    previousCo2 += co2Interval;
    Co2 = ag.s8.getCo2();
    Serial.printf("CO2: %d\r\n", Co2);
  }
}

void updatePm() {
  if (currentMillis - previousPm >= pmInterval) {
    previousPm += pmInterval;
    if (ag.pms5003t_1.readData()) {
      pm01 = ag.pms5003t_1.getPm01Ae();
      pm25 = ag.pms5003t_1.getPm25Ae();
      pm10 = ag.pms5003t_1.getPm10Ae();
      pm03PCount = ag.pms5003t_1.getPm03ParticleCount();
      temp = ag.pms5003t_1.getTemperature();
      hum = ag.pms5003t_1.getRelativeHumidity();
    }
  }
}

void sendPing() {
  String payload =
      "{\"wifi\":" + String(WiFi.RSSI()) + ", \"boot\":" + loopCount + "}";
  if (postToServer(payload)) {
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNNECTED);
  } else {
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNECT_FAILED);
  }
  delay(5000);
}

bool postToServer(String &payload) {
  String POSTURL = APIROOT +
                   "sensors/airgradient:" + String(getNormalizedMac()) +
                   "/measures";
  WiFiClient client;
  HTTPClient http;

  ag.statusLed.setOn();

  http.begin(client, POSTURL);
  http.addHeader("content-type", "application/json");
  int httpCode = http.POST(payload);
  Serial.printf("Post to %s, %d\r\n", POSTURL.c_str(), httpCode);
  http.end();

  ag.statusLed.setOff();

  return (httpCode == 200);
}

void sendToServer() {
  if (currentMillis - previoussendToServer >= sendToServerInterval) {
    previoussendToServer += sendToServerInterval;
    String payload =
        "{\"wifi\":" + String(WiFi.RSSI()) +
        (Co2 < 0 ? "" : ", \"rco2\":" + String(Co2)) +
        (pm01 < 0 ? "" : ", \"pm01\":" + String(pm01)) +
        (pm25 < 0 ? "" : ", \"pm02\":" + String(pm25)) +
        (pm10 < 0 ? "" : ", \"pm10\":" + String(pm10)) +
        (pm03PCount < 0 ? "" : ", \"pm003_count\":" + String(pm03PCount)) +
        (TVOC < 0 ? "" : ", \"tvoc_index\":" + String(TVOC)) +
        (NOX < 0 ? "" : ", \"nox_index\":" + String(NOX)) +
        ", \"atmp\":" + String(temp) +
        (hum < 0 ? "" : ", \"rhum\":" + String(hum)) +
        ", \"boot\":" + loopCount + "}";

    if (WiFi.status() == WL_CONNECTED) {
      postToServer(payload);
      resetWatchdog();
      loopCount++;
    } else {
      Serial.println("WiFi Disconnected");
    }
  }
}

void countdown(int from) {
  debug("\n");
  while (from > 0) {
    debug(String(from--));
    debug(" ");
    delay(1000);
  }
  debug("\n");
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
  HOTSPOT = "airgradient-" + String(getNormalizedMac());

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
  wifiManager.autoConnect(HOTSPOT.c_str(), WIFI_HOTSPOT_PASSWORD_DEFAULT);

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
  if (WiFi.isConnected() == false) {
    ledSmHandler(APP_SM_WIFI_MANAGER_CONNECT_FAILED);
  } else {
    wifiHasConfig = true;
  }
}

void debug(String msg) {
  if (DEBUG)
    Serial.print(msg);
}

void debug(int msg) {
  if (DEBUG)
    Serial.print(msg);
}

void debugln(String msg) {
  if (DEBUG)
    Serial.println(msg);
}

void debugln(int msg) {
  if (DEBUG)
    Serial.println(msg);
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

  if (ag.pms5003t_1.begin(Serial0) == false) {
    failedHandler("Init PMS5003T failed");
  }

  if (ag.s8.begin(Serial1) == false) {
    failedHandler("Init SenseAirS8 failed");
  }

  if (ag.sgp41.begin(Wire) == false) {
    failedHandler("Init SGP41 failed");
  }
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    vTaskDelay(1000);
  }
}

void updateServerConfigLoadTime(void) {
  serverConfigLoadTime = millis();
  if (serverConfigLoadTime == 0) {
    serverConfigLoadTime = 1;
  }
}

void showConfig(void) {
  Serial.println("Server configuration: ");
  Serial.printf("         inF: %s\r\n", serverConfig.inF ? "true" : "false");
  Serial.printf("     inUSAQI: %s\r\n",
                serverConfig.inUSAQI ? "true" : "false");
  Serial.printf("useRGBLedBar: %d\r\n", (int)serverConfig.ledBarMode);
  Serial.printf("       Model: %.*s\r\n", sizeof(serverConfig.model),
                serverConfig.model);
  Serial.printf(" Mqtt Broker: %.*s\r\n", sizeof(serverConfig.mqttBroker),
                serverConfig.mqttBroker);
}

void getServerConfig(void) {
  /** Only trigger load configuration again after pollServerConfigInterval sec
   */
  if (serverConfigLoadTime) {
    uint32_t ms = (uint32_t)(millis() - serverConfigLoadTime);
    if (ms < pollServerConfigInterval) {
      return;
    }
  }

  updateServerConfigLoadTime();

  Serial.println("Trigger load server configuration");
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(
        "Ignore get server configuration because WIFI not connected");
    return;
  }

  // WiFiClient wifiClient;
  HTTPClient httpClient;

  String getUrl = "http://hw.airgradient.com/sensors/airgradient:" +
                  String(getNormalizedMac()) + "/one/config";
  Serial.println("HttpClient get: " + getUrl);
  if (httpClient.begin(getUrl) == false) {
    Serial.println("HttpClient init failed");
    updateServerConfigLoadTime();
    return;
  }

  int respCode = httpClient.GET();

  /** get failure */
  if (respCode != 200) {
    Serial.printf("HttpClient get failed: %d\r\n", respCode);
    updateServerConfigLoadTime();
    httpClient.end();
    configFailed = true;
    return;
  }

  String respContent = httpClient.getString();
  Serial.println("Server config: " + respContent);
  httpClient.end();

  /** Parse JSON */
  JSONVar root = JSON.parse(respContent);
  if (JSON.typeof_(root) == "undefined") {
    Serial.println("Server configura JSON invalid");
    updateServerConfigLoadTime();
    configFailed = true;
    return;
  }
  configFailed = false;

  /** Get "country" */
  bool inF = serverConfig.inF;
  if (JSON.typeof_(root["country"]) == "string") {
    String country = root["country"];
    if (country == "US") {
      inF = true;
    } else {
      inF = false;
    }
  }

  /** Get "pmStandard" */
  bool inUSAQI = serverConfig.inUSAQI;
  if (JSON.typeof_(root["pmStandard"]) == "string") {
    String standard = root["pmStandard"];
    if (standard == "ugm3") {
      inUSAQI = false;
    } else {
      inUSAQI = true;
    }
  }

  /** Get CO2 "co2CalibrationRequested" */
  co2CalibrationRequest = false;
  if (JSON.typeof_(root["co2CalibrationRequested"]) == "boolean") {
    co2CalibrationRequest = root["co2CalibrationRequested"];
  }

  /** get "model" */
  String model = "";
  if (JSON.typeof_(root["model"]) == "string") {
    String _model = root["model"];
    model = _model;
  }

  /** get "mqttBrokerUrl" */
  String mqtt = "";
  if (JSON.typeof_(root["mqttBrokerUrl"]) == "string") {
    String _mqtt = root["mqttBrokerUrl"];
    mqtt = _mqtt;
  }

  if (inF != serverConfig.inF) {
    serverConfig.inF = inF;
  }
  if (inUSAQI != serverConfig.inUSAQI) {
    serverConfig.inUSAQI = inUSAQI;
  }
  if (model.length()) {
    if (model != String(serverConfig.model)) {
      memset(serverConfig.model, 0, sizeof(serverConfig.model));
      memcpy(serverConfig.model, model.c_str(), model.length());
    }
  }
  if (mqtt.length()) {
    if (mqtt != String(serverConfig.mqttBroker)) {
      memset(serverConfig.mqttBroker, 0, sizeof(serverConfig.mqttBroker));
      memcpy(serverConfig.mqttBroker, mqtt.c_str(), mqtt.length());
    }
  }

  /** Show server configuration */
  showConfig();

  /** Calibration */
  if (co2CalibrationRequest) {
    co2Calibration();
  }
}

void co2Calibration(void) {
  /** Count down for co2CalibCountdown secs */
  for (int i = 0; i < co2CalibCountdown; i++) {
    Serial.printf("Start CO2 calib after %d sec\r\n", co2CalibCountdown - i);
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
    ag.statusLed.setOn();
    delay(50);
    ag.statusLed.setOff();
    delay(950);
    ag.statusLed.setOn();
    delay(50);
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_WIFI_MANAGER_CONNECT_FAILED: {
    ag.statusLed.setOff();
    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 3 * 2; i++) {
        ag.statusLed.setToggle();
        delay(100);
      }
      delay(2000);
    }
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECT_FAILED: {
    ag.statusLed.setOff();
    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 4 * 2; i++) {
        ag.statusLed.setToggle();
        delay(100);
      }
      delay(2000);
    }
    ag.statusLed.setOff();
    break;
  }
  case APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED: {
    ag.statusLed.setOff();
    for (int j = 0; j < 3; j++) {
      for (int i = 0; i < 5 * 2; i++) {
        ag.statusLed.setToggle();
        delay(100);
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
