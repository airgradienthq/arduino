/*
This is the code for the AirGradient ONE open-source hardware indoor Air Quality
Monitor with an ESP32-C3 Microcontroller.

It is an air quality monitor for PM2.5, CO2, TVOCs, NOx, Temperature and
Humidity with a small display, an RGB led bar and can send data over Wifi.

Open source air quality monitors and kits are available:
Indoor Monitor: https://www.airgradient.com/indoor/
Outdoor Monitor: https://www.airgradient.com/outdoor/

Build Instructions: https://www.airgradient.com/documentation/one-v9/

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.16-rc.2
"Arduino_JSON" by Arduino version 0.2.0
"U8g2" by oliver version 2.34.22

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

Configuration parameters, e.g. Celsius / Fahrenheit or PM unit (US AQI vs ug/m3)
can be set through the AirGradient dashboard.

If you have any questions please visit our forum at
https://forum.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <WiFiManager.h>

#include <AirGradient.h>
#include <Arduino_JSON.h>
#include <U8g2lib.h>

/**
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
#define DISPLAY_DELAY_SHOW_CONTENT_MS 3000   /** ms */
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
      String _country = root["country"];
      country = _country;

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
    } else {
      co2Calib = false;
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

    /** Get "ledBarTestRequested" */
    if (JSON.typeof_(root["ledBarTestRequested"]) == "boolean") {
      ledBarTestRequested = root["ledBarTestRequested"];
    } else {
      ledBarTestRequested = false;
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
  int getCo2Abccalib(void) { return co2AbcCalib; }

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

  /**
   * @brief Get the Country
   *
   * @return String
   */
  String getCountry(void) { return country; }

private:
  bool inF;                 /** Temperature unit, true: F, false: C */
  bool inUSAQI;             /** PMS unit, true: USAQI, false: ugm3 */
  bool configFailed;        /** Flag indicate get server configuration failed */
  bool serverFailed;        /** Flag indicate post data to server failed */
  bool co2Calib;            /** Is co2Ppmcalibration requset */
  bool ledBarTestRequested; /** */
  int co2AbcCalib = -1;     /** update auto calibration number of day */
  UseLedBar ledBarMode = UseLedBarCO2; /** */
  char models[20];                     /** */
  char mqttBroker[256];                /** */
  String country;                      /***/
};
AgServer agServer;

/** Create airgradient instance for 'ONE_INDOOR' board */
AirGradient ag(ONE_INDOOR);

/** Create u8g2 instance */
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

/** wifi manager instance */
WiFiManager wifiManager;

static bool wifiHasConfig = false;      /** */
static int connectCountDown;            /** wifi configuration countdown */
static int ledCount;                    /** For LED animation */
static int ledSmState = APP_SM_NORMAL;  /** Save display SM */
static int dispSmState = APP_SM_NORMAL; /** Save LED SM */

static int tvocIndex = -1;
static int noxIndex = -1;
static int co2Ppm = -1;
static int pm25 = -1;
static int pm01 = -1;
static int pm10 = -1;
static int pm03PCount = -1;
static float temp = -1;
static int hum = -1;
static int bootCount;
static String wifiSSID = "";

static void boardInit(void);
static void failedHandler(String msg);
static void serverConfigPoll(void);
static void co2Calibration(void);
static void setRGBledPMcolor(int pm25Value);
static void ledSmHandler(int sm);
static void ledSmHandler(void);
static void dispSmHandler(int sm);
static void sensorLedColorHandler(void);
static void appLedHandler(void);
static void appDispHandler(void);
static void updateWiFiConnect(void);
static void updateDispLedBar(void);
static void tvocPoll(void);
static void pmPoll(void);
static void sendDataToServer(void);
static void tempHumPoll(void);
static void co2Poll(void);

/** Init schedule */
AgSchedule dispLedSchedule(DISP_UPDATE_INTERVAL, updateDispLedBar);
AgSchedule configSchedule(SERVER_CONFIG_UPDATE_INTERVAL, serverConfigPoll);
AgSchedule serverSchedule(SERVER_SYNC_INTERVAL, sendDataToServer);
AgSchedule co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Poll);
AgSchedule pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, pmPoll);
AgSchedule tempHumSchedule(SENSOR_TEMP_HUM_UPDATE_INTERVAL, tempHumPoll);
AgSchedule tvocSchedule(SENSOR_TVOC_UPDATE_INTERVAL, tvocPoll);

void setup() {
  /** Serial fore print debug message */
  Serial.begin(115200);

  /** Init I2C */
  Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin());

  /** Display init */
  u8g2.begin();

  /** Show boot display */
  displayShowText("One V9", "Lib Ver: " + ag.getVersion(), "");
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

  /** Init sensor */
  boardInit();

  /** Init AirGradient server */
  agServer.begin();

  /** WIFI connect */
  connectToWifi();

  /**
   * Send first data to ping server and get server configuration
   */
  if (WiFi.status() == WL_CONNECTED) {
    sendPing();
    Serial.println(F("WiFi connected!"));
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    /** Get first connected to wifi */
    agServer.pollServerConfig(getDevId());
    if (agServer.isConfigFailed()) {
      dispSmHandler(APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED);
      ledSmHandler(APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED);
      delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
    }
  }

  /** Show display Warning up */
  displayShowText("Warming Up", "Serial Number:", String(getNormalizedMac()));
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

  appLedHandler();
  appDispHandler();
}

void loop() {
  /** Handle schedule */
  dispLedSchedule.run();
  configSchedule.run();
  serverSchedule.run();
  co2Schedule.run();
  pmsSchedule.run();
  tempHumSchedule.run();
  tvocSchedule.run();

  /** Check for handle WiFi reconnect */
  updateWiFiConnect();
}

static void setTestColor(char color) {
  int r = 0;
  int g = 0;
  int b = 0;
  switch (color) {
  case 'g':
    g = 255;
    break;
  case 'y':
    r = 255;
    g = 255;
    break;
  case 'o':
    r = 255;
    g = 128;
    break;
  case 'r':
    r = 255;
    break;
  case 'b':
    b = 255;
    break;
  case 'w':
    r = 255;
    g = 255;
    b = 255;
    break;
  case 'p':
    r = 153;
    b = 153;
    break;
  case 'z':
    r = 102;
    break;
  case 'n':
  default:
    break;
  }
  ag.ledBar.setColor(r, g, b);
}

static void ledTest() {
  displayShowText("LED Test", "running", ".....");
  setTestColor('r');
  ag.ledBar.show();
  delay(1000);
  setTestColor('g');
  ag.ledBar.show();
  delay(1000);
  setTestColor('b');
  ag.ledBar.show();
  delay(1000);
  setTestColor('w');
  ag.ledBar.show();
  delay(1000);
  setTestColor('n');
  ag.ledBar.show();
  delay(1000);
}
static void ledTest2Min(void) {
  uint32_t tstart = millis();

  Serial.println("Start run LED test for 2 min");
  while (1) {
    ledTest();
    uint32_t ms = (uint32_t)(millis() - tstart);
    if (ms >= (60 * 1000 * 2)) {
      Serial.println("LED test after 2 min finish");
      break;
    }
  }
}

static void co2Poll(void) {
  co2Ppm = ag.s8.getCo2();
  Serial.printf("CO2 index: %d\r\n", co2Ppm);
}

static void sendPing() {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  root["boot"] = bootCount;

  /** Change disp and led state */
  dispSmHandler(APP_SM_WIFI_OK_SERVER_CONNECTING);
  ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNECTING);

  /** Task handle led connecting animation */
  xTaskCreate(
      [](void *obj) {
        for (;;) {
          ledSmHandler();
          if (ledSmState != APP_SM_WIFI_OK_SERVER_CONNECTING) {
            break;
          }
          delay(LED_BAR_ANIMATION_PERIOD);
        }
        vTaskDelete(NULL);
      },
      "task_led", 2048, NULL, 5, NULL);

  delay(1500);
  if (agServer.postToServer(getDevId(), JSON.stringify(root))) {
    dispSmHandler(APP_SM_WIFI_OK_SERVER_CONNNECTED);
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNNECTED);
  } else {
    dispSmHandler(APP_SM_WIFI_OK_SERVER_CONNECT_FAILED);
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNECT_FAILED);
  }
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

  ledSmHandler(APP_SM_NORMAL);
}

static void displayShowWifiText(String ln1, String ln2, String ln3,
                                String ln4) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_t0_16_tf);
    u8g2.drawStr(1, 10, ln1.c_str());
    u8g2.drawStr(1, 25, ln2.c_str());
    u8g2.drawStr(1, 40, ln3.c_str());
    u8g2.drawStr(1, 55, ln4.c_str());
  } while (u8g2.nextPage());
}

static void displayShowText(String ln1, String ln2, String ln3) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_t0_16_tf);
    u8g2.drawStr(1, 10, String(ln1).c_str());
    u8g2.drawStr(1, 30, String(ln2).c_str());
    u8g2.drawStr(1, 50, String(ln3).c_str());
  } while (u8g2.nextPage());
}

static void displayShowDashboard(String err) {
  char strBuf[10];

  /** Clear display dashboard */
  u8g2.firstPage();

  /** Show content to dashboard */
  do {
    u8g2.setFont(u8g2_font_t0_16_tf);
    if ((err == NULL) || err.isEmpty()) {
      /** Show temperature */
      if (agServer.isTemperatureUnitF()) {
        if (temp > -10001) {
          float tempF = (temp * 9 / 5) + 32;
          sprintf(strBuf, "%.1f°F", tempF);
        } else {
          sprintf(strBuf, "-°F");
        }
        u8g2.drawUTF8(1, 10, strBuf);
      } else {
        if (temp > -10001) {
          sprintf(strBuf, "%.1f°C", temp);
        } else {
          sprintf(strBuf, "-°C");
        }
        u8g2.drawUTF8(1, 10, strBuf);
      }

      /** Show humidity */
      if (hum >= 0) {
        sprintf(strBuf, "%d%%", hum);
      } else {
        sprintf(strBuf, " -%%");
      }
      if (hum > 99) {
        u8g2.drawStr(97, 10, strBuf);
      } else {
        u8g2.drawStr(105, 10, strBuf);
      }
    } else {
      Serial.println("Disp show error: " + err);
      int strWidth = u8g2.getStrWidth(err.c_str());
      u8g2.drawStr((126 - strWidth) / 2, 10, err.c_str());
    }

    /** Draw horizontal line */
    u8g2.drawLine(1, 13, 128, 13);

    /** Show CO2 label */
    u8g2.setFont(u8g2_font_t0_12_tf);
    u8g2.drawUTF8(1, 27, "CO2");

    /** Show CO2 value */
    u8g2.setFont(u8g2_font_t0_22b_tf);
    if (co2Ppm > 0) {
      sprintf(strBuf, "%d", co2Ppm);
    } else {
      sprintf(strBuf, "%s", "-");
    }
    u8g2.drawStr(1, 48, strBuf);

    /** Show CO2 value index */
    u8g2.setFont(u8g2_font_t0_12_tf);
    u8g2.drawStr(1, 61, "ppm");

    /** Draw vertical line */
    u8g2.drawLine(45, 14, 45, 64);
    u8g2.drawLine(82, 14, 82, 64);

    /** Draw PM2.5 label */
    u8g2.setFont(u8g2_font_t0_12_tf);
    u8g2.drawStr(48, 27, "PM2.5");

    /** Draw PM2.5 value */
    u8g2.setFont(u8g2_font_t0_22b_tf);
    if (agServer.isPMSinUSAQI()) {
      if (pm25 >= 0) {
        sprintf(strBuf, "%d", ag.pms5003.convertPm25ToUsAqi(pm25));
      } else {
        sprintf(strBuf, "%s", "-");
      }
      u8g2.drawStr(48, 48, strBuf);
      u8g2.setFont(u8g2_font_t0_12_tf);
      u8g2.drawUTF8(48, 61, "AQI");
    } else {
      if (pm25 >= 0) {
        sprintf(strBuf, "%d", pm25);
      } else {
        sprintf(strBuf, "%s", "-");
      }
      u8g2.drawStr(48, 48, strBuf);
      u8g2.setFont(u8g2_font_t0_12_tf);
      u8g2.drawUTF8(48, 61, "ug/m³");
    }

    /** Draw tvocIndexlabel */
    u8g2.setFont(u8g2_font_t0_12_tf);
    u8g2.drawStr(85, 27, "tvoc:");

    /** Draw tvocIndexvalue */
    if (tvocIndex >= 0) {
      sprintf(strBuf, "%d", tvocIndex);
    } else {
      sprintf(strBuf, "%s", "-");
    }
    u8g2.drawStr(85, 39, strBuf);

    /** Draw NOx label */
    u8g2.drawStr(85, 53, "NOx:");
    if (noxIndex >= 0) {
      sprintf(strBuf, "%d", noxIndex);
    } else {
      sprintf(strBuf, "%s", "-");
    }
    u8g2.drawStr(85, 63, strBuf);
  } while (u8g2.nextPage());
}

/**
 * @brief Must reset each 5min to avoid ESP32 reset
 */
static void resetWatchdog() { ag.watchdog.reset(); }

static bool wifiMangerClientConnected(void) {
  return WiFi.softAPgetStationNum() ? true : false;
}

// Wifi Manager
static void connectToWifi() {
  /** get wifi AP ssid */
  wifiSSID = "airgradient-" + String(getNormalizedMac());
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setTimeout(WIFI_CONNECT_COUNTDOWN_MAX);

  wifiManager.setAPCallback([](WiFiManager *obj) {
    /** This callback if wifi connnected failed and try to start configuration
     * portal */
    connectCountDown = WIFI_CONNECT_COUNTDOWN_MAX;
    ledCount = LED_BAR_COUNT_INIT_VALUE;
    dispSmState = APP_SM_WIFI_MANAGER_MODE;
    ledSmState = APP_SM_WIFI_MANAGER_MODE;
  });
  wifiManager.setSaveConfigCallback([]() {
    /** Wifi connected save the configuration */
    dispSmState = APP_SM_WIFI_MANAGER_STA_CONNECTED;
    ledSmHandler(APP_SM_WIFI_MANAGER_STA_CONNECTED);
  });
  wifiManager.setSaveParamsCallback([]() {
    /** Wifi set connect: ssid, password */
    ledCount = LED_BAR_COUNT_INIT_VALUE;
    dispSmState = APP_SM_WIFI_MANAGER_STA_CONNECTING;
    ledSmState = APP_SM_WIFI_MANAGER_STA_CONNECTING;
  });

  displayShowText("Connecting to", "config WiFi", "...");
  wifiManager.autoConnect(wifiSSID.c_str(), WIFI_HOTSPOT_PASSWORD_DEFAULT);
  xTaskCreate(
      [](void *obj) {
        while (wifiManager.getConfigPortalActive()) {
          wifiManager.process();
        }
        vTaskDelete(NULL);
      },
      "wifi_cfg", 4096, NULL, 10, NULL);

  /** Wait for WiFi connect and show LED, display status */
  uint32_t dispPeriod = millis();
  uint32_t ledPeriod = millis();
  bool clientConnectChanged = false;

  int dispSmStateOld = dispSmState;
  while (wifiManager.getConfigPortalActive()) {
    /** LED and display animation */
    if (WiFi.isConnected() == false) {
      /** Display countdown */
      uint32_t ms = (uint32_t)(millis() - dispPeriod);
      if (ms >= 1000) {
        dispPeriod = millis();
        dispSmHandler(dispSmState);
      } else {
        if (dispSmStateOld != dispSmState) {
          dispSmStateOld = dispSmState;
          dispSmHandler(dispSmState);
        }
      }

      /** LED animations */
      ms = (uint32_t)(millis() - ledPeriod);
      if (ms >= LED_BAR_ANIMATION_PERIOD) {
        ledPeriod = millis();
        ledSmHandler();
      }

      /** Check for client connect to change led color */
      bool clientConnected = wifiMangerClientConnected();
      if (clientConnected != clientConnectChanged) {
        clientConnectChanged = clientConnected;
        if (clientConnectChanged) {
          ledSmHandler(APP_SM_WIFI_MAMAGER_PORTAL_ACTIVE);
        } else {
          ledCount = LED_BAR_COUNT_INIT_VALUE;
          ledSmHandler(APP_SM_WIFI_MANAGER_MODE);
          dispSmHandler(APP_SM_WIFI_MANAGER_MODE);
        }
      }
    }
  }

  /** Show display wifi connect result failed */
  if (WiFi.isConnected() == false) {
    ledSmHandler(APP_SM_WIFI_MANAGER_CONNECT_FAILED);
    dispSmHandler(APP_SM_WIFI_MANAGER_CONNECT_FAILED);
    delay(6000);
  } else {
    wifiHasConfig = true;
  }

  /** Update LED bar color */
  appLedHandler();
}

static String getDevId(void) { return getNormalizedMac(); }

static String getNormalizedMac() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}

static void setRGBledCO2color(int co2Value) {
  if (co2Value >= 300 && co2Value < 800) {
    setRGBledColor('g');
  } else if (co2Value >= 800 && co2Value < 1000) {
    setRGBledColor('y');
  } else if (co2Value >= 1000 && co2Value < 1500) {
    setRGBledColor('o');
  } else if (co2Value >= 1500 && co2Value < 2000) {
    setRGBledColor('r');
  } else if (co2Value >= 2000 && co2Value < 3000) {
    setRGBledColor('p');
  } else if (co2Value >= 3000 && co2Value < 10000) {
    setRGBledColor('z');
  } else {
    setRGBledColor('n');
  }
}

static void setRGBledColor(char color) {
  int r = 0;
  int g = 0;
  int b = 0;
  switch (color) {
  case 'g':
    g = 255;
    break;
  case 'y':
    r = 255;
    g = 255;
    break;
  case 'o':
    r = 255;
    g = 128;
    break;
  case 'r':
    r = 255;
    break;
  case 'b':
    b = 255;
    break;
  case 'w':
    r = 255;
    g = 255;
    b = 255;
    break;
  case 'p':
    r = 153;
    b = 153;
    break;
  case 'z':
    r = 102;
    break;
  case 'n':
  default:
    break;
  }

  /** Sensor LED indicator has only show status on last 2 LED on LED Bar */
  int ledNum = ag.ledBar.getNumberOfLeds() - 1;
  ag.ledBar.setColor(r, g, b, ledNum);

  ledNum = ag.ledBar.getNumberOfLeds() - 2;
  ag.ledBar.setColor(r, g, b, ledNum);
}

/**
 * @brief Initialize board
 */
static void boardInit(void) {
  /** Init LED Bar */
  ag.ledBar.begin();

  /** Button init */
  ag.button.begin();

  /** Init sensor SGP41 */
  if (ag.sgp41.begin(Wire) == false) {
    failedHandler("Init SGP41 failed");
  }

  /** INit SHT */
  if (ag.sht4x.begin(Wire) == false) {
    failedHandler("Init SHT failed");
  }

  /** Init watchdog */
  ag.watchdog.begin();

  /** Init S8 CO2 sensor */
  if (ag.s8.begin(Serial1) == false) {
    failedHandler("Init SenseAirS8 failed");
  }

  /** Init PMS5003 */
  if (ag.pms5003.begin(Serial0) == false) {
    failedHandler("Init PMS5003 failed");
  }

  /** Run LED test on start up */
  displayShowText("Press now for", "LED test", "");
  bool test = false;
  uint32_t stime = millis();
  while (1) {
    if (ag.button.getState() == ag.button.BUTTON_PRESSED) {
      test = true;
      break;
    }
    delay(1);
    uint32_t ms = (uint32_t)(millis() - stime);
    if (ms >= 3000) {
      break;
    }
  }
  if (test) {
    ledTest();
  }
}

/**
 * @brief Failed handler
 *
 * @param msg Failure message
 */
static void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    vTaskDelay(1000);
  }
}

/**
 * @brief Send data to server
 */
static void serverConfigPoll(void) {
  if (agServer.pollServerConfig(getDevId())) {
    if (agServer.isCo2Calib()) {
      co2Calibration();
    }
    if (agServer.getCo2Abccalib() > 0) {
      if (ag.s8.setAutoCalib(agServer.getCo2Abccalib() * 24) == false) {
        Serial.println("Set S8 auto calib failed");
      }
    }
    if (agServer.isLedBarTestRequested()) {
      if (agServer.getCountry() == "TH") {
        ledTest2Min();
      } else {
        ledTest();
      }
    }
  }
}

/**
 * @brief Calibration CO2 sensor, it's base calibration, after calib complete
 * the value will be start at 400 if do calib on clean environment
 */
static void co2Calibration(void) {
  /** Count down for co2CalibCountdown secs */
  for (int i = 0; i < SENSOR_CO2_CALIB_COUNTDOWN_MAX; i++) {
    displayShowText(
        "Start CO2 calib",
        "after " + String(SENSOR_CO2_CALIB_COUNTDOWN_MAX - i) + " sec", "");
    delay(1000);
  }

  if (ag.s8.setBaselineCalibration()) {
    displayShowText("Calibration", "success", "");
    delay(1000);
    displayShowText("Wait for", "calib finish", "...");
    int count = 0;
    while (ag.s8.isBaseLineCalibrationDone() == false) {
      delay(1000);
      count++;
    }
    displayShowText("Calib finish", "after " + String(count), "sec");
    delay(2000);
  } else {
    displayShowText("Calibration", "failure!!!", "");
    delay(2000);
  }

  /** Update display */
  appDispHandler();
}

/**
 * @brief Set LED color for special PMS value
 *
 * @param pm25Value PMS2.5 value
 */
static void setRGBledPMcolor(int pm25Value) {
  if (pm25Value >= 0 && pm25Value < 10)
    setRGBledColor('g');
  if (pm25Value >= 10 && pm25Value < 35)
    setRGBledColor('y');
  if (pm25Value >= 35 && pm25Value < 55)
    setRGBledColor('o');
  if (pm25Value >= 55 && pm25Value < 150)
    setRGBledColor('r');
  if (pm25Value >= 150 && pm25Value < 250)
    setRGBledColor('p');
  if (pm25Value >= 250 && pm25Value < 1000)
    setRGBledColor('p');
}

static void singleLedAnimation(uint8_t r, uint8_t g, uint8_t b) {
  if (ledCount < 0) {
    ledCount = 0;
    ag.ledBar.setColor(r, g, b, ledCount);
  } else {
    ledCount++;
    if (ledCount >= ag.ledBar.getNumberOfLeds()) {
      ledCount = 0;
    }
    ag.ledBar.setColor(r, g, b, ledCount);
  }
}

/**
 * @brief LED state machine handler
 *
 * @param sm APP state machine
 */
static void ledSmHandler(int sm) {
  if (sm > APP_SM_NORMAL) {
    return;
  }

  ledSmState = sm;
  ag.ledBar.clear(); // Set all LED OFF
  switch (sm) {
  case APP_SM_WIFI_MANAGER_MODE: {
    /** In WiFi Manager Mode */
    /** Turn LED OFF */
    /** Turn midle LED Color */
    ag.ledBar.setColor(0, 0, 255, ag.ledBar.getNumberOfLeds() / 2);
    break;
  }
  case APP_SM_WIFI_MAMAGER_PORTAL_ACTIVE: {
    /** WiFi Manager has connected to mobile phone */
    ag.ledBar.setColor(0, 0, 255);
    break;
  }
  case APP_SM_WIFI_MANAGER_STA_CONNECTING: {
    /** after SSID and PW entered and OK clicked, connection to WiFI network is
     * attempted */
    singleLedAnimation(255, 255, 255);
    break;
  }
  case APP_SM_WIFI_MANAGER_STA_CONNECTED: {
    /** Connecting to WiFi worked */
    ag.ledBar.setColor(255, 255, 255);
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECTING: {
    /** once connected to WiFi an attempt to reach the server is performed */
    singleLedAnimation(0, 255, 0);
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNNECTED: {
    /** Server is reachable, all ﬁne */
    ag.ledBar.setColor(0, 255, 0);
    break;
  }
  case APP_SM_WIFI_MANAGER_CONNECT_FAILED: {
    /** Cannot connect to WiFi (e.g. wrong password, WPA Enterprise etc.) */
    ag.ledBar.setColor(255, 0, 0);
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECT_FAILED: {
    /** Connected to WiFi but server not reachable, e.g. ﬁrewall block/
     * whitelisting needed etc. */
    ag.ledBar.setColor(233, 183, 54); /** orange */
    break;
  }
  case APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED: {
    /** Server reachable but sensor not conﬁgured correctly */
    ag.ledBar.setColor(139, 24, 248); /** violet */
    break;
  }
  case APP_SM_WIFI_LOST: {
    /** Connection to WiFi network failed credentials incorrect encryption not
     * supported etc. */

    /** WIFI failed status LED color */
    ag.ledBar.setColor(255, 0, 0, 0);
    /** Show CO2 or PM color status */
    sensorLedColorHandler();
    break;
  }
  case APP_SM_SERVER_LOST: {
    /** Connected to WiFi network but the server cannot be reached through the
     * internet, e.g. blocked by ﬁrewall */

    ag.ledBar.setColor(233, 183, 54, 0);

    /** Show CO2 or PM color status */
    sensorLedColorHandler();
    break;
  }
  case APP_SM_SENSOR_CONFIG_FAILED: {
    /** Server is reachable but there is some conﬁguration issue to be ﬁxed on
     * the server side */

    ag.ledBar.setColor(139, 24, 248, 0);

    /** Show CO2 or PM color status */
    sensorLedColorHandler();
    break;
  }
  case APP_SM_NORMAL: {

    sensorLedColorHandler();
    break;
  }
  default:
    break;
  }
  ag.ledBar.show();
}

/**
 * @brief LED state machine handler
 */
static void ledSmHandler(void) { ledSmHandler(ledSmState); }

/**
 * @brief Display state machine handler
 *
 * @param sm APP state machine
 */
static void dispSmHandler(int sm) {
  if (sm > APP_SM_NORMAL) {
    return;
  }

  dispSmState = sm;

  switch (sm) {
  case APP_SM_WIFI_MANAGER_MODE:
  case APP_SM_WIFI_MAMAGER_PORTAL_ACTIVE: {
    if (connectCountDown >= 0) {
      displayShowWifiText(String(connectCountDown) + "s to connect",
                          "to WiFi hotspot:", "\"airgradient-",
                          getDevId() + "\"");
      connectCountDown--;
    }
    break;
  }
  case APP_SM_WIFI_MANAGER_STA_CONNECTING: {
    displayShowText("Trying to", "connect to WiFi", "...");
    break;
  }
  case APP_SM_WIFI_MANAGER_STA_CONNECTED: {
    displayShowText("WiFi connection", "successful", "");
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECTING: {
    displayShowText("Connecting to", "Server", "...");
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNNECTED: {
    displayShowText("Server", "connection", "successful");
    break;
  }
  case APP_SM_WIFI_MANAGER_CONNECT_FAILED: {
    displayShowText("WiFi not", "connected", "");
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECT_FAILED: {
    // displayShowText("Server not", "reachable", "");
    break;
  }
  case APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED: {
    displayShowText("Monitor not", "setup on", "dashboard");
    break;
  }
  case APP_SM_WIFI_LOST: {
    displayShowDashboard("WiFi N/A");
    break;
  }
  case APP_SM_SERVER_LOST: {
    displayShowDashboard("Server N/A");
    break;
  }
  case APP_SM_SENSOR_CONFIG_FAILED: {
    displayShowDashboard("Add to Dashboard");
    break;
  }
  case APP_SM_NORMAL: {
    displayShowDashboard("");
  }
  detault:
    break;
  }
}

/**
 * @brief Handle change LED color base on sensor value of CO2 and PMS
 */
static void sensorLedColorHandler(void) {
  switch (agServer.getLedBarMode()) {
  case UseLedBarCO2:
    setRGBledCO2color(co2Ppm);
    break;
  case UseLedBarPM:
    setRGBledPMcolor(pm25);
    break;
  default:
    ag.ledBar.setColor(0, 0, 0, ag.ledBar.getNumberOfLeds() - 1);
    ag.ledBar.setColor(0, 0, 0, ag.ledBar.getNumberOfLeds() - 2);
    break;
  }
}

/**
 * @brief APP LED color handler
 */
static void appLedHandler(void) {
  uint8_t state = APP_SM_NORMAL;
  if (WiFi.isConnected() == false) {
    state = APP_SM_WIFI_LOST;
  } else if (agServer.isConfigFailed()) {
    state = APP_SM_SENSOR_CONFIG_FAILED;
  } else if (agServer.isServerFailed()) {
    state = APP_SM_SERVER_LOST;
  }
  ledSmHandler(state);
}

/**
 * @brief APP display content handler
 */
static void appDispHandler(void) {
  uint8_t state = APP_SM_NORMAL;
  if (WiFi.isConnected() == false) {
    state = APP_SM_WIFI_LOST;
  } else if (agServer.isConfigFailed()) {
    state = APP_SM_SENSOR_CONFIG_FAILED;
  } else if (agServer.isServerFailed()) {
    state = APP_SM_SERVER_LOST;
  }
  dispSmHandler(state);
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
 * @brief APP display and LED handler
 *
 */
static void updateDispLedBar(void) {
  appDispHandler();
  appLedHandler();
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
  if (ag.pms5003.readData()) {
    pm01 = ag.pms5003.getPm01Ae();
    pm25 = ag.pms5003.getPm25Ae();
    pm10 = ag.pms5003.getPm10Ae();
    pm03PCount = ag.pms5003.getPm03ParticleCount();

    Serial.printf("      PMS0.1: %d\r\n", pm01);
    Serial.printf("      PMS2.5: %d\r\n", pm25);
    Serial.printf("     PMS10.0: %d\r\n", pm10);
    Serial.printf("PMS3.0 Count: %d\r\n", pm03PCount);
  } else {
    pm01 = -1;
    pm25 = -1;
    pm10 = -1;
    pm03PCount = -1;
  }
}

/**
 * @brief Send data to server
 *
 */
static void sendDataToServer(void) {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  if (co2Ppm >= 0) {
    root["rco2"] = co2Ppm;
  }
  if (pm01 >= 0) {
    root["pm01"] = pm01;
  }
  if (pm25 >= 0) {
    root["pm02"] = pm25;
  }
  if (pm10 >= 0) {
    root["pm10"] = pm10;
  }
  if (pm03PCount >= 0) {
    root["pm003_count"] = pm03PCount;
  }
  if (tvocIndex >= 0) {
    root["tvoc_index"] = tvocIndex;
  }
  if (noxIndex >= 0) {
    root["noxIndex"] = noxIndex;
  }
  if (temp >= 0) {
    root["atmp"] = temp;
  }
  if (hum >= 0) {
    root["rhum"] = hum;
  }
  root["boot"] = bootCount;

  // NOTE Need determine offline mode to reset watchdog timer
  if (agServer.postToServer(getDevId(), JSON.stringify(root))) {
    resetWatchdog();
  }
  bootCount++;
}

/**
 * @brief Update temperature and humidity value
 */
static void tempHumPoll(void) {
  temp = ag.sht4x.getTemperature();
  hum = ag.sht4x.getRelativeHumidity();

  Serial.printf("Temperature: %0.2f\r\n", temp);
  Serial.printf("   Humidity: %d\r\n", hum);
}
