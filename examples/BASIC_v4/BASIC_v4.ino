/*
This is the code for the AirGradient DIY BASIC Air Quality Monitor with an D1
ESP8266 Microcontroller.

It is an air quality monitor for PM2.5, CO2, Temperature and Humidity with a
small display and can send data over Wifi.

Open source air quality monitors and kits are available:
Indoor Monitor: https://www.airgradient.com/indoor/
Outdoor Monitor: https://www.airgradient.com/outdoor/

Build Instructions:
https://www.airgradient.com/documentation/diy-v4/

Following libraries need to be installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.16-rc.2
"Arduino_JSON" by Arduino version 0.2.0
"U8g2" by oliver version 2.34.22

Please make sure you have esp8266 board manager installed. Tested with
version 3.1.2.

Set board to "LOLIN(WEMOS) D1 R2 & mini"

Configuration parameters, e.g. Celsius / Fahrenheit or PM unit (US AQI vs ug/m3)
can be set through the AirGradient dashboard.

If you have any questions please visit our forum at
https://forum.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include <AirGradient.h>
#include <Arduino_JSON.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h>

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
    WiFiClient wifiClient;
    HTTPClient client;
    if (client.begin(wifiClient, uri) == false) {
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
    Serial.printf(" S8 calib period: %d\r\n", co2AbcCalib);
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

/** Create airgradient instance for 'DIY_BASIC' board */
AirGradient ag = AirGradient(DIY_BASIC);

static int co2Ppm = -1;
static int pm25 = -1;
static float temp = -1;
static int hum = -1;
static long val;
static String wifiSSID = "";
static bool wifiHasConfig = false; /** */

static void boardInit(void);
static void failedHandler(String msg);
static void co2Calibration(void);
static void serverConfigPoll(void);
static void co2Poll(void);
static void pmPoll(void);
static void tempHumPoll(void);
static void sendDataToServer(void);
static void dispHandler(void);
static String getDevId(void);
static void updateWiFiConnect(void);

AgSchedule configSchedule(SERVER_CONFIG_UPDATE_INTERVAL, serverConfigPoll);
AgSchedule serverSchedule(SERVER_SYNC_INTERVAL, sendDataToServer);
AgSchedule dispSchedule(DISP_UPDATE_INTERVAL, dispHandler);
AgSchedule co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Poll);
AgSchedule pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, pmPoll);
AgSchedule tempHumSchedule(SENSOR_TEMP_HUM_UPDATE_INTERVAL, tempHumPoll);

void setup() {
  Serial.begin(115200);

  /** Init I2C */
  Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin());
  delay(1000);

  /** Board init */
  boardInit();

  /** Init AirGradient server */
  agServer.begin();

  /** Show boot display */
  displayShowText("DIY basic", "Lib:" + ag.getVersion(), "");
  delay(2000);

  /** WiFi connect */
  connectToWifi();
  if (WiFi.status() == WL_CONNECTED) {
    wifiHasConfig = true;
    sendPing();

    agServer.pollServerConfig(getDevId());
    if (agServer.isCo2Calib()) {
      co2Calibration();
    }
  }

  /** Show serial number display */
  ag.display.clear();
  ag.display.setCursor(1, 1);
  ag.display.setText("Warm Up");
  ag.display.setCursor(1, 15);
  ag.display.setText("Serial#");
  ag.display.setCursor(1, 29);
  String id = getNormalizedMac();
  Serial.println("Device id: " + id);
  String id1 = id.substring(0, 9);
  String id2 = id.substring(9, 12);
  ag.display.setText("\'" + id1);
  ag.display.setCursor(1, 40);
  ag.display.setText(id2 + "\'");
  ag.display.show();

  delay(5000);
}

void loop() {
  configSchedule.run();
  serverSchedule.run();
  dispSchedule.run();
  co2Schedule.run();
  pmsSchedule.run();
  tempHumSchedule.run();

  updateWiFiConnect();
}

static void sendPing() {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  root["boot"] = 0;

  // delay(1500);
  if (agServer.postToServer(getDevId(), JSON.stringify(root))) {
    // Ping Server succses
  } else {
    // Ping server failed
  }
  // delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
}

void displayShowText(String ln1, String ln2, String ln3) {
  char buf[9];
  ag.display.clear();

  ag.display.setCursor(1, 1);
  ag.display.setText(ln1);
  ag.display.setCursor(1, 19);
  ag.display.setText(ln2);
  ag.display.setCursor(1, 37);
  ag.display.setText(ln3);

  ag.display.show();
}

// Wifi Manager
void connectToWifi() {
  WiFiManager wifiManager;
  wifiSSID = "AG-" + String(ESP.getChipId(), HEX);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConfigPortalTimeout(WIFI_CONNECT_COUNTDOWN_MAX);
  wifiManager.autoConnect(wifiSSID.c_str(), WIFI_HOTSPOT_PASSWORD_DEFAULT);

  uint32_t lastTime = millis();
  int count = WIFI_CONNECT_COUNTDOWN_MAX;
  displayShowText(String(WIFI_CONNECT_COUNTDOWN_MAX) + " sec",
                  "SSID:", wifiSSID);
  while (wifiManager.getConfigPortalActive()) {
    wifiManager.process();
    uint32_t ms = (uint32_t)(millis() - lastTime);
    if (ms >= 1000) {
      lastTime = millis();
      displayShowText(String(count) + " sec", "SSID:", wifiSSID);
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
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
  }
}

static void boardInit(void) {
  /** Init SHT sensor */
  if (ag.sht.begin(Wire) == false) {
    failedHandler("SHT init failed");
  }

  /** CO2 init */
  if (ag.s8.begin(&Serial) == false) {
    failedHandler("SenseAirS8 init failed");
  }

  /** PMS init */
  if (ag.pms5003.begin(&Serial) == false) {
    failedHandler("PMS5003 init failed");
  }

  /** Display init */
  ag.display.begin(Wire);
  ag.display.setTextColor(1);
}

static void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}

static void co2Calibration(void) {
  /** Count down for co2CalibCountdown secs */
  for (int i = 0; i < SENSOR_CO2_CALIB_COUNTDOWN_MAX; i++) {
    displayShowText("CO2 calib", "after",
                    String(SENSOR_CO2_CALIB_COUNTDOWN_MAX - i) + " sec");
    delay(1000);
  }

  if (ag.s8.setBaselineCalibration()) {
    displayShowText("Calib", "success", "");
    delay(1000);
    displayShowText("Wait for", "finish", "...");
    int count = 0;
    while (ag.s8.isBaseLineCalibrationDone() == false) {
      delay(1000);
      count++;
    }
    displayShowText("Finish", "after", String(count) + " sec");
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
  } else {
    displayShowText("Calib", "failure!!!", "");
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
  }
}

static void serverConfigPoll(void) {
  if (agServer.pollServerConfig(getDevId())) {
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

static void co2Poll() {
  co2Ppm = ag.s8.getCo2();
  Serial.printf("CO2 index: %d\r\n", co2Ppm);
}

void pmPoll() {
  if (ag.pms5003.readData()) {
    pm25 = ag.pms5003.getPm25Ae();
    Serial.printf("PMS2.5: %d\r\n", pm25);
  } else {
    pm25 = -1;
  }
}

static void tempHumPoll() {
  if (ag.sht.measure()) {
    temp = ag.sht.getTemperature();
    hum = ag.sht.getRelativeHumidity();
    Serial.printf("Temperature: %0.2f\r\n", temp);
    Serial.printf("   Humidity: %d\r\n", hum);
  } else {
    Serial.println("Meaure SHT failed");
  }
}

static void sendDataToServer() {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  if (co2Ppm >= 0) {
    root["rco2"] = co2Ppm;
  }
  if (pm25 >= 0) {
    root["pm02"] = pm25;
  }
  if (temp >= 0) {
    root["atmp"] = temp;
  }
  if (hum >= 0) {
    root["rhum"] = hum;
  }

  if (agServer.postToServer(getDevId(), JSON.stringify(root)) == false) {
    Serial.println("Post to server failed");
  }
}

static void dispHandler() {
  String ln1 = "";
  String ln2 = "";
  String ln3 = "";

  if (agServer.isPMSinUSAQI()) {
    ln1 = "AQI:" + String(ag.pms5003.convertPm25ToUsAqi(pm25));
  } else {
    ln1 = "PM :" + String(pm25) + " ug";
  }
  ln2 = "CO2:" + String(co2Ppm);

  if (agServer.isTemperatureUnitF()) {
    ln3 = String((temp * 9 / 5) + 32).substring(0, 4) + " " + String(hum) + "%";
  } else {
    ln3 = String(temp).substring(0, 4) + " " + String(hum) + "%";
  }
  displayShowText(ln1, ln2, ln3);
}

static String getDevId(void) { return getNormalizedMac(); }

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

String getNormalizedMac() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}
