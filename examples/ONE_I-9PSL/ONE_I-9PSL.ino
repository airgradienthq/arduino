/*
This is the code for the AirGradient ONE open-source hardware indoor Air Quality Monitor with an ESP32-C3 Microcontroller.

It is an air quality monitor for PM2.5, CO2, TVOCs, NOx, Temperature and Humidity with a small display, an RGB led bar and can send data over Wifi.

Open source air quality monitors and kits are available:
Indoor Monitor: https://www.airgradient.com/indoor/
Outdoor Monitor: https://www.airgradient.com/outdoor/

Build Instructions: https://www.airgradient.com/documentation/one-v9/

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.16-rc.2
"Arduino_JSON" by Arduino version 0.2.0
"U8g2" by oliver version 2.34.22

Please make sure you have esp32 board manager installed. Tested with version 2.0.11.

Important flashing settings:
- Set board to "ESP32C3 Dev Module"
- Enable "USB CDC On Boot"
- Flash frequency "80Mhz"
- Flash mode "QIO"
- Flash size "4MB"
- Partition scheme "Default 4MB with spiffs (1.2MB APP/1,5MB SPIFFS)"
- JTAG adapter "Disabled"

Configuration parameters, e.g. Celsius / Fahrenheit or PM unit (US AQI vs ug/m3) can be set through the AirGradient dashboard.

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

#define WIFI_CONNECT_COUNTDOWN_MAX 180 /** sec */
#define LED_BAR_COUNT_INIT_VALUE (-1)
#define LED_BAR_ANIMATION_PERIOD 100         /** ms */
#define DISP_UPDATE_INTERVAL 5000            /** ms */
#define SERVER_CONFIG_UPDATE_INTERVAL 30000  /** ms */
#define SERVER_SYNC_INTERVAL 60000           /** ms */
#define SENSOR_CO2_CALIB_COUNTDOWN_MAX 5     /** sec */
#define SENSOR_TVOC_UPDATE_INTERVAL 1000     /** ms */
#define SENSOR_CO2_UPDATE_INTERVAL 5000      /** ms */
#define SENSOR_PM_UPDATE_INTERVAL 5000       /** ms */
#define SENSOR_TEMP_HUM_UPDATE_INTERVAL 5000 /** ms */
#define WIFI_HOTSPOT_PASSWORD_DEFAULT "cleanair"

/**
 * @brief Use use LED bar state
 *
 */
typedef enum {
  UseLedBarOff, /** Don't use LED bar */
  UseLedBarPM,  /** Use LED bar for PMS */
  UseLedBarCO2, /** Use LED bar for CO2 */
} UseLedBar;

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

#define DEBUG true

/** Create airgradient instance */
AirGradient ag(BOARD_ONE_INDOOR_MONITOR_V9_0);

WiFiManager wifiManager;                /** wifi manager instance */
static int connectCountDown;            /** wifi configuration countdown */
static int ledCount;                    /** For LED animation */
static int ledSmState = APP_SM_NORMAL;  /** Save display SM */
static int dispSmState = APP_SM_NORMAL; /** Save LED SM */
static bool configFailed = false; /** Save is get server configuration failed */
static bool serverFailed = false; /** Save is send server failed */

// Display bottom right
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

String APIROOT = "http://hw.airgradient.com/";

bool co2CalibrationRequest = false;
bool ledBarTestRequested = false;
uint32_t serverConfigLoadTime = 0;
String HOSTPOT = "";
static bool wifiHasConfig = false;

// set to true if you want to connect to wifi. You have 60 seconds to connect.
// Then it will go into an offline mode.
boolean connectWIFI = true;

int loopCount = 0;

unsigned long currentMillis = 0;
unsigned long previousOled = 0;
unsigned long previoussendToServer = 0;

unsigned long previousTVOC = 0;
int TVOC = -1;
int NOX = -1;

unsigned long previousCo2 = 0;
int Co2 = 0;

unsigned long previousPm = 0;
int pm25 = -1;
int pm01 = -1;
int pm10 = -1;
int pm03PCount = -1;

unsigned long previousTempHum = 0;
float temp;
int hum;

int buttonConfig = 0;
PushButton::State lastState = PushButton::BUTTON_RELEASED;
PushButton::State currentState;
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;

void boardInit(void);
void failedHandler(String msg);
void getServerConfig(void);
void co2Calibration(void);
void setRGBledPMcolor(int pm25Value);
void setWifiConnectLedColor(void);
void setWifiConnectingLedColor(void);
void ledSmHandler(int sm);
void ledSmHandler(void);
void dispSmHandler(int sm);
void sensorLedColorHandler(void);
void appLedHandler(void);
void appDispHandler(void);
void appLedUnUseHandler(void);
void updateWiFiConnect(void);

void setup() {
  if (DEBUG) {
    Serial.begin(115200);
  }

  /** Init default server configuration */
  serverConfig.inF = false;
  serverConfig.inUSAQI = false;
  serverConfig.ledBarMode = UseLedBarCO2;

  /** Init I2C */
  Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin());

  /** Display init */
  u8g2.begin();

  /** Show boot display */
  displayShowText("One V9", "Lib Ver: " + ag.getVersion(), "");
  delay(2000); /** Boot display wait */

  /** Init sensor */
  boardInit();

  /** WIFI connect */
  if (connectWIFI) {
    connectToWifi();
  }
  if (WiFi.status() == WL_CONNECTED) {
    sendPing();
    Serial.println(F("WiFi connected!"));
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    /** Get first connected to wifi */
    getServerConfig();
    if (configFailed) {
      dispSmHandler(APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED);
      ledSmHandler(APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED);
      delay(5000);
    }
  }

  /** Show display Warning up */
  displayShowText("Warming Up", "Serial Number:", String(getNormalizedMac()));
  delay(10000);

  appLedHandler();
  appDispHandler();
}

void loop() {
  currentMillis = millis();
  updateTVOC();
  updateDispLedBar();
  updateCo2();
  updatePm();
  updateTempHum();
  sendToServer();
  getServerConfig();
  updateWiFiConnect();
}

void ledTest() {
  displayShowText("LED Test", "running", ".....");
  setRGBledColor('r');
  delay(1000);
  setRGBledColor('g');
  delay(1000);
  setRGBledColor('b');
  delay(1000);
  setRGBledColor('w');
  delay(1000);
  setRGBledColor('n');
  delay(1000);
  // LED Test
}

void updateTVOC() {
  if (currentMillis - previousTVOC >= SENSOR_TVOC_UPDATE_INTERVAL) {
    previousTVOC += SENSOR_TVOC_UPDATE_INTERVAL;

    TVOC = ag.sgp41.getTvocIndex();
    NOX = ag.sgp41.getNoxIndex();

    Serial.println(String(TVOC));
    Serial.println(String(NOX));
  }
}

void updateCo2() {
  if (currentMillis - previousCo2 >= SENSOR_CO2_UPDATE_INTERVAL) {
    previousCo2 += SENSOR_CO2_UPDATE_INTERVAL;
    Co2 = ag.s8.getCo2();
    Serial.println(String(Co2));
  }
}

void updatePm() {
  if (currentMillis - previousPm >= SENSOR_PM_UPDATE_INTERVAL) {
    previousPm += SENSOR_PM_UPDATE_INTERVAL;
    if (ag.pms5003.readData()) {
      pm01 = ag.pms5003.getPm01Ae();
      pm25 = ag.pms5003.getPm25Ae();
      pm10 = ag.pms5003.getPm10Ae();
      pm03PCount = ag.pms5003.getPm03ParticleCount();
    } else {
      pm01 = -1;
      pm25 = -1;
      pm10 = -1;
      pm03PCount = -1;
    }
  }
}

void updateTempHum() {
  if (currentMillis - previousTempHum >= SENSOR_TEMP_HUM_UPDATE_INTERVAL) {
    previousTempHum += SENSOR_TEMP_HUM_UPDATE_INTERVAL;
    temp = ag.sht.getTemperature();
    hum = ag.sht.getRelativeHumidity();
  }
}

void updateDispLedBar() {
  if (currentMillis - previousOled >= DISP_UPDATE_INTERVAL) {
    previousOled += DISP_UPDATE_INTERVAL;
    appDispHandler();
    appLedHandler();
  }
}

void sendPing() {
  String payload =
      "{\"wifi\":" + String(WiFi.RSSI()) + ", \"boot\":" + loopCount + "}";
  dispSmHandler(APP_SM_WIFI_OK_SERVER_CONNECTING);
  delay(1500);
  if (postToServer(payload)) {
    dispSmHandler(APP_SM_WIFI_OK_SERVER_CONNNECTED);
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNNECTED);
  } else {
    dispSmHandler(APP_SM_WIFI_OK_SERVER_CONNECT_FAILED);
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNECT_FAILED);
  }
  delay(5000);
}

void displayShowText(String ln1, String ln2, String ln3) {
  char buf[9];
  // u8g2.firstPage();
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_t0_16_tf);
    u8g2.drawStr(1, 10, String(ln1).c_str());
    u8g2.drawStr(1, 30, String(ln2).c_str());
    u8g2.drawStr(1, 50, String(ln3).c_str());
  } while (u8g2.nextPage());
}

void displayShowDashboard(String err) {
  char buf[9];

  /** Clear display dashboard */
  u8g2.firstPage();

  /** Show content to dashboard */
  do {
    u8g2.setFont(u8g2_font_t0_16_tf);
    if ((err == NULL) || err.isEmpty()) {
      /** Show temperature */
      if (serverConfig.inF) {
        if (temp > -10001) {
          float tempF = (temp * 9 / 5) + 32;
          sprintf(buf, "%.1f°F", tempF);
        } else {
          sprintf(buf, "-°F");
        }
        u8g2.drawUTF8(1, 10, buf);
      } else {
        if (temp > -10001) {
          sprintf(buf, "%.1f°C", temp);
        } else {
          sprintf(buf, "-°C");
        }
        u8g2.drawUTF8(1, 10, buf);
      }

      /** Show humidity */
      if (hum >= 0) {
        sprintf(buf, "%d%%", hum);
      } else {
        sprintf(buf, " -%%");
      }
      if (hum > 99) {
        u8g2.drawStr(97, 10, buf);
      } else {
        u8g2.drawStr(105, 10, buf);
        // there might also be single digits, not considered, sprintf might
        // actually support a leading space
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
    if (Co2 > 0) {
      sprintf(buf, "%d", Co2);
    } else {
      sprintf(buf, "%s", "-");
    }
    u8g2.drawStr(1, 48, buf);

    /** Show CO2 value index */
    u8g2.setFont(u8g2_font_t0_12_tf);
    u8g2.drawStr(1, 61, "ppm");

    /** Draw vertical line */
    u8g2.drawLine(45, 15, 45, 64);

    /** Draw PM2.5 label */
    u8g2.setFont(u8g2_font_t0_12_tf);
    u8g2.drawStr(48, 27, "PM2.5");

    /** Draw PM2.5 value */
    u8g2.setFont(u8g2_font_t0_22b_tf);
    if (serverConfig.inUSAQI) {
      if (pm25 >= 0) {
        sprintf(buf, "%d", ag.pms5003.convertPm25ToUsAqi(pm25));
      } else {
        sprintf(buf, "%s", "-");
      }
      u8g2.drawStr(48, 48, buf);
      u8g2.setFont(u8g2_font_t0_12_tf);
      u8g2.drawUTF8(48, 61, "AQI");
    } else {
      if (pm25 >= 0) {
        sprintf(buf, "%d", pm25);
      } else {
        sprintf(buf, "%s", "-");
      }
      u8g2.drawStr(48, 48, buf);
      u8g2.setFont(u8g2_font_t0_12_tf);
      u8g2.drawUTF8(48, 61, "ug/m³");
    }

    /** Draw vertical line */
    u8g2.drawLine(82, 15, 82, 64);

    /** Draw TVOC label */
    u8g2.setFont(u8g2_font_t0_12_tf);
    u8g2.drawStr(85, 27, "TVOC:");

    /** Draw TVOC value */
    if (TVOC >= 0) {
      sprintf(buf, "%d", TVOC);
    } else {
      sprintf(buf, "%s", "-");
    }
    u8g2.drawStr(85, 39, buf);

    /** Draw NOx label */
    u8g2.drawStr(85, 53, "NOx:");
    if (NOX >= 0) {
      sprintf(buf, "%d", NOX);
    } else {
      sprintf(buf, "%s", "-");
    }
    u8g2.drawStr(85, 63, buf);
  } while (u8g2.nextPage());
}

bool postToServer(String payload) {
  Serial.println(payload);
  String POSTURL = APIROOT +
                   "sensors/airgradient:" + String(getNormalizedMac()) +
                   "/measures";
  Serial.println(POSTURL);
  WiFiClient client;
  HTTPClient http;
  http.begin(client, POSTURL);
  http.addHeader("content-type", "application/json");
  int httpCode = http.POST(payload);
  String response = http.getString();
  Serial.println(httpCode);
  Serial.println(response);
  http.end();

  return (httpCode == 200);
}

void sendToServer() {
  if (currentMillis - previoussendToServer >= SERVER_SYNC_INTERVAL) {
    previoussendToServer += SERVER_SYNC_INTERVAL;
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
      Serial.println(payload);
      if (postToServer(payload)) {
        serverFailed = false;
      } else {
        serverFailed = true;
      }
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

void resetWatchdog() { ag.watchdog.reset(); }

bool wifiMangerClientConnected(void) {
  return WiFi.softAPgetStationNum() ? true : false;
}

// Wifi Manager
void connectToWifi() {
  /** get wifi AP ssid */
  HOSTPOT = "airgradient-" + String(getNormalizedMac());
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
  wifiManager.autoConnect(HOSTPOT.c_str(), WIFI_HOTSPOT_PASSWORD_DEFAULT);
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

void setRGBledCO2color(int co2Value) {
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

void setRGBledColor(char color) {
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
  int ledNum = ag.ledBar.getNumberOfLed() - 1;
  ag.ledBar.setColor(r, g, b, ledNum);

  ledNum = ag.ledBar.getNumberOfLed() - 2;
  ag.ledBar.setColor(r, g, b, ledNum);
}

void boardInit(void) {
  /** Init LED Bar */
  ag.ledBar.begin();

  /** Button init */
  ag.button.begin();

  /** Init sensor SGP41 */
  if (ag.sgp41.begin(Wire) == false) {
    failedHandler("Init SGP41 failed");
  }

  /** INit SHT */
  if (ag.sht.begin(Wire) == false) {
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
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    vTaskDelay(1000);
  }
}

void getServerConfigLoadTime(void) {
  serverConfigLoadTime = millis();
  if (serverConfigLoadTime == 0) {
    serverConfigLoadTime = 1;
  }
}

void getServerConfig(void) {
  /** Only trigger load configuration again after pollServerConfigInterval sec
   */
  if (serverConfigLoadTime) {
    uint32_t ms = (uint32_t)(millis() - serverConfigLoadTime);
    if (ms < SERVER_CONFIG_UPDATE_INTERVAL) {
      return;
    }
  }

  getServerConfigLoadTime();

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
    getServerConfigLoadTime();
    configFailed = true;
    return;
  }

  int respCode = httpClient.GET();

  /** get failure */
  if (respCode != 200) {
    Serial.printf("HttpClient get failed: %d\r\n", respCode);
    getServerConfigLoadTime();

    /** Close client */
    httpClient.end();

    configFailed = true;

    return;
  }
  configFailed = false;

  /** Get configuration content */
  String respContent = httpClient.getString();
  Serial.println("Server config: " + respContent);

  /** close client */
  httpClient.end();

  /** Parse JSON */
  JSONVar root = JSON.parse(respContent);
  if (JSON.typeof_(root) == "undefined") {
    Serial.println("Server configura JSON invalid");
    getServerConfigLoadTime();
    return;
  }

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

  /** Get "ledBarTestRequested" */
  if (JSON.typeof_(root["ledBarTestRequested"]) == "boolean") {
    ledBarTestRequested = root["ledBarTestRequested"];
  }

  /** get "ledBarMode" */
  uint8_t ledBarMode = serverConfig.ledBarMode;
  if (JSON.typeof_(root["ledBarMode"]) == "string") {
    String _ledBarMode = root["ledBarMode"];
    if (_ledBarMode == "co2") {
      ledBarMode = UseLedBarCO2;
    } else if (_ledBarMode == "pm") {
      ledBarMode = UseLedBarPM;
    } else if (_ledBarMode == "off") {
      ledBarMode = UseLedBarOff;
    }
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
  if (ledBarMode != serverConfig.ledBarMode) {
    serverConfig.ledBarMode = ledBarMode;
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
  showServerConfig();

  /** Calibration */
  if (ledBarTestRequested) {
    ledTest();
  }
  if (co2CalibrationRequest) {
    co2Calibration();
  }

  /** Update LED status */
  appLedHandler();
}

void showServerConfig(void) {
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

void co2Calibration(void) {
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

void setRGBledPMcolor(int pm25Value) {
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

void setWifiConnectLedColor(void) { ag.ledBar.setColor(0, 0, 255); }

void setWifiConnectingLedColor(void) { ag.ledBar.setColor(255, 255, 255); }

void singleLedAnimation(uint8_t r, uint8_t g, uint8_t b) {
  if (ledCount < 0) {
    ledCount = 0;
    ag.ledBar.setColor(r, g, b, ledCount);
  } else {
    ledCount++;
    if (ledCount >= ag.ledBar.getNumberOfLed()) {
      ledCount = 0;
    }
    ag.ledBar.setColor(r, g, b, ledCount);
  }
}

void ledSmHandler(int sm) {
  if (sm > APP_SM_NORMAL) {
    return;
  }

  ledSmState = sm;
  ag.ledBar.clear();  // Set all LED OFF
  switch (sm) {
  case APP_SM_WIFI_MANAGER_MODE: {
    /** In WiFi Manager Mode */
    /** Turn LED OFF */
    /** Turn midle LED Color */
    ag.ledBar.setColor(0, 0, 255, ag.ledBar.getNumberOfLed() / 2);
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

void ledSmHandler(void) { ledSmHandler(ledSmState); }

void dispSmHandler(int sm) {
  if (sm > APP_SM_NORMAL) {
    return;
  }

  dispSmState = sm;

  switch (sm) {
  case APP_SM_WIFI_MANAGER_MODE:
  case APP_SM_WIFI_MAMAGER_PORTAL_ACTIVE: {
    if (connectCountDown >= 0) {
      displayShowText(String(connectCountDown) + "s to connect",
                      "to airgradient-", String(getNormalizedMac()));
      connectCountDown--;
    }
    break;
  }
  case APP_SM_WIFI_MANAGER_STA_CONNECTING: {
    displayShowText("Trying to", "connect to WiFi", "..");
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
    //displayShowText("Server not", "reachable", "");
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

void sensorLedColorHandler(void) {
  switch (serverConfig.ledBarMode) {
  case UseLedBarCO2:
    setRGBledCO2color(Co2);
    break;
  case UseLedBarPM:
    setRGBledPMcolor(pm25);
    break;
  default:
    ag.ledBar.setColor(0, 0, 0, ag.ledBar.getNumberOfLed() - 1);
    ag.ledBar.setColor(0, 0, 0, ag.ledBar.getNumberOfLed() - 2);
    break;
  }
}

void appLedHandler(void) {
  uint8_t state = APP_SM_NORMAL;
  if (WiFi.isConnected() == false) {
    state = APP_SM_WIFI_LOST;
  } else if (configFailed) {
    state = APP_SM_SENSOR_CONFIG_FAILED;
  } else if (serverFailed) {
    state = APP_SM_SERVER_LOST;
  }
  ledSmHandler(state);
}

void appDispHandler(void) {
  uint8_t state = APP_SM_NORMAL;
  if (WiFi.isConnected() == false) {
    state = APP_SM_WIFI_LOST;
  } else if (configFailed) {
    state = APP_SM_SENSOR_CONFIG_FAILED;
  } else if (serverFailed) {
    state = APP_SM_SERVER_LOST;
  }
  dispSmHandler(state);
}

void updateWiFiConnect(void) {
  static uint32_t lastRetry;
  if (wifiHasConfig == false) {
    return;
  }
  if (WiFi.isConnected()) {
    lastRetry = millis();
    return;
  }
  uint32_t ms = (uint32_t)(millis() - lastRetry);
  if (ms >= 10000) {
    lastRetry = millis();
    WiFi.reconnect();
  }
}
