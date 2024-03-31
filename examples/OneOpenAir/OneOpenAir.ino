/*
This is the combined firmware code for AirGradient ONE and AirGradient Open Air
open-source hardware Air Quality Monitor with ESP32-C3 Microcontroller.

It is an air quality monitor for PM2.5, CO2, TVOCs, NOx, Temperature and
Humidity with a small display, an RGB led bar and can send data over Wifi.

Open source air quality monitors and kits are available:
Indoor Monitor: https://www.airgradient.com/indoor/
Outdoor Monitor: https://www.airgradient.com/outdoor/

Build Instructions: AirGradient ONE:
https://www.airgradient.com/documentation/one-v9/ Build Instructions:
AirGradient Open Air:
https://www.airgradient.com/documentation/open-air-pst-kit-1-3/

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
- Partition scheme "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"
- JTAG adapter "Disabled"

Configuration parameters, e.g. Celsius / Fahrenheit or PM unit (US AQI vs ug/m3)
can be set through the AirGradient dashboard.

If you have any questions please visit our forum at
https://forum.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include "mqtt_client.h"
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <WiFiManager.h>

#include "EEPROM.h"
#include "LocalConfig.h"
#include <AirGradient.h>
#include <Arduino_JSON.h>
#include <ESPmDNS.h>
#include <U8g2lib.h>
#include <WebServer.h>

/**
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
  APP_SM_SENSOR_CONFIG_FAILED, /** Server is reachable but there is some
                                  conﬁguration issue to be fixed on the server
                                  side */
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
#define DISP_UPDATE_INTERVAL 2500            /** ms */
#define SERVER_CONFIG_UPDATE_INTERVAL 15000  /** ms */
#define SERVER_SYNC_INTERVAL 60000           /** ms */
#define MQTT_SYNC_INTERVAL 60000             /** ms */
#define SENSOR_CO2_CALIB_COUNTDOWN_MAX 5     /** sec */
#define SENSOR_TVOC_UPDATE_INTERVAL 1000     /** ms */
#define SENSOR_CO2_UPDATE_INTERVAL 4000      /** ms */
#define SENSOR_PM_UPDATE_INTERVAL 2000       /** ms */
#define SENSOR_TEMP_HUM_UPDATE_INTERVAL 2000 /** ms */
#define DISPLAY_DELAY_SHOW_CONTENT_MS 2000   /** ms */
#define WIFI_HOTSPOT_PASSWORD_DEFAULT                                          \
  "cleanair" /** default WiFi AP password                                      \
              */

/** I2C define */
#define I2C_SDA_PIN 7
#define I2C_SCL_PIN 6
#define OLED_I2C_ADDR 0x3C

enum {
  FW_MODE_I_9PSL, /** ONE_INDOOR */
  FW_MODE_O_1PST, /** PMS5003T, S8 and SGP41 */
  FW_MODE_O_1PPT, /** PMS5003T_1, PMS5003T_2, SGP41 */
  FW_MODE_O_1PP,  /** PMS5003T_1, PMS5003T_2 */
  FW_MDOE_O_1PS   /** PMS5003T, S8 */
};

/**
 * @brief Schedule handle with timing period
 *
 */
class AgSchedule {
public:
  AgSchedule(int period, void (*handler)(void))
      : period(period), handler(handler) {}

  /**
   * @brief Handle schedule
   *
   */
  void run(void) {
    uint32_t ms = (uint32_t)(millis() - count);
    if (ms >= period) {
      handler();
      count = millis();
    }
  }
  /**
   * @brief Set schedule handle period
   *
   * @param period
   */
  void setPeriod(int period) { this->period = period; }

private:
  void (*handler)(void); /** Callback handle */
  int period;            /** Schedule handle period */
  int count;             /** Schedule time count check */
};

/**
 * @brief AirGradient server configuration and sync data
 */
class AgServer {
public:
  AgServer(LocalConfig &localConfig) : config(localConfig) {}

  /**
   * @brief Initialize airgradient server, it's load the server configuration if
   * failed load it to default.
   *
   */
  void begin(void) {
    configFailed = false;
    serverFailed = false;
  }

  /**
   * @brief Fetch server configuration, if get sucessed and configuratrion
   * parameter has changed store into local storage
   *
   * @param id Device ID
   * @return true Success
   * @return false Failure
   */
  bool fetchServerConfiguration(String id) {
    if (config.isLocallyControlled()) {
      Serial.println("Ignore fetch server configuration");
      return false;
    }

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

    return config.parse(respContent, false);
  }

  /**
   * @brief Post data to Airgradient server
   *
   * @param id Device Id
   * @param payload Data payload
   * @return true Success
   * @return false Failure
   */
  bool postToServer(String id, String payload) {
    if (config.isPostDataToAirGradient() == false) {
      Serial.println("Ignore post to Airgrdient server");
      return true;
    }

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

private:
  bool configFailed; /** Flag indicate get server configuration failed */
  bool serverFailed; /** Flag indicate post data to server failed */
  LocalConfig &config;
};

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
                                       mqtt_event_handler, this) != ESP_OK) {
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

  /**
   * @brief Update mqtt connection changed
   *
   * @param connected
   */
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

static AgMqtt agMqtt;
static TaskHandle_t mqttTask = NULL;
static LocalConfig localConfig(Serial);
static AgServer agServer(localConfig);

static AirGradient *ag;
static U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0,
                                               /* reset=*/U8X8_PIN_NONE);
static WiFiManager wifiManager;
static WebServer webServer;

static bool wifiHasConfig = false;      /** */
static int connectCountDown;            /** wifi configuration countdown */
static int ledCount;                    /** For LED animation */
static int ledSmState = APP_SM_NORMAL;  /** Save display SM */
static int dispSmState = APP_SM_NORMAL; /** Save LED SM */

/** Init schedule */
static bool hasSensorS8 = true;
static bool hasSensorPMS1 = true;
static bool hasSensorPMS2 = true;
static bool hasSensorSGP = true;
static bool hasSensorSHT = true;
static int pmFailCount = 0;
static uint32_t factoryBtnPressTime = 0;
static int getCO2FailCount = 0;
static uint32_t addToDashboardTime;
static bool isAddToDashboard = true;
static bool offlineMode = false;
static int fwMode = FW_MODE_I_9PSL;

static int tvocIndex = -1;
static int tvocRawIndex = -1;
static int noxIndex = -1;
static int noxRawIndex = -1;
static int co2Ppm = -1;
static float temp = -1001;
static int hum = -1;
static int bootCount;
static String wifiSSID = "";

static int pm25_1 = -1;
static int pm01_1 = -1;
static int pm10_1 = -1;
static int pm03PCount_1 = -1;
static float temp_1 = -1001;
static int hum_1 = -1;

static int pm25_2 = -1;
static int pm01_2 = -1;
static int pm10_2 = -1;
static int pm03PCount_2 = -1;
static float temp_2 = -1001;
static int hum_2 = -1;

static int pm1Value01;
static int pm1Value25;
static int pm1Value10;
static int pm1PCount;
static int pm1temp;
static int pm1hum;
static int pm2Value01;
static int pm2Value25;
static int pm2Value10;
static int pm2PCount;
static int pm2temp;
static int pm2hum;
static int countPosition;
const int targetCount = 20;

static bool ledBarButtonTest = false;
static bool localConfigUpdate = false;

static void boardInit(void);
static void failedHandler(String msg);
static void updateServerConfiguration(void);
static void co2Calibration(void);
static void setRGBledPMcolor(int pm25Value);
static void ledSmHandler(int sm);
static void ledSmHandler(void);
static void dispSmHandler(int sm);
static void sensorLedColorHandler(void);
static void appLedHandler(void);
static void appDispHandler(void);
static void updateWiFiConnect(void);
static void displayAndLedBarUpdate(void);
static void tvocUpdate(void);
static void pmUpdate(void);
static void sendDataToServer(void);
static void tempHumUpdate(void);
static void co2Update(void);
static void showNr(void);
static void webServerInit(void);
static String getServerSyncData(bool localServer);
static void createMqttTask(void);
static void factoryConfigReset(void);
static void wdgFeedUpdate(void);

AgSchedule dispLedSchedule(DISP_UPDATE_INTERVAL, displayAndLedBarUpdate);
AgSchedule configSchedule(SERVER_CONFIG_UPDATE_INTERVAL,
                          updateServerConfiguration);
AgSchedule serverSchedule(SERVER_SYNC_INTERVAL, sendDataToServer);
AgSchedule co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Update);
AgSchedule pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, pmUpdate);
AgSchedule tempHumSchedule(SENSOR_TEMP_HUM_UPDATE_INTERVAL, tempHumUpdate);
AgSchedule tvocSchedule(SENSOR_TVOC_UPDATE_INTERVAL, tvocUpdate);
AgSchedule wdgFeedSchedule(60000, wdgFeedUpdate);

void setup() {
  /** Serial for print debug message */
  Serial.begin(115200);
  delay(100); /** For bester show log */
  showNr();

  /** Initialize local configure */
  localConfig.begin();

  /** Init I2C */
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  delay(1000);

  /** Detect board type */
  Wire.beginTransmission(OLED_I2C_ADDR);
  if (Wire.endTransmission() == 0x00) {
    Serial.println("Detect ONE_INDOOR");
    ag = new AirGradient(BoardType::ONE_INDOOR);
  } else {
    Serial.println("Detect OPEN_AIR");
    ag = new AirGradient(BoardType::OPEN_AIR_OUTDOOR);
  }

  /** Init AirGradient server */
  agServer.begin();

  /** Init sensor */
  boardInit();

  /** Connecting wifi */
  if (isOneIndoor()) {
    if (ledBarButtonTest) {
      ledTest();
    } else {
      /** Check LED mode to disabled LED */
      if (localConfig.getLedBarMode() == UseLedBarOff) {
        ag->ledBar.setEnable(false);
      }
      connectToWifi();
    }
  } else {
    connectToWifi();
  }

  /**
   * Send first data to ping server and get server configuration
   */
  if (WiFi.isConnected()) {
    webServerInit();

    /** MQTT init */
    if (localConfig.getMqttBrokerUri().isEmpty() == false) {
      if (agMqtt.begin(localConfig.getMqttBrokerUri())) {
        createMqttTask();
        Serial.println("MQTT client init success");
      } else {
        Serial.println("MQTT client init failure");
      }
    }

    sendPing();
    Serial.println(F("WiFi connected!"));
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    /** Get first connected to wifi */
    agServer.fetchServerConfiguration(getDevId());
    if (agServer.isConfigFailed()) {
      if (isOneIndoor()) {
        dispSmHandler(APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED);
      }
      ledSmHandler(APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED);
      delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
    } else {
      ag->ledBar.setEnable(localConfig.getLedBarMode() != UseLedBarOff);
    }
  } else {
    offlineMode = true;
  }

  /** Show display Warning up */
  if (isOneIndoor()) {
    displayShowText("Warming Up", "Serial Number:", String(getNormalizedMac()));
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
  }

  appLedHandler();
  if (isOneIndoor()) {
    appDispHandler();
  }
}

void loop() {
  /** Handle schedule */
  dispLedSchedule.run();
  configSchedule.run();
  serverSchedule.run();

  if (hasSensorS8) {
    co2Schedule.run();
  }

  if (hasSensorPMS1 || hasSensorPMS2) {
    pmsSchedule.run();
  }

  if (isOneIndoor()) {
    if (hasSensorSHT) {
      delay(100);
      tempHumSchedule.run();
    }
  }

  if (hasSensorSGP) {
    tvocSchedule.run();
  }

  /** Auto reset external watchdog timer on offline mode and
   * postDataToAirGradient disabled. */
  if (offlineMode || (localConfig.isPostDataToAirGradient() == false)) {
    wdgFeedSchedule.run();
  }

  /** Check for handle WiFi reconnect */
  updateWiFiConnect();

  /** factory reset handle */
  factoryConfigReset();

  /** Read PMS on loop */
  if (isOneIndoor()) {
    if (hasSensorPMS1) {
      ag->pms5003.handle();
    }
  } else {
    if (hasSensorPMS1) {
      ag->pms5003t_1.handle();
    }
    if (hasSensorPMS2) {
      ag->pms5003t_2.handle();
    }
  }

  if (localConfigUpdate) {
    localConfigUpdate = false;
    configUpdateHandle();
  }
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
  ag->ledBar.setColor(r, g, b);
}

static void ledTest() {
  displayShowText("LED Test", "running", ".....");
  setTestColor('r');
  ag->ledBar.show();
  delay(1000);
  setTestColor('g');
  ag->ledBar.show();
  delay(1000);
  setTestColor('b');
  ag->ledBar.show();
  delay(1000);
  setTestColor('w');
  ag->ledBar.show();
  delay(1000);
  setTestColor('n');
  ag->ledBar.show();
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

static void co2Update(void) {
  int value = ag->s8.getCo2();
  if (value >= 0) {
    co2Ppm = value;
    getCO2FailCount = 0;
    Serial.printf("CO2 (ppm): %d\r\n", co2Ppm);
  } else {
    getCO2FailCount++;
    Serial.printf("Get CO2 failed: %d\r\n", getCO2FailCount);
    if (getCO2FailCount >= 3) {
      co2Ppm = 1;
    }
  }
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
                       "\",airgradient_device_type=\"" + ag->getBoardName() +
                       "\",airgradient_library_version=\"" + ag->getVersion() +
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

  float _temp = -1001;
  float _hum = -1;
  int pm01 = -1;
  int pm25 = -1;
  int pm10 = -1;
  int pm03PCount = -1;
  if (hasSensorPMS1 && hasSensorPMS2) {
    _temp = (temp_1 + temp_2) / 2.0f;
    _hum = (hum_1 + hum_2) / 2.0f;
    pm01 = (pm01_1 + pm01_2) / 2;
    pm25 = (pm25_1 + pm25_2) / 2;
    pm10 = (pm10_1 + pm10_2) / 2;
    pm03PCount = (pm03PCount_1 + pm03PCount_2) / 2;
  } else {
    if (isOneIndoor()) {
      if (hasSensorSHT) {
        _temp = temp;
        _hum = hum;
      }
    } else {
      if (hasSensorPMS1) {
        _temp = temp_1;
        _hum = hum_1;
        pm01 = pm01_1;
        pm25 = pm25_1;
        pm10 = pm10_1;
        pm03PCount = pm03PCount_1;
      }
      if (hasSensorPMS2) {
        _temp = temp_2;
        _hum = hum_2;
        pm01 = pm01_2;
        pm25 = pm25_2;
        pm10 = pm10_2;
        pm03PCount = pm03PCount_2;
      }
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

  {
    if (_temp > -1001) {
      add_metric("temperature",
                 "The ambient temperature as measured by the AirGradient SHT "
                 "sensor, in degrees Celsius",
                 "gauge", "celsius");
      add_metric_point("", String(_temp));
    }
    if (_hum >= 0) {
      add_metric(
          "humidity",
          "The relative humidity as measured by the AirGradient SHT sensor",
          "gauge", "percent");
      add_metric_point("", String(_hum));
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
    Serial.println("Init mDNS failed");
    return;
  }

  webServer.on("/measures/current", HTTP_GET, webServerMeasureCurrentGet);
  // Make it possible to query this device from Prometheus/OpenMetrics.
  webServer.on("/metrics", HTTP_GET, webServerMetricsGet);
  webServer.on("/config", HTTP_GET, localConfigGet);
  webServer.on("/config", HTTP_PUT, localConfigPut);
  webServer.begin();
  MDNS.addService("_airgradient", "_tcp", 80);
  MDNS.addServiceTxt("_airgradient", "_tcp", "model", getFirmwareModeName());
  MDNS.addServiceTxt("_airgradient", "_tcp", "serialno", getDevId());
  MDNS.addServiceTxt("_airgradient", "_tcp", "fw_ver", ag->getVersion());
  MDNS.addServiceTxt("_airgradient", "_tcp", "vendor", "AirGradient");

  if (xTaskCreate(webServerHandler, "webserver", 1024 * 4, NULL, 5, NULL) !=
      pdTRUE) {
    Serial.println("Create task handle webserver failed");
  }
  Serial.printf("Webserver init: %s.local\r\n", host.c_str());
}

static void localConfigGet() {
  webServer.send(200, "application/json", localConfig.toString());
}
static void localConfigPut() {
  String data = webServer.arg(0);
  String response = "Failure";
  if (localConfig.parse(data, true)) {
    localConfigUpdate = true;
    response = "Success";
  } else {
    Serial.println("PUT data invalid");
  }
  webServer.send(200, "text/plain", response);
}

static String getServerSyncData(bool localServer) {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  if (localServer) {
    root["serialno"] = getDevId();
  }
  if (hasSensorS8) {
    if (co2Ppm >= 0) {
      root["rco2"] = co2Ppm;
    }
  }

  if (isOneIndoor()) {
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
        if (localServer) {
          root["pm003Count"] = pm03PCount_1;
        } else {
          root["pm003_count"] = pm03PCount_1;
        }
      }
    }

    if (hasSensorSHT) {
      if (temp > -1001) {
        root["atmp"] = ag->round2(temp);
      }
      if (hum >= 0) {
        root["rhum"] = hum;
      }
    }

  } else {
    if (hasSensorPMS1 && hasSensorPMS2) {
      root["pm01"] = ag->round2((pm01_1 + pm01_2) / 2.0);
      root["pm02"] = ag->round2((pm25_1 + pm25_2) / 2.0);
      root["pm10"] = ag->round2((pm10_1 + pm10_2) / 2.0);
      if (localServer) {
        root["pm003Count"] = ag->round2((pm03PCount_1 + pm03PCount_2) / 2.0);
      } else {
        root["pm003_count"] = ag->round2((pm03PCount_1 + pm03PCount_2) / 2.0);
      }
      root["atmp"] = ag->round2((temp_1 + temp_2) / 2.0);
      root["rhum"] = ag->round2((hum_1 + hum_2) / 2.0);
    }

    if (fwMode == FW_MDOE_O_1PS || fwMode == FW_MODE_O_1PST) {
      if (hasSensorPMS1) {
        root["pm01"] = pm01_1;
        root["pm02"] = pm25_1;
        root["pm10"] = pm10_1;
        if (localServer) {
          root["pm003Count"] = pm03PCount_1;
        } else {
          root["pm003_count"] = pm03PCount_1;
        }
        root["atmp"] = ag->round2(temp_1);
        root["rhum"] = hum_1;
      }
      if (hasSensorPMS2) {
        root["pm01"] = pm01_2;
        root["pm02"] = pm25_2;
        root["pm10"] = pm10_2;
        if (localServer) {
          root["pm003Count"] = pm03PCount_2;
        } else {
          root["pm003_count"] = pm03PCount_2;
        }
        root["atmp"] = ag->round2(temp_2);
        root["rhum"] = hum_2;
      }
    } else {
      if (hasSensorPMS1) {
        root["channels"]["1"]["pm01"] = pm01_1;
        root["channels"]["1"]["pm02"] = pm25_1;
        root["channels"]["1"]["pm10"] = pm10_1;
        if (localServer) {
          root["channels"]["1"]["pm003Count"] = pm03PCount_1;
        } else {
          root["channels"]["1"]["pm003_count"] = pm03PCount_1;
        }
        root["channels"]["1"]["atmp"] = ag->round2(temp_1);
        root["channels"]["1"]["rhum"] = hum_1;
      }
      if (hasSensorPMS2) {
        root["channels"]["2"]["pm01"] = pm01_2;
        root["channels"]["2"]["pm02"] = pm25_2;
        root["channels"]["2"]["pm10"] = pm10_2;
        if (localServer) {
          root["channels"]["2"]["pm003Count"] = pm03PCount_2;
        } else {
          root["channels"]["2"]["pm003_count"] = pm03PCount_2;
        }
        root["channels"]["2"]["atmp"] = ag->round2(temp_2);
        root["channels"]["2"]["rhum"] = hum_2;
      }
    }
  }

  if (hasSensorSGP) {
    if (tvocIndex >= 0) {
      if (localServer) {
        root["tvocIndex"] = tvocIndex;
      } else {
        root["tvoc_index"] = tvocIndex;
      }
    }
    if (tvocRawIndex >= 0) {
      root["tvoc_raw"] = tvocRawIndex;
    }
    if (noxIndex >= 0) {
      if (localServer) {
        root["noxIndex"] = noxIndex;
      } else {
        root["nox_index"] = noxIndex;
      }
    }
    if (noxRawIndex >= 0) {
      root["nox_raw"] = noxRawIndex;
    }
  }
  root["boot"] = bootCount;

  if (localServer) {
    root["ledMode"] = localConfig.getLedBarModeName();
    root["firmwareVersion"] = ag->getVersion();
    root["fwMode"] = getFirmwareModeName();
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
              Serial.println("MQTT sync success");
            } else {
              Serial.println("MQTT sync failure");
            }
          }
        }
      },
      "mqtt-task", 1024 * 2, NULL, 6, &mqttTask);

  if (mqttTask == NULL) {
    Serial.println("Creat mqttTask failed");
  }
}

static void factoryConfigReset(void) {
  if (ag->button.getState() == ag->button.BUTTON_PRESSED) {
    if (factoryBtnPressTime == 0) {
      factoryBtnPressTime = millis();
    } else {
      uint32_t ms = (uint32_t)(millis() - factoryBtnPressTime);
      if (ms >= 2000) {
        // Show display message: For factory keep for x seconds
        // Count display.
        if (isOneIndoor()) {
          displayShowText("Factory reset", "keep pressed", "for 8 sec");
        } else {
          Serial.println("Factory reset, keep pressed for 8 sec");
        }

        int count = 7;
        while (ag->button.getState() == ag->button.BUTTON_PRESSED) {
          delay(1000);
          if (isOneIndoor()) {

            displayShowText("Factory reset", "keep pressed",
                            "for " + String(count) + " sec");
          } else {
            Serial.printf("Factory reset, keep pressed for %d sec\r\n", count);
          }
          count--;
          // ms = (uint32_t)(millis() - factoryBtnPressTime);
          if (count == 0) {
            /** Stop MQTT task first */
            if (mqttTask) {
              vTaskDelete(mqttTask);
              mqttTask = NULL;
            }

            /** Disconnect WIFI */
            WiFi.disconnect();
            wifiManager.resetSettings();

            /** Reset local config */
            localConfig.reset();

            if (isOneIndoor()) {
              displayShowText("Factory reset", "successful", "");
            } else {
              Serial.println("Factory reset successful");
            }
            delay(3000);
            ESP.restart();
          }
        }

        /** Show current content cause reset ignore */
        factoryBtnPressTime = 0;
        if (isOneIndoor()) {
          appDispHandler();
        }
      }
    }
  } else {
    if (factoryBtnPressTime != 0) {
      if (isOneIndoor()) {
        /** Restore last display content */
        appDispHandler();
      }
    }
    factoryBtnPressTime = 0;
  }
}

static void wdgFeedUpdate(void) {
  ag->watchdog.reset();
  Serial.println();
  Serial.println("External watchdog feed");
  Serial.println();
}

static void sendPing() {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  root["boot"] = bootCount;

  /** Change disp and led state */
  if (isOneIndoor()) {
    dispSmHandler(APP_SM_WIFI_OK_SERVER_CONNECTING);
  }
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
    if (isOneIndoor()) {
      dispSmHandler(APP_SM_WIFI_OK_SERVER_CONNECTED);
    }
    ledSmHandler(APP_SM_WIFI_OK_SERVER_CONNECTED);
  } else {
    if (isOneIndoor()) {
      dispSmHandler(APP_SM_WIFI_OK_SERVER_CONNECT_FAILED);
    }
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
      if (localConfig.isTemperatureUnitInF()) {
        if (temp > -1001) {
          float tempF = (temp * 9 / 5) + 32;
          sprintf(strBuf, "%.1f°F", tempF);
        } else {
          sprintf(strBuf, "-°F");
        }
        u8g2.drawUTF8(1, 10, strBuf);
      } else {
        if (temp > -1001) {
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
      Serial.println("Display show error: " + err);
      int strWidth = u8g2.getStrWidth(err.c_str());
      u8g2.drawStr((126 - strWidth) / 2, 10, err.c_str());

      if (err == "WiFi N/A") {
        u8g2.setFont(u8g2_font_t0_12_tf);
        if (localConfig.isTemperatureUnitInF()) {
          if (temp > -1001) {
            float tempF = (temp * 9 / 5) + 32;
            sprintf(strBuf, "%.1f", tempF);
          } else {
            sprintf(strBuf, "-");
          }
          u8g2.drawUTF8(1, 10, strBuf);
        } else {
          if (temp > -1001) {
            sprintf(strBuf, "%.1f", temp);
          } else {
            sprintf(strBuf, "-");
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
      }
    }

    /** Draw horizontal line */
    u8g2.drawLine(1, 13, 128, 13);

    /** Show CO2 label */
    u8g2.setFont(u8g2_font_t0_12_tf);
    u8g2.drawUTF8(1, 27, "CO2");

    /** Show CO2 value */
    u8g2.setFont(u8g2_font_t0_22b_tf);
    if (co2Ppm > 0) {
      int val = 9999;
      if (co2Ppm < 10000) {
        val = co2Ppm;
      }
      sprintf(strBuf, "%d", val);
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
    if (localConfig.isPmStandardInUSAQI()) {
      if (pm25_1 >= 0) {
        sprintf(strBuf, "%d", ag->pms5003.convertPm25ToUsAqi(pm25_1));
      } else {
        sprintf(strBuf, "%s", "-");
      }
      u8g2.drawStr(48, 48, strBuf);
      u8g2.setFont(u8g2_font_t0_12_tf);
      u8g2.drawUTF8(48, 61, "AQI");
    } else {
      if (pm25_1 >= 0) {
        sprintf(strBuf, "%d", pm25_1);
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
static void resetWatchdog() { ag->watchdog.reset(); }

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
    if (isOneIndoor()) {
      dispSmState = APP_SM_WIFI_MANAGER_MODE;
    }
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
    if (isOneIndoor()) {
      dispSmState = APP_SM_WIFI_MANAGER_STA_CONNECTING;
    }
    ledSmState = APP_SM_WIFI_MANAGER_STA_CONNECTING;
  });

  if (isOneIndoor()) {
    displayShowText("Connecting to", "WiFi", "...");
  } else {
    Serial.println("Connecting to WiFi...");
  }
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
      if (isOneIndoor()) {
        if (ms >= 1000) {
          dispPeriod = millis();
          dispSmHandler(dispSmState);
        } else {
          if (dispSmStateOld != dispSmState) {
            dispSmStateOld = dispSmState;
            dispSmHandler(dispSmState);
          }
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
          ledSmHandler(APP_SM_WIFI_MANAGER_PORTAL_ACTIVE);
        } else {
          ledCount = LED_BAR_COUNT_INIT_VALUE;
          ledSmHandler(APP_SM_WIFI_MANAGER_MODE);
          if (isOneIndoor()) {
            dispSmHandler(APP_SM_WIFI_MANAGER_MODE);
          }
        }
      }
    }
  }

  /** Show display wifi connect result failed */
  if (WiFi.isConnected() == false) {
    ledSmHandler(APP_SM_WIFI_MANAGER_CONNECT_FAILED);
    if (isOneIndoor()) {
      dispSmHandler(APP_SM_WIFI_MANAGER_CONNECT_FAILED);
    }
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
  if (co2Value <= 400) {
    /** G; 1 */
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
  } else if (co2Value <= 700) {
    /** GG; 2 */
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
  } else if (co2Value <= 1000) {
    /** YYY; 3 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
  } else if (co2Value <= 1333) {
    /** YYYY; 4 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 4);
  } else if (co2Value <= 1666) {
    /** YYYYY; 5 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 5);
  } else if (co2Value <= 2000) {
    /** RRRRRR; 6 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
  } else if (co2Value <= 2666) {
    /** RRRRRRR; 7 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
  } else if (co2Value <= 3333) {
    /** RRRRRRRR; 8 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
  } else if (co2Value <= 4000) {
    /** RRRRRRRRR; 9 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 9);
  } else { /** > 4000 */
    /* PRPRPRPRP; 9 */
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 9);
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
  int ledNum = ag->ledBar.getNumberOfLeds() - 1;
  ag->ledBar.setColor(r, g, b, ledNum);

  ledNum = ag->ledBar.getNumberOfLeds() - 2;
  ag->ledBar.setColor(r, g, b, ledNum);
}

void dispSensorNotFound(String ss) {
  displayShowText("Sensor init", "Error:", ss + " not found");
  delay(2000);
}

static void oneIndoorInit(void) {
  hasSensorPMS2 = false;

  /** Display init */
  u8g2.begin();

  /** Show boot display */
  Serial.println("Firmware Version: " + ag->getVersion());
  displayShowText("AirGradient ONE", "FW Version: ", ag->getVersion());
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

  ag->ledBar.begin();
  ag->button.begin();
  ag->watchdog.begin();

  /** Init sensor SGP41 */
  if (ag->sgp41.begin(Wire) == false) {
    Serial.println("SGP41 sensor not found");
    hasSensorSGP = false;
    dispSensorNotFound("SGP41");
  }

  /** INit SHT */
  if (ag->sht.begin(Wire) == false) {
    Serial.println("SHTx sensor not found");
    hasSensorSHT = false;
    dispSensorNotFound("SHT");
  }

  /** Init S8 CO2 sensor */
  if (ag->s8.begin(Serial1) == false) {
    // failedHandler("Init SenseAirS8 failed");
    Serial.println("CO2 S8 sensor not found");
    hasSensorS8 = false;
    dispSensorNotFound("S8");
  }

  /** Init PMS5003 */
  if (ag->pms5003.begin(Serial0) == false) {
    Serial.println("PMS sensor not found");
    hasSensorPMS1 = false;

    dispSensorNotFound("PMS");
  }

  /** Run LED test on start up */
  displayShowText("Press now for", "LED test &", "offline mode");
  ledBarButtonTest = false;
  uint32_t stime = millis();
  while (true) {
    if (ag->button.getState() == ag->button.BUTTON_PRESSED) {
      ledBarButtonTest = true;
      break;
    }
    delay(1);
    uint32_t ms = (uint32_t)(millis() - stime);
    if (ms >= 3000) {
      break;
    }
  }
}
static void openAirInit(void) {
  hasSensorSHT = false;

  fwMode = FW_MODE_O_1PST;
  Serial.println("Firmware Version: " + ag->getVersion());

  ag->watchdog.begin();
  ag->button.begin();
  ag->statusLed.begin();

  /** detect sensor: PMS5003, PMS5003T, SGP41 and S8 */
  /**
   * Serial1 and Serial0 is use for connect S8 and PM sensor or both PM
   */
  bool serial1Available = true;
  bool serial0Available = true;

  if (ag->s8.begin(Serial1) == false) {
    Serial1.end();
    delay(200);
    Serial.println("Can not detect S8 on Serial1, try on Serial0");
    /** Check on other port */
    if (ag->s8.begin(Serial0) == false) {
      hasSensorS8 = false;

      Serial.println("CO2 S8 sensor not found");
      Serial.println("Can not detect S8 run mode 'PPT'");
      fwMode = FW_MODE_O_1PPT;

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

  if (ag->sgp41.begin(Wire) == false) {
    hasSensorSGP = false;
    Serial.println("SGP sensor not found");

    if (hasSensorS8 == false) {
      Serial.println("Can not detect SGP run mode 'O-1PP'");
      fwMode = FW_MODE_O_1PP;
    } else {
      Serial.println("Can not detect SGP run mode 'O-1PS'");
      fwMode = FW_MDOE_O_1PS;
    }
  }

  /** Try to find the PMS on other difference port with S8 */
  if (fwMode == FW_MODE_O_1PST) {
    bool pmInitSuccess = false;
    if (serial0Available) {
      if (ag->pms5003t_1.begin(Serial0) == false) {
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
        if (ag->pms5003t_1.begin(Serial1) == false) {
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
    if (ag->pms5003t_1.begin(Serial0) == false) {
      hasSensorPMS1 = false;
      Serial.println("PMS1 sensor not found");
    } else {
      Serial.println("Found PMS 1 on Serial0");
    }
    if (ag->pms5003t_2.begin(Serial1) == false) {
      hasSensorPMS2 = false;
      Serial.println("PMS2 sensor not found");
    } else {
      Serial.println("Found PMS 2 on Serial1");
    }
  }

  /** update the PMS poll period base on fw mode and sensor available */
  if (fwMode != FW_MODE_O_1PST) {
    if (hasSensorPMS1 && hasSensorPMS2) {
      pmsSchedule.setPeriod(2000);
    }
  }

  Serial.printf("Firmware Mode: %s\r\n", getFirmwareModeName());
}

static String getFirmwareModeName() {
  switch (fwMode) {
  case FW_MODE_I_9PSL:
    return "I-9PSL";
  case FW_MODE_O_1PP:
    return "O-1PP";
  case FW_MODE_O_1PPT:
    return "O-1PPT";
  case FW_MODE_O_1PST:
    return "O-1PST";
  case FW_MDOE_O_1PS:
    return "0-1PS";
  default:
    break;
  }
  return "UNKNOWN";
}

static bool isOneIndoor(void) {
  return ag->getBoardType() == BoardType::ONE_INDOOR;
}

/**
 * @brief Initialize board
 */
static void boardInit(void) {
  if (isOneIndoor()) {
    oneIndoorInit();
  } else {
    openAirInit();
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
static void updateServerConfiguration(void) {
  if (agServer.fetchServerConfiguration(getDevId())) {
    configUpdateHandle();
  }
}

static void configUpdateHandle() {
  if (localConfig.isCo2CalibrationRequested()) {
    if (hasSensorS8) {
      co2Calibration();
    } else {
      Serial.println("CO2 S8 not available, calibration ignored");
    }
  }

  // Update LED bar
  if (isOneIndoor()) {
    ag->ledBar.setEnable(localConfig.getLedBarMode() != UseLedBarOff);
  }

  if (localConfig.getCO2CalirationAbcDays() > 0) {
    if (hasSensorS8) {
      int newHour = localConfig.getCO2CalirationAbcDays() * 24;
      Serial.printf("Requested abcDays setting: %d days (%d hours)\r\n",
                    localConfig.getCO2CalirationAbcDays(), newHour);
      int curHour = ag->s8.getAbcPeriod();
      Serial.printf("Current S8 abcDays setting: %d (hours)\r\n", curHour);
      if (curHour == newHour) {
        Serial.println("'abcDays' unchanged");
      } else {
        if (ag->s8.setAbcPeriod(localConfig.getCO2CalirationAbcDays() * 24) ==
            false) {
          Serial.println("Set S8 abcDays period failed");
        } else {
          Serial.println("Set S8 abcDays period success");
        }
      }
    } else {
      Serial.println("CO2 S8 not available, set 'abcDays' ignored");
    }
  }

  if (localConfig.isLedBarTestRequested()) {
    if (localConfig.getCountry() == "TH") {
      ledTest2Min();
    } else {
      ledTest();
    }
  }

  String mqttUri = localConfig.getMqttBrokerUri();
  if (mqttUri != agMqtt.getUri()) {
    agMqtt.end();

    if (mqttTask != NULL) {
      vTaskDelete(mqttTask);
      mqttTask = NULL;
    }
    if (mqttUri.length() > 0) {
      if (agMqtt.begin(mqttUri)) {
        Serial.println("Connect to MQTT broker successful");
        createMqttTask();
      } else {
        Serial.println("Connect to MQTT broker failed");
      }
    }
  }
}

/**
 * @brief Calibration CO2 sensor, it's base calibration, after calib complete
 * the value will be start at 400 if do calib on clean environment
 */
static void co2Calibration(void) {
  Serial.println("co2Calibration: Start");
  /** Count down for co2CalibCountdown secs */
  for (int i = 0; i < SENSOR_CO2_CALIB_COUNTDOWN_MAX; i++) {
    if (isOneIndoor()) {
      displayShowText(
          "Start CO2 calib",
          "after " + String(SENSOR_CO2_CALIB_COUNTDOWN_MAX - i) + " sec", "");
    } else {
      Serial.printf("Start CO2 calib after %d sec\r\n",
                    SENSOR_CO2_CALIB_COUNTDOWN_MAX - i);
    }
    delay(1000);
  }

  if (ag->s8.setBaselineCalibration()) {
    if (isOneIndoor()) {
      displayShowText("Calibration", "success", "");
    } else {
      Serial.println("Calibration success");
    }
    delay(1000);
    if (isOneIndoor()) {
      displayShowText("Wait for", "calib finish", "...");
    } else {
      Serial.println("Wait for calibration finish...");
    }
    int count = 0;
    while (ag->s8.isBaseLineCalibrationDone() == false) {
      delay(1000);
      count++;
    }
    if (isOneIndoor()) {
      displayShowText("Calib finish", "after " + String(count), "sec");
    } else {
      Serial.printf("Calibration finish after %d sec\r\n", count);
    }
    delay(2000);
  } else {
    if (isOneIndoor()) {
      displayShowText("Calibration", "failure!!!", "");
    } else {
      Serial.println("Calibration failure!!!");
    }
    delay(2000);
  }

  /** Update display */
  if (isOneIndoor()) {
    appDispHandler();
  }
}

/**
 * @brief Set LED color for special PMS value
 *
 * @param pm25Value PMS2.5 value
 */
static void setRGBledPMcolor(int pm25Value) {
  if (pm25Value <= 5) {
    /** G; 1 */
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
  } else if (pm25Value <= 10) {
    /** GG; 2 */
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
  } else if (pm25Value <= 20) {
    /** YYY; 3 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
  } else if (pm25Value <= 35) {
    /** YYYY; 4 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 4);
  } else if (pm25Value <= 45) {
    /** YYYYY; 5 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 5);
  } else if (pm25Value <= 55) {
    /** RRRRRR; 6 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
  } else if (pm25Value <= 65) {
    /** RRRRRRR; 7 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
  } else if (pm25Value <= 150) {
    /** RRRRRRRR; 8 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
  } else if (pm25Value <= 250) {
    /** RRRRRRRRR; 9 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 9);
  } else { /** > 250 */
    /* PRPRPRPRP; 9 */
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 9);
  }
}

static void singleLedAnimation(uint8_t r, uint8_t g, uint8_t b) {
  if (ledCount < 0) {
    ledCount = 0;
    ag->ledBar.setColor(r, g, b, ledCount);
  } else {
    ledCount++;
    if (ledCount >= ag->ledBar.getNumberOfLeds()) {
      ledCount = 0;
    }
    ag->ledBar.setColor(r, g, b, ledCount);
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
  if (isOneIndoor()) {
    ag->ledBar.clear(); // Set all LED OFF
  }
  switch (sm) {
  case APP_SM_WIFI_MANAGER_MODE: {
    /** In WiFi Manager Mode */
    /** Turn LED OFF */
    /** Turn midle LED Color */
    if (isOneIndoor()) {
      ag->ledBar.setColor(0, 0, 255, ag->ledBar.getNumberOfLeds() / 2);
    } else {
      ag->statusLed.setToggle();
    }
    break;
  }
  case APP_SM_WIFI_MANAGER_PORTAL_ACTIVE: {
    /** WiFi Manager has connected to mobile phone */
    if (isOneIndoor()) {
      ag->ledBar.setColor(0, 0, 255);
    } else {
      ag->statusLed.setOn();
    }
    break;
  }
  case APP_SM_WIFI_MANAGER_STA_CONNECTING: {
    /** after SSID and PW entered and OK clicked, connection to WiFI network is
     * attempted */
    if (isOneIndoor()) {
      singleLedAnimation(255, 255, 255);
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case APP_SM_WIFI_MANAGER_STA_CONNECTED: {
    /** Connecting to WiFi worked */
    if (isOneIndoor()) {
      ag->ledBar.setColor(255, 255, 255);
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECTING: {
    /** once connected to WiFi an attempt to reach the server is performed */
    if (isOneIndoor()) {
      singleLedAnimation(0, 255, 0);
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECTED: {
    /** Server is reachable, all ﬁne */
    if (isOneIndoor()) {
      ag->ledBar.setColor(0, 255, 0);
    } else {
      ag->statusLed.setOff();

      /** two time slow blink, then off */
      for (int i = 0; i < 2; i++) {
        ledBlinkDelay(LED_SLOW_BLINK_DELAY);
      }

      ag->statusLed.setOff();
    }
    break;
  }
  case APP_SM_WIFI_MANAGER_CONNECT_FAILED: {
    /** Cannot connect to WiFi (e.g. wrong password, WPA Enterprise etc.) */
    if (isOneIndoor()) {
      ag->ledBar.setColor(255, 0, 0);
    } else {
      ag->statusLed.setOff();

      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
          ledBlinkDelay(LED_FAST_BLINK_DELAY);
        }
        delay(2000);
      }
      ag->statusLed.setOff();
    }
    break;
  }
  case APP_SM_WIFI_OK_SERVER_CONNECT_FAILED: {
    /** Connected to WiFi but server not reachable, e.g. firewall block/
     * whitelisting needed etc. */
    if (isOneIndoor()) {
      ag->ledBar.setColor(233, 183, 54); /** orange */
    } else {
      ag->statusLed.setOff();
      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 4; i++) {
          ledBlinkDelay(LED_FAST_BLINK_DELAY);
        }
        delay(2000);
      }
      ag->statusLed.setOff();
    }
    break;
  }
  case APP_SM_WIFI_OK_SERVER_OK_SENSOR_CONFIG_FAILED: {
    /** Server reachable but sensor not configured correctly */
    if (isOneIndoor()) {
      ag->ledBar.setColor(139, 24, 248); /** violet */
    } else {
      ag->statusLed.setOff();
      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 5; i++) {
          ledBlinkDelay(LED_FAST_BLINK_DELAY);
        }
        delay(2000);
      }
      ag->statusLed.setOff();
    }
    break;
  }
  case APP_SM_WIFI_LOST: {
    /** Connection to WiFi network failed credentials incorrect encryption not
     * supported etc. */
    if (isOneIndoor()) {
      /** WIFI failed status LED color */
      ag->ledBar.setColor(255, 0, 0, 0);
      /** Show CO2 or PM color status */
      sensorLedColorHandler();
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case APP_SM_SERVER_LOST: {
    /** Connected to WiFi network but the server cannot be reached through the
     * internet, e.g. blocked by firewall */
    if (isOneIndoor()) {
      ag->ledBar.setColor(233, 183, 54, 0);

      /** Show CO2 or PM color status */
      sensorLedColorHandler();
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case APP_SM_SENSOR_CONFIG_FAILED: {
    /** Server is reachable but there is some conﬁguration issue to be fixed on
     * the server side */
    if (isOneIndoor()) {
      ag->ledBar.setColor(139, 24, 248, 0);

      /** Show CO2 or PM color status */
      sensorLedColorHandler();
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case APP_SM_NORMAL: {
    if (isOneIndoor()) {
      sensorLedColorHandler();
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  default:
    break;
  }

  if (isOneIndoor()) {
    ag->ledBar.show();
  }
}

static void ledBlinkDelay(uint32_t tdelay) {
  ag->statusLed.setOn();
  delay(tdelay);
  ag->statusLed.setOff();
  delay(tdelay);
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
  case APP_SM_WIFI_MANAGER_PORTAL_ACTIVE: {
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
  case APP_SM_WIFI_OK_SERVER_CONNECTED: {
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
    uint32_t ms = (uint32_t)(millis() - addToDashboardTime);
    if (ms >= 5000) {
      addToDashboardTime = millis();
      if (isAddToDashboard) {
        displayShowDashboard("Add to Dashboard");
      } else {
        displayShowDashboard(getDevId());
      }
      isAddToDashboard = !isAddToDashboard;
    }
    break;
  }
  case APP_SM_NORMAL: {
    displayShowDashboard("");
  }
  default:
    break;
  }
}

/**
 * @brief Handle change LED color base on sensor value of CO2 and PMS
 */
static void sensorLedColorHandler(void) {
  switch (localConfig.getLedBarMode()) {
  case UseLedBarCO2:
    setRGBledCO2color(co2Ppm);
    break;
  case UseLedBarPM:
    setRGBledPMcolor(pm25_1);
    break;
  case UseLedBarOff:
    ag->ledBar.clear();
    break;
  default:
    ag->ledBar.clear();
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
static void displayAndLedBarUpdate(void) {
  if (isOneIndoor()) {
    if (factoryBtnPressTime == 0) {
      appDispHandler();
    }
  }
  appLedHandler();
}

/**
 * @brief Update tvocIndexindex
 *
 */
static void tvocUpdate(void) {
  tvocIndex = ag->sgp41.getTvocIndex();
  tvocRawIndex = ag->sgp41.getTvocRaw();
  noxIndex = ag->sgp41.getNoxIndex();
  noxRawIndex = ag->sgp41.getNoxRaw();

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
  if (isOneIndoor()) {
    if (ag->pms5003.isFailed() == false) {
      pm01_1 = ag->pms5003.getPm01Ae();
      pm25_1 = ag->pms5003.getPm25Ae();
      pm10_1 = ag->pms5003.getPm10Ae();
      pm03PCount_1 = ag->pms5003.getPm03ParticleCount();

      Serial.println();
      Serial.printf("PM1 ug/m3: %d\r\n", pm01_1);
      Serial.printf("PM2.5 ug/m3: %d\r\n", pm25_1);
      Serial.printf("PM10 ug/m3: %d\r\n", pm10_1);
      Serial.printf("PM0.3 Count: %d\r\n", pm03PCount_1);
      pmFailCount = 0;
    } else {
      pmFailCount++;
      Serial.printf("PMS read failed: %d\r\n", pmFailCount);
      if (pmFailCount >= 3) {
        pm01_1 = -1;
        pm25_1 = -1;
        pm10_1 = -1;
        pm03PCount_1 = -1;
      }
    }
  } else {
    bool pmsResult_1 = false;
    bool pmsResult_2 = false;
    if (hasSensorPMS1 && (ag->pms5003t_1.isFailed() == false)) {
      pm01_1 = ag->pms5003t_1.getPm01Ae();
      pm25_1 = ag->pms5003t_1.getPm25Ae();
      pm10_1 = ag->pms5003t_1.getPm10Ae();
      pm03PCount_1 = ag->pms5003t_1.getPm03ParticleCount();
      temp_1 = ag->pms5003t_1.getTemperature();
      hum_1 = ag->pms5003t_1.getRelativeHumidity();

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

    if (hasSensorPMS2 && (ag->pms5003t_2.isFailed() == false)) {
      pm01_2 = ag->pms5003t_2.getPm01Ae();
      pm25_2 = ag->pms5003t_2.getPm25Ae();
      pm10_2 = ag->pms5003t_2.getPm10Ae();
      pm03PCount_2 = ag->pms5003t_2.getPm03ParticleCount();
      temp_2 = ag->pms5003t_2.getTemperature();
      hum_2 = ag->pms5003t_2.getRelativeHumidity();

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
      ag->sgp41.setCompensationTemperatureHumidity(temp, hum);
    }
  }
}

/**
 * @brief Send data to server
 *
 */
static void sendDataToServer(void) {
  String syncData = getServerSyncData(false);
  if (agServer.postToServer(getDevId(), syncData)) {
    resetWatchdog();
  }

  bootCount++;
}

/**
 * @brief Update temperature and humidity value
 */
static void tempHumUpdate(void) {
  if (ag->sht.measure()) {

    temp = ag->sht.getTemperature();
    hum = ag->sht.getRelativeHumidity();

    Serial.printf("Temperature in C: %0.2f\r\n", temp);
    Serial.printf("Relative Humidity: %d\r\n", hum);

    // Update compensation temperature and humidity for SGP41
    if (hasSensorSGP) {
      ag->sgp41.setCompensationTemperatureHumidity(temp, hum);
    }
  } else {
    Serial.println("SHT read failed");
  }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  AgMqtt *mqtt = (AgMqtt *)handler_args;

  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  esp_mqtt_client_handle_t client = event->client;
  int msg_id;
  switch ((esp_mqtt_event_id_t)event_id) {
  case MQTT_EVENT_CONNECTED:
    Serial.println("MQTT_EVENT_CONNECTED");
    // msg_id = esp_mqtt_client_subscribe(client, "helloworld", 0);
    // Serial.printf("sent subscribe successful, msg_id=%d\r\n", msg_id);
    mqtt->_connectionHandler(true);
    break;
  case MQTT_EVENT_DISCONNECTED:
    Serial.println("MQTT_EVENT_DISCONNECTED");
    mqtt->_connectionHandler(false);
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
