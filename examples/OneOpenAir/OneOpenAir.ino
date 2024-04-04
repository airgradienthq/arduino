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
"Arduino_JSON" by Arduino version 0.2.0

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

#include <HardwareSerial.h>
// #include <WiFiManager.h>

#include "AgApiClient.h"
#include "AgConfigure.h"
#include "AgSchedule.h"
#include "AgStateMachine.h"
#include "AgWiFiConnector.h"
#include "EEPROM.h"
#include "MqttClient.h"
#include <AirGradient.h>
#include <Arduino_JSON.h>
#include <ESPmDNS.h>
#include <WebServer.h>

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

/** Default WiFi AP password */
#define WIFI_HOTSPOT_PASSWORD_DEFAULT "cleanair"

/** I2C define */
#define I2C_SDA_PIN 7
#define I2C_SCL_PIN 6
#define OLED_I2C_ADDR 0x3C

static MqttClient mqttClient(Serial);
static TaskHandle_t mqttTask = NULL;
static AgConfigure localConfig(Serial);
static AgApiClient apiClient(Serial, localConfig);
static AgValue agValue;
static AirGradient *ag;
static AgOledDisplay disp(localConfig, agValue, Serial);
static AgStateMachine sm(disp, Serial, agValue, localConfig);
static AgWiFiConnector wifiConnector(disp, Serial, sm);
static WebServer webServer;

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
static AgFirmwareMode fwMode = FW_MODE_I_9PSL;

static int bootCount;

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
static void appLedHandler(void);
static void appDispHandler(void);
static void updateWiFiConnect(void);
static void displayAndLedUpdate(void);
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

AgSchedule dispLedSchedule(DISP_UPDATE_INTERVAL, displayAndLedUpdate);
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

  /** Init sensor */
  boardInit();

  disp.setAirGradient(ag);
  sm.setAirGradient(ag);
  wifiConnector.setAirGradient(ag);

  /** Connecting wifi */
  bool connectWifi = false;
  if (ag->isOneIndoor()) {
    if (ledBarButtonTest) {
      ledBarTest();
    } else {
      /** Check LED mode to disabled LED */
      if (localConfig.getLedBarMode() == LedBarModeOff) {
        ag->ledBar.setEnable(false);
      }
      connectWifi = true;
    }
  } else {
    connectWifi = true;
  }

  if (connectWifi) {
    /** Init AirGradient server */
    apiClient.begin();
    apiClient.setAirGradient(ag);

    if (wifiConnector.connect()) {
      Serial.println("Connect to wifi failed");

      /**
       * Send first data to ping server and get server configuration
       */
      if (wifiConnector.isConnected()) {
        webServerInit();

        /** MQTT init */
        if (localConfig.getMqttBrokerUri().isEmpty() == false) {
          if (mqttClient.begin(localConfig.getMqttBrokerUri())) {
            createMqttTask();
            Serial.println("MQTT client init success");
          } else {
            Serial.println("MQTT client init failure");
          }
        }

        sendPing();
        Serial.println(F("WiFi connected!"));
        Serial.println("IP address: ");
        Serial.println(wifiConnector.localIpStr());

        /** Get first connected to wifi */
        apiClient.fetchServerConfiguration();
        if (apiClient.isFetchConfigureFailed()) {
          if (ag->isOneIndoor()) {
            sm.displayHandle(AgStateMachineWiFiOkServerOkSensorConfigFailed);
          }
          sm.ledHandle(AgStateMachineWiFiOkServerOkSensorConfigFailed);
          delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
        } else {
          ag->ledBar.setEnable(localConfig.getLedBarMode() != LedBarModeOff);
        }
      } else {
        offlineMode = true;
      }
    }
  }

  /** Show display Warning up */
  if (ag->isOneIndoor()) {
    disp.setText("Warming Up", "Serial Number:", ag->deviceId().c_str());
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
  }

  appLedHandler();
  if (ag->isOneIndoor()) {
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

  if (ag->isOneIndoor()) {
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
  wifiConnector.handle();

  /** factory reset handle */
  factoryConfigReset();

  /** Read PMS on loop */
  if (ag->isOneIndoor()) {
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

  /** check that local configura changed then do some action */
  if (localConfigUpdate) {
    localConfigUpdate = false;
    configUpdateHandle();
  }
}

static void ledBarTestColor(char color) {
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

static void ledBarTest() {
  disp.setText("LED Test", "running", ".....");
  ledBarTestColor('r');
  ag->ledBar.show();
  delay(1000);
  ledBarTestColor('g');
  ag->ledBar.show();
  delay(1000);
  ledBarTestColor('b');
  ag->ledBar.show();
  delay(1000);
  ledBarTestColor('w');
  ag->ledBar.show();
  delay(1000);
  ledBarTestColor('n');
  ag->ledBar.show();
  delay(1000);
}

static void ledBarTest2Min(void) {
  uint32_t tstart = millis();

  Serial.println("Start run LED test for 2 min");
  while (1) {
    ledBarTest();
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
    agValue.CO2 = value;
    getCO2FailCount = 0;
    Serial.printf("CO2 (ppm): %d\r\n", agValue.CO2);
  } else {
    getCO2FailCount++;
    Serial.printf("Get CO2 failed: %d\r\n", getCO2FailCount);
    if (getCO2FailCount >= 3) {
      agValue.CO2 = -1;
    }
  }
}

static void showNr(void) { Serial.println("Serial nr: " + ag->deviceId()); }

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
  add_metric_point("airgradient_serial_number=\"" + ag->deviceId() +
                       "\",airgradient_device_type=\"" + ag->getBoardName() +
                       "\",airgradient_library_version=\"" + ag->getVersion() +
                       "\"",
                   "1");

  add_metric("config_ok",
             "1 if the AirGradient device was able to successfully fetch its "
             "configuration from the server",
             "gauge");
  add_metric_point("", apiClient.isFetchConfigureFailed() ? "0" : "1");

  add_metric(
      "post_ok",
      "1 if the AirGradient device was able to successfully send to the server",
      "gauge");
  add_metric_point("", apiClient.isPostToServerFailed() ? "0" : "1");

  add_metric(
      "wifi_rssi",
      "WiFi signal strength from the AirGradient device perspective, in dBm",
      "gauge", "dbm");
  add_metric_point("", String(wifiConnector.RSSI()));

  if (hasSensorS8 && agValue.CO2 >= 0) {
    add_metric("co2",
               "Carbon dioxide concentration as measured by the AirGradient S8 "
               "sensor, in parts per million",
               "gauge", "ppm");
    add_metric_point("", String(agValue.CO2));
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
    if (ag->isOneIndoor()) {
      if (hasSensorSHT) {
        _temp = agValue.Temperature;
        _hum = agValue.Humidity;
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
    if (agValue.TVOC >= 0) {
      add_metric("tvoc_index",
                 "The processed Total Volatile Organic Compounds (TVOC) index "
                 "as measured by the AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(agValue.TVOC));
    }
    if (agValue.TVOCRaw >= 0) {
      add_metric("tvoc_raw",
                 "The raw input value to the Total Volatile Organic Compounds "
                 "(TVOC) index as measured by the AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(agValue.TVOCRaw));
    }
    if (agValue.NOx >= 0) {
      add_metric("nox_index",
                 "The processed Nitrous Oxide (NOx) index as measured by the "
                 "AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(agValue.NOx));
    }
    if (agValue.NOxRaw >= 0) {
      add_metric("nox_raw",
                 "The raw input value to the Nitrous Oxide (NOx) index as "
                 "measured by the AirGradient SGP sensor",
                 "gauge");
      add_metric_point("", String(agValue.NOxRaw));
    }
  }

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
  String host = "airgradient_" + ag->deviceId();
  if (!MDNS.begin(host.c_str())) {
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
  MDNS.addServiceTxt("_airgradient", "_tcp", "model",
                     AgFirmwareModeName(fwMode));
  MDNS.addServiceTxt("_airgradient", "_tcp", "serialno", ag->deviceId());
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
  String response = "";
  int statusCode = 400; // Status code for data invalid
  if (localConfig.parse(data, true)) {
    localConfigUpdate = true;
    statusCode = 200;
    response = "Success";
  } else {
    response = "Set for cloud configuration. Local configuration ignored";
  }
  webServer.send(statusCode, "text/plain", response);
}

static String getServerSyncData(bool localServer) {
  JSONVar root;
  root["wifi"] = wifiConnector.RSSI();
  if (localServer) {
    root["serialno"] = ag->deviceId();
  }
  if (hasSensorS8) {
    if (agValue.CO2 >= 0) {
      root["rco2"] = agValue.CO2;
    }
  }

  if (ag->isOneIndoor()) {
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
      if (agValue.Temperature > -1001) {
        root["atmp"] = ag->round2(agValue.Temperature);
      }
      if (agValue.Humidity >= 0) {
        root["rhum"] = agValue.Humidity;
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
    if (agValue.TVOC >= 0) {
      if (localServer) {
        root["tvocIndex"] = agValue.TVOC;
      } else {
        root["tvoc_index"] = agValue.TVOC;
      }
    }
    if (agValue.TVOCRaw >= 0) {
      root["tvoc_raw"] = agValue.TVOCRaw;
    }
    if (agValue.NOx >= 0) {
      if (localServer) {
        root["noxIndex"] = agValue.NOx;
      } else {
        root["nox_index"] = agValue.NOx;
      }
    }
    if (agValue.NOxRaw >= 0) {
      root["nox_raw"] = agValue.NOxRaw;
    }
  }
  root["boot"] = bootCount;

  if (localServer) {
    root["ledMode"] = localConfig.getLedBarModeName();
    root["firmwareVersion"] = ag->getVersion();
    root["fwMode"] = AgFirmwareModeName(fwMode);
  }

  return JSON.stringify(root);
}

static void createMqttTask(void) {
  if (mqttTask) {
    vTaskDelete(mqttTask);
    mqttTask = NULL;
    Serial.println("Delete old MQTT task");
  }

  Serial.println("Create new MQTT task");
  xTaskCreate(
      [](void *param) {
        for (;;) {
          delay(MQTT_SYNC_INTERVAL);

          /** Send data */
          if (mqttClient.isConnected()) {
            String payload = getServerSyncData(false);
            String topic = "airgradient/readings/" + ag->deviceId();

            if (mqttClient.publish(topic.c_str(), payload.c_str(),
                                   payload.length())) {
              Serial.println("MQTT sync success");
            } else {
              Serial.println("MQTT sync failure");
            }
          }
        }
      },
      "mqtt-task", 1024 * 4, NULL, 6, &mqttTask);

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
        if (ag->isOneIndoor()) {
          disp.setText("Factory reset", "keep pressed", "for 8 sec");
        } else {
          Serial.println("Factory reset, keep pressed for 8 sec");
        }

        int count = 7;
        while (ag->button.getState() == ag->button.BUTTON_PRESSED) {
          delay(1000);
          if (ag->isOneIndoor()) {

            String str = "for " + String(count) + " sec";
            disp.setText("Factory reset", "keep pressed", str.c_str());
          } else {
            Serial.printf("Factory reset, keep pressed for %d sec\r\n", count);
          }
          count--;
          if (count == 0) {
            /** Stop MQTT task first */
            if (mqttTask) {
              vTaskDelete(mqttTask);
              mqttTask = NULL;
            }

            /** Disconnect WIFI */
            wifiConnector.disconnect();
            wifiConnector.reset();

            /** Reset local config */
            localConfig.reset();

            if (ag->isOneIndoor()) {
              disp.setText("Factory reset", "successful", "");
            } else {
              Serial.println("Factory reset successful");
            }
            delay(3000);
            ESP.restart();
          }
        }

        /** Show current content cause reset ignore */
        factoryBtnPressTime = 0;
        if (ag->isOneIndoor()) {
          appDispHandler();
        }
      }
    }
  } else {
    if (factoryBtnPressTime != 0) {
      if (ag->isOneIndoor()) {
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
  root["wifi"] = wifiConnector.RSSI();
  root["boot"] = bootCount;

  /** Change disp and led state */
  if (ag->isOneIndoor()) {
    sm.displayHandle(AgStateMachineWiFiOkServerConnecting);
  }
  sm.ledHandle(AgStateMachineWiFiOkServerConnecting);

  /** Task handle led connecting animation */
  xTaskCreate(
      [](void *obj) {
        for (;;) {
          // ledSmHandler();
          sm.ledHandle();
          if (sm.getLedState() != AgStateMachineWiFiOkServerConnecting) {
            break;
          }
          delay(LED_BAR_ANIMATION_PERIOD);
        }
        vTaskDelete(NULL);
      },
      "task_led", 2048, NULL, 5, NULL);

  delay(1500);
  if (apiClient.postToServer(JSON.stringify(root))) {
    if (ag->isOneIndoor()) {
      sm.displayHandle(AgStateMachineWiFiOkServerConnected);
    }
    sm.ledHandle(AgStateMachineWiFiOkServerConnected);
  } else {
    if (ag->isOneIndoor()) {
      sm.displayHandle(AgStateMachineWiFiOkServerConnectFailed);
    }
    sm.ledHandle(AgStateMachineWiFiOkServerConnectFailed);
  }
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
  sm.ledHandle(AgStateMachineNormal);
}

/**
 * @brief Must reset each 5min to avoid ESP32 reset
 */
static void resetWatchdog() { ag->watchdog.reset(); }

void dispSensorNotFound(String ss) {
  ss = ss + " not found";
  disp.setText("Sensor init", "Error:", ss.c_str());
  delay(2000);
}

static void oneIndoorInit(void) {
  hasSensorPMS2 = false;

  /** Display init */
  disp.begin();

  /** Show boot display */
  Serial.println("Firmware Version: " + ag->getVersion());
  disp.setText("AirGradient ONE", "FW Version: ", ag->getVersion().c_str());
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
  disp.setText("Press now for", "LED test &", "offline mode");
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

  Serial.printf("Firmware Mode: %s\r\n", AgFirmwareModeName(fwMode));
}

/**
 * @brief Initialize board
 */
static void boardInit(void) {
  if (ag->isOneIndoor()) {
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
  if (apiClient.fetchServerConfiguration()) {
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
  if (ag->isOneIndoor()) {
    ag->ledBar.setEnable(localConfig.getLedBarMode() != LedBarModeOff);
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
      ledBarTest2Min();
    } else {
      ledBarTest();
    }
  }

  String mqttUri = localConfig.getMqttBrokerUri();
  if (mqttClient.isCurrentUri(mqttUri) == false) {
    mqttClient.end();

    if (mqttTask != NULL) {
      vTaskDelete(mqttTask);
      mqttTask = NULL;
    }
    if (mqttUri.length() > 0) {
      if (mqttClient.begin(mqttUri)) {
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
    if (ag->isOneIndoor()) {
      String str =
          "after " + String(SENSOR_CO2_CALIB_COUNTDOWN_MAX - i) + " sec";
      disp.setText("Start CO2 calib", str.c_str(), "");
    } else {
      Serial.printf("Start CO2 calib after %d sec\r\n",
                    SENSOR_CO2_CALIB_COUNTDOWN_MAX - i);
    }
    delay(1000);
  }

  if (ag->s8.setBaselineCalibration()) {
    if (ag->isOneIndoor()) {
      disp.setText("Calibration", "success", "");
    } else {
      Serial.println("Calibration success");
    }
    delay(1000);
    if (ag->isOneIndoor()) {
      disp.setText("Wait for", "calib finish", "...");
    } else {
      Serial.println("Wait for calibration finish...");
    }
    int count = 0;
    while (ag->s8.isBaseLineCalibrationDone() == false) {
      delay(1000);
      count++;
    }
    if (ag->isOneIndoor()) {
      String str = "after " + String(count);
      disp.setText("Calib finish", str.c_str(), "sec");
    } else {
      Serial.printf("Calibration finish after %d sec\r\n", count);
    }
    delay(2000);
  } else {
    if (ag->isOneIndoor()) {
      disp.setText("Calibration", "failure!!!", "");
    } else {
      Serial.println("Calibration failure!!!");
    }
    delay(2000);
  }

  /** Update display */
  if (ag->isOneIndoor()) {
    appDispHandler();
  }
}

/**
 * @brief APP LED color handler
 */
static void appLedHandler(void) {
  AgStateMachineState state = AgStateMachineNormal;
  if (wifiConnector.isConnected() == false) {
    state = AgStateMachineWiFiLost;
  } else if (apiClient.isFetchConfigureFailed()) {
    state = AgStateMachineSensorConfigFailed;
  } else if (apiClient.isPostToServerFailed()) {
    state = AgStateMachineServerLost;
  }

  sm.ledHandle(state);
}

/**
 * @brief APP display content handler
 */
static void appDispHandler(void) {
  AgStateMachineState state = AgStateMachineNormal;
  if (wifiConnector.isConnected() == false) {
    state = AgStateMachineWiFiLost;
  } else if (apiClient.isFetchConfigureFailed()) {
    state = AgStateMachineSensorConfigFailed;
  } else if (apiClient.isPostToServerFailed()) {
    state = AgStateMachineServerLost;
  }

  sm.displayHandle(state);
}

/**
 * @brief APP display and LED handler
 *
 */
static void displayAndLedUpdate(void) {
  if (ag->isOneIndoor()) {
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
  agValue.TVOC = ag->sgp41.getTvocIndex();
  agValue.TVOCRaw = ag->sgp41.getTvocRaw();
  agValue.NOx = ag->sgp41.getNoxIndex();
  agValue.NOxRaw = ag->sgp41.getNoxRaw();

  Serial.println();
  Serial.printf("TVOC index: %d\r\n", agValue.TVOC);
  Serial.printf("TVOC raw: %d\r\n", agValue.TVOCRaw);
  Serial.printf("NOx index: %d\r\n", agValue.NOx);
  Serial.printf("NOx raw: %d\r\n", agValue.NOxRaw);
}

/**
 * @brief Update PMS data
 *
 */
static void pmUpdate(void) {
  if (ag->isOneIndoor()) {
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
    agValue.PM25 = pm25_1;
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

    if (pmsResult_1 && pmsResult_2) {
      agValue.Temperature = (temp_1 + temp_2) / 2;
      agValue.Humidity = (hum_1 + hum_2) / 2;
    } else {
      if (pmsResult_1) {
        agValue.Temperature = temp_1;
        agValue.Humidity = hum_1;
      }
      if (pmsResult_2) {
        agValue.Temperature = temp_2;
        agValue.Humidity = hum_2;
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
  if (apiClient.postToServer(syncData)) {
    resetWatchdog();
  }

  bootCount++;
}

/**
 * @brief Update temperature and humidity value
 */
static void tempHumUpdate(void) {
  if (ag->sht.measure()) {
    agValue.Temperature = ag->sht.getTemperature();
    agValue.Humidity = ag->sht.getRelativeHumidity();

    Serial.printf("Temperature in C: %0.2f\r\n", agValue.Temperature);
    Serial.printf("Relative Humidity: %d\r\n", agValue.Humidity);

    // Update compensation temperature and humidity for SGP41
    if (hasSensorSGP) {
      ag->sgp41.setCompensationTemperatureHumidity(agValue.Temperature,
                                                   agValue.Humidity);
    }
  } else {
    Serial.println("SHT read failed");
  }
}
