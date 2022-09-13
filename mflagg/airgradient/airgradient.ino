/*
  This is the code for the AirGradient DIY BASIC Air Quality Sensor with an ESP8266 Microcontroller.

  It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

  Build Instructions: https://www.airgradient.com/open-airgradient/instructions/diy/

  Kits (including a pre-soldered version) are available: https://www.airgradient.com/open-airgradient/kits/

  The codes needs the following libraries installed:
  “WifiManager by tzapu, tablatronix” tested with version 2.0.11-beta
  “U8g2” by oliver tested with version 2.32.15
  "ESP8266 and ESP32 OLED driver for SSD1306 displays" by ThingPulse

  Configuration:
  Please set in the code below the configuration parameters.

  If you have any questions please visit our forum at https://forum.airgradient.com/

  If you are a school or university contact us for a free trial on the AirGradient platform.
  https://www.airgradient.com/

  MIT License

*/


#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <SSD1306Wire.h>

#include "src/AirGradient/AirGradient.h"
//#include "src/font/font.h"

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

// CONFIGURATION START

// set to true to switch from Celcius to Fahrenheit
boolean inF = true;

// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI = true;

// CONFIGURATION END

unsigned long currentMillis = 0;

const int oledInterval = 2000;
unsigned long previousOled = 0;

const int sendToServerInterval = 10000;
unsigned long previoussendToServer = 0;

const int co2Interval = 5000;
unsigned long previousCo2 = 0;
int Co2 = 0;

const int pm2_5Interval = 5000;
unsigned long previousPm2_5 = 0;
int pm1 = 0;
int pm2_5 = 0;
int pm10 = 0;

const int tempHumInterval = 2500;
unsigned long previousTempHum = 0;
float temp = 0;
float temp_offset_c = -2.6;
int hum = 0;
int hum_offset = 12;
int displaypage = 0;

String APIROOT = "http://hw.airgradient.com/";

void setup()
{
  Serial.begin(115200);

  display.init();
  display.flipScreenVertically();

  if (connectWIFI) {
    connectToWifi();
  }

  showTextRectangle("Init", String(ESP.getChipId(), HEX), true, false);

  ag.CO2_Init();
  ag.PMS_Init();
  ag.TMP_RH_Init(0x44);

  // 0 - 255
  display.setBrightness(75);
}


void loop()
{
  currentMillis = millis();
  updateOLED();
  updateCo2();
  updatePm25();
  updateTempHum();
  sendToServer();
}

void updateCo2()
{
  if (currentMillis - previousCo2 >= co2Interval) {
    previousCo2 += co2Interval;
    Co2 = ag.getCO2_Raw();
    Serial.println("CO2: " + String(Co2));
  }
}

void updatePm25()
{
  if (currentMillis - previousPm2_5 >= pm2_5Interval) {
    previousPm2_5 += pm2_5Interval;
    pm1 = ag.getPM1_Raw();
    pm2_5 = ag.getPM2_Raw();
    pm10 = ag.getPM10_Raw();
    Serial.println("PM1: " + String(pm1) + " PM 2.5: " + String(pm2_5) + " PM10: " + String(pm10));
  }
}

void updateTempHum()
{
  if (currentMillis - previousTempHum >= tempHumInterval) {
    previousTempHum += tempHumInterval;
    TMP_RH result = ag.periodicFetchData();
    temp = result.t + temp_offset_c;
    hum = result.rh + hum_offset;
    Serial.println("Temp: " + String(temp) + " Humidity: " + hum);
  }
}

void updateOLED() {
  if (currentMillis - previousOled >= oledInterval) {
    previousOled += oledInterval;

    switch (displaypage) {
      case 0:
        showTextRectangle("AQI", String(PM_TO_AQI_US(pm2_5)), false, false);
        displaypage = 1;
        break;
      case 1:
        showTextRectangle("PM2.5", String(pm2_5), false, true);
        displaypage = 2;
        break;
      case 2:
        showTextRectangle("PM1", String(pm1), false, false);
        displaypage = 3;
        break;
      case 3:
        showTextRectangle("PM10", String(pm10), false, false);
        displaypage = 4;
        break;
      case 4:
        showTextRectangle("CO2", String(Co2), false, false);
        displaypage = 5;
        break;
      case 5:
        if (inF) {
          showTextRectangle("°F", String((temp * 9 / 5) + 32, 1), false, false);
        } else {
          showTextRectangle("°C", String(temp), false, false);
        }
        displaypage = 6;
        break;
      case 6:
        showTextRectangle("Hum", String(hum) + "%", false, false);
        displaypage = 0;
        break;
    }
  }
}

void showTextRectangle(String ln1, String ln2, boolean small, boolean slightly_smaller_header) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (slightly_smaller_header) {
    display.setFont(ArialMT_Plain_16);
    display.drawString(32, 16, ln1);
    display.setFont(ArialMT_Plain_24);
    display.drawString(32, 38, ln2);
    display.display();
  } else {
    if (small) {
      display.setFont(ArialMT_Plain_16);
    } else {
      display.setFont(ArialMT_Plain_24);
    }
    display.drawString(32, 16, ln1);
    display.drawString(32, 38, ln2);
    display.display();
  }
}

void sendToServer() {
  if (currentMillis - previoussendToServer >= sendToServerInterval) {
    previoussendToServer += sendToServerInterval;
    String payload = "{\"wifi\":" + String(WiFi.RSSI())
                     + ", \"rco2\":" + String(Co2)
                     + ", \"pm02\":" + String(pm2_5)
                     + ", \"atmp\":" + String(temp)
                     + ", \"rhum\":" + String(hum)
                     + "}";

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(payload);
      String POSTURL = APIROOT + "sensors/airgradient:" + String(ESP.getChipId(), HEX) + "/measures";
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
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  }
}

// Wifi Manager
void connectToWifi() {
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-" + String(ESP.getChipId(), HEX);
  wifiManager.setTimeout(60);
  showTextRectangle("WiFi", "Conn.", true, false);
  if (!wifiManager.autoConnect((const char * ) HOTSPOT.c_str())) {
    showTextRectangle("offline", "mode", true, false);
    Serial.println("failed to connect and hit timeout");
    delay(6000);
  }
}

// Calculate PM2.5 US AQI
int PM_TO_AQI_US(int pm02) {
  if (pm02 <= 12.0) return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4) return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4) return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4) return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4) return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4) return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4) return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else return 500;
};
