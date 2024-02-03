/*
This is the code for the AirGradient DIY BASIC Air Quality Monitor with an D1 ESP8266 Microcontroller.

It is an air quality monitor for PM2.5, CO2, Temperature and Humidity with a small display and can send data over Wifi.

Open source air quality monitors and kits are available:
Indoor Monitor: https://www.airgradient.com/indoor/
Outdoor Monitor: https://www.airgradient.com/outdoor/

Build Instructions:
https://www.airgradient.com/documentation/diy-v4/

Following libraries need to be installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.16-rc.2
"Arduino_JSON" by Arduino version 0.2.0
"U8g2" by oliver version 2.34.22

Please make sure you have esp8266 board manager installed. Tested with version 3.1.2.

Set board to "LOLIN(WEMOS) D1 R2 & mini"

Configuration parameters, e.g. Celsius / Fahrenheit or PM unit (US AQI vs ug/m3) can be set through the AirGradient dashboard.

If you have any questions please visit our forum at
https://forum.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include <AirGradient.h>
#include <Arduino_JSON.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <U8g2lib.h>
#include <WiFiClient.h>
#include <WiFiManager.h>

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

AirGradient ag = AirGradient(BOARD_DIY_BASIC_KIT);

// CONFIGURATION START

// set to the endpoint you would like to use
String APIROOT = "http://hw.airgradient.com/";

String wifiApPass = "cleanair";

// set to true if you want to connect to wifi. You have 60 seconds to connect.
// Then it will go into an offline mode.
boolean connectWIFI = true;

// CONFIGURATION END

unsigned long currentMillis = 0;

const int oledInterval = 5000;
unsigned long previousOled = 0;
bool co2CalibrationRequest = false;
uint32_t serverConfigLoadTime = 0;
String HOSTPOT = "";

const int sendToServerInterval = 60000;
const int pollServerConfigInterval = 30000;
const int co2CalibCountdown = 5; /** Seconds */
unsigned long previoussendToServer = 0;

const int co2Interval = 5000;
unsigned long previousCo2 = 0;
int Co2 = 0;

const int pm25Interval = 5000;
unsigned long previousPm25 = 0;
int pm25 = 0;

const int tempHumInterval = 2500;
unsigned long previousTempHum = 0;
float temp = 0;
int hum = 0;
long val;

void failedHandler(String msg);
void boardInit(void);
void getServerConfig(void);
void co2Calibration(void);

void setup() {
  Serial.begin(115200);

  /** Init I2C */
  Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin());

  /** Board init */
  boardInit();

  /** Show boot display */
  displayShowText("Basic v4", "Lib:" + ag.getVersion(), "");
  delay(2000);

  if (connectWIFI) {
    connectToWifi();
  }

  /** Show display */
  displayShowText("Warm Up", "Serial#", String(ESP.getChipId(), HEX));
  delay(10000);

  getServerConfig();
}

void loop() {
  currentMillis = millis();
  updateOLED();
  updateCo2();
  updatePm25();
  updateTempHum();
  sendToServer();
  getServerConfig();
}

void updateCo2() {
  if (currentMillis - previousCo2 >= co2Interval) {
    previousCo2 += co2Interval;
    Co2 = ag.s8.getCo2();
    Serial.println(String(Co2));
  }
}

void updatePm25() {
  if (currentMillis - previousPm25 >= pm25Interval) {
    previousPm25 += pm25Interval;
    if (ag.pms5003.readData()) {
      pm25 = ag.pms5003.getPm25Ae();
      Serial.printf("PM25: %d\r\n", pm25);
    }
  }
}

void updateTempHum() {
  if (currentMillis - previousTempHum >= tempHumInterval) {
    previousTempHum += tempHumInterval;

    /** Get temperature and humidity */
    temp = ag.sht.getTemperature();
    hum = ag.sht.getRelativeHumidity();

    /** Print debug message */
    Serial.printf("SHT Humidity: %d%, Temperature: %0.2f\r\n", hum, temp);
  }
}

void updateOLED() {
  if (currentMillis - previousOled >= oledInterval) {
    previousOled += oledInterval;

    String ln1;
    String ln2;
    String ln3;

    if (serverConfig.inUSAQI) {
      ln1 = "AQI:" + String(ag.pms5003.convertPm25ToUsAqi(pm25));
    } else {
      ln1 = "PM :" + String(pm25) + " ug";
    }
    ln2 = "CO2:" + String(Co2);

    if (serverConfig.inF) {
      ln3 =
          String((temp * 9 / 5) + 32).substring(0, 4) + " " + String(hum) + "%";
    } else {
      ln3 = String(temp).substring(0, 4) + " " + String(hum) + "%";
    }
    displayShowText(ln1, ln2, ln3);
  }
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

void sendToServer() {
  if (currentMillis - previoussendToServer >= sendToServerInterval) {
    previoussendToServer += sendToServerInterval;

    String payload = "{\"wifi\":" + String(WiFi.RSSI()) +
                     (Co2 < 0 ? "" : ", \"rco2\":" + String(Co2)) +
                     (pm25 < 0 ? "" : ", \"pm02\":" + String(pm25)) +
                     ", \"atmp\":" + String(temp) +
                     (hum < 0 ? "" : ", \"rhum\":" + String(hum)) + "}";

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(payload);
      String POSTURL = APIROOT +
                       "sensors/airgradient:" + String(ESP.getChipId(), HEX) +
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
    } else {
      Serial.println("WiFi Disconnected");
    }
  }
}

// Wifi Manager
void connectToWifi() {
  WiFiManager wifiManager;
  // WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AG-" + String(ESP.getChipId(), HEX);
  // displayShowText("Connect", "AG-", String(ESP.getChipId(), HEX));
  delay(2000);
  // wifiManager.setTimeout(90);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.autoConnect(HOTSPOT.c_str(), wifiApPass.c_str());

  uint32_t lastTime = millis();
  int count = 179;
  displayShowText("180 sec", "SSID:",HOTSPOT);
  while (wifiManager.getConfigPortalActive()) {
    wifiManager.process();
    uint32_t ms = (uint32_t)(millis() - lastTime);
    if (ms >= 1000) {
      lastTime = millis();
      displayShowText(String(count) +  " sec", "SSID:",HOTSPOT);
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
    delay(6000);
  }
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}

void boardInit(void) {
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

void updateServerConfigLoadTime(void) {
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
                  String(ESP.getChipId(), HEX) + "/one/config";
  Serial.println("HttpClient get: " + getUrl);
  WiFiClient client;
  if (httpClient.begin(client, getUrl) == false) {
    Serial.println("HttpClient init failed");
    updateServerConfigLoadTime();
    return;
  }

  int respCode = httpClient.GET();

  /** get failure */
  if (respCode != 200) {
    Serial.printf("HttpClient get failed: %d\r\n", respCode);
    updateServerConfigLoadTime();
    return;
  }

  String respContent = httpClient.getString();
  Serial.println("Server config: " + respContent);

  /** Parse JSON */
  JSONVar root = JSON.parse(respContent);
  if (JSON.typeof_(root) == "undefined") {
    Serial.println("Server configura JSON invalid");
    updateServerConfigLoadTime();
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
    displayShowText("CO2 calib", "after",
                    String(co2CalibCountdown - i) + " sec");
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
    delay(2000);
  } else {
    displayShowText("Calib", "failure!!!", "");
    delay(2000);
  }
}
