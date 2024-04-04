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
#include "AgSchedule.h"
#include "AgConfigure.h"
#include "AgApiClient.h"
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
#define SENSOR_TEMP_HUM_UPDATE_INTERVAL 2000 /** ms */
#define DISPLAY_DELAY_SHOW_CONTENT_MS 2000   /** ms */
#define WIFI_HOTSPOT_PASSWORD_DEFAULT                                          \
  "cleanair" /** default WiFi AP password                                      \
              */

/** Create airgradient instance for 'DIY_BASIC' board */
static AirGradient ag = AirGradient(DIY_BASIC);
static AgConfigure localConfig(Serial);
static AgApiClient apiClient(Serial, localConfig);

static int co2Ppm = -1;
static int pm25 = -1;
static float temp = -1001;
static int hum = -1;
static long val;
static String wifiSSID = "";
static bool wifiHasConfig = false; /** */

static void boardInit(void);
static void failedHandler(String msg);
static void co2Calibration(void);
static void updateServerConfiguration(void);
static void co2Update(void);
static void pmUpdate(void);
static void tempHumUpdate(void);
static void sendDataToServer(void);
static void dispHandler(void);
static String getDevId(void);
static void updateWiFiConnect(void);
static void showNr(void);

bool hasSensorS8 = true;
bool hasSensorPMS = true;
bool hasSensorSHT = true;
int pmFailCount = 0;
int getCO2FailCount = 0;
AgSchedule configSchedule(SERVER_CONFIG_UPDATE_INTERVAL,
                          updateServerConfiguration);
AgSchedule serverSchedule(SERVER_SYNC_INTERVAL, sendDataToServer);
AgSchedule dispSchedule(DISP_UPDATE_INTERVAL, dispHandler);
AgSchedule co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Update);
AgSchedule pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, pmUpdate);
AgSchedule tempHumSchedule(SENSOR_TEMP_HUM_UPDATE_INTERVAL, tempHumUpdate);

void setup() {
  Serial.begin(115200);
  showNr();

  /** Init I2C */
  Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin());
  delay(1000);

  /** Board init */
  boardInit();

  /** Init AirGradient server */
  apiClient.begin();
  apiClient.setAirGradient(&ag);

  /** Show boot display */
  displayShowText("DIY basic", "Lib:" + ag.getVersion(), "");
  delay(2000);

  /** WiFi connect */
  connectToWifi();
  if (WiFi.status() == WL_CONNECTED) {
    wifiHasConfig = true;
    sendPing();

    apiClient.fetchServerConfiguration();
    if (localConfig.isCo2CalibrationRequested()) {
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
  if (hasSensorS8) {
    co2Schedule.run();
  }
  if (hasSensorPMS) {
    pmsSchedule.run();
  }
  if (hasSensorSHT) {
    tempHumSchedule.run();
  }

  updateWiFiConnect();

  /** Read PMS on loop */
  ag.pms5003.handle();
}

static void sendPing() {
  JSONVar root;
  root["wifi"] = WiFi.RSSI();
  root["boot"] = 0;

  // delay(1500);
  if (apiClient.postToServer(JSON.stringify(root))) {
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
  delay(100);
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
    hasSensorSHT = false;
    Serial.println("SHT sensor not found");
  }

  /** CO2 init */
  if (ag.s8.begin(&Serial) == false) {
    Serial.println("CO2 S8 snsor not found");
    hasSensorS8 = false;
  }

  /** PMS init */
  if (ag.pms5003.begin(&Serial) == false) {
    Serial.println("PMS sensor not found");
    hasSensorPMS = false;
  }

  /** Display init */
  ag.display.begin(Wire);
  ag.display.setTextColor(1);
  ag.display.clear();
  ag.display.show();
  delay(100);
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

static void updateServerConfiguration(void) {
  if (apiClient.fetchServerConfiguration()) {
    if (localConfig.isCo2CalibrationRequested()) {
      if (hasSensorS8) {
        co2Calibration();
      } else {
        Serial.println("CO2 S8 not available, calib ignored");
      }
    }
    if (localConfig.getCO2CalirationAbcDays() > 0) {
      if (hasSensorS8) {
        int newHour = localConfig.getCO2CalirationAbcDays() * 24;
        Serial.printf("abcDays config: %d days(%d hours)\r\n",
                      localConfig.getCO2CalirationAbcDays(), newHour);
        int curHour = ag.s8.getAbcPeriod();
        Serial.printf("Current config: %d (hours)\r\n", curHour);
        if (curHour == newHour) {
          Serial.println("set 'abcDays' ignored");
        } else {
          if (ag.s8.setAbcPeriod(localConfig.getCO2CalirationAbcDays() * 24) ==
              false) {
            Serial.println("Set S8 abcDays period calib failed");
          } else {
            Serial.println("Set S8 abcDays period calib success");
          }
        }
      } else {
        Serial.println("CO2 S8 not available, set 'abcDays' ignored");
      }
    }
  }
}

static void co2Update() {
  int value = ag.s8.getCo2();
  if (value >= 0) {
    co2Ppm = value;
    getCO2FailCount = 0;
    Serial.printf("CO2 index: %d\r\n", co2Ppm);
  } else {
    getCO2FailCount++;
    Serial.printf("Get CO2 failed: %d\r\n", getCO2FailCount);
    if (getCO2FailCount >= 3) {
      co2Ppm = -1;
    }
  }
}

void pmUpdate() {
  if (ag.pms5003.isFailed() == false) {
    pm25 = ag.pms5003.getPm25Ae();
    Serial.printf("PMS2.5: %d\r\n", pm25);
    pmFailCount = 0;
  } else {
    Serial.printf("PM read failed, %d", pmFailCount);
    pmFailCount++;
    if (pmFailCount >= 3) {
      pm25 = -1;
    }
  }
}

static void tempHumUpdate() {
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
  if (temp > -1001) {
    root["atmp"] = ag.round2(temp);
  }
  if (hum >= 0) {
    root["rhum"] = hum;
  }

  if (apiClient.postToServer(JSON.stringify(root)) == false) {
    Serial.println("Post to server failed");
  }
}

static void dispHandler() {
  String ln1 = "";
  String ln2 = "";
  String ln3 = "";

  if (localConfig.isPmStandardInUSAQI()) {
    if (pm25 < 0) {
      ln1 = "AQI: -";
    } else {
      ln1 = "AQI:" + String(ag.pms5003.convertPm25ToUsAqi(pm25));
    }
  } else {
    if (pm25 < 0) {
      ln1 = "PM :- ug";

    } else {
      ln1 = "PM :" + String(pm25) + " ug";
    }
  }
  if (co2Ppm > -1001) {
    ln2 = "CO2:" + String(co2Ppm);
  } else {
    ln2 = "CO2: -";
  }

  String _hum = "-";
  if (hum > 0) {
    _hum = String(hum);
  }

  String _temp = "-";

  if (localConfig.isTemperatureUnitInF()) {
    if (temp > -1001) {
      _temp = String((temp * 9 / 5) + 32).substring(0, 4);
    }
    ln3 = _temp + " " + _hum + "%";
  } else {
    if (temp > -1001) {
      _temp = String(temp).substring(0, 4);
    }
    ln3 = _temp + " " + _hum + "%";
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

static void showNr(void) {
  Serial.println();
  Serial.println("Serial nr: " + getDevId());
}

String getNormalizedMac() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}
