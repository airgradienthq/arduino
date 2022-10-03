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
#include "src/font/font.h"

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

// CONFIGURATION START

// set to true to switch from Celcius to Fahrenheit
boolean inF = true;

// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI = true;

// CONFIGURATION END

unsigned long currentMillis = 0;

const int oledInterval = 3000;
unsigned long previousOled = 0;

const int invertInterval = 3600000;
unsigned long previousInvert = 0;
bool inverted = false;

const int sendToHubitatInterval = 30000;
unsigned long previoussendToHubitat = 0;

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
float temp_offset_c = -3.2;
int hum = 0;
int hum_offset = 14;
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

  showTextRectangle("Init", String(ESP.getChipId(), HEX), true);

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
  sendToHubitat();
  sendToServer();
}

void updateCo2()
{
  if (currentMillis - previousCo2 >= co2Interval) {
    previousCo2 += co2Interval;
    int reading = ag.getCO2_Raw();
    if (reading > 0) {
      Co2 = reading;
    } else {
      Serial.println("Invalid CO2: " + String(reading));
    }

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

    if (((result.t >= 0.01) || (result.t <= -0.01)) && result.rh > 0) {
      temp = result.t + temp_offset_c;
      hum = result.rh + hum_offset;
    } else {
      Serial.println("Invalid temp or humidity: " + String(result.t) + " " + result.rh);
    }

    Serial.println("Temp: " + String(temp) + " Humidity: " + hum);
  }
}

void updateOLED() {
  if (currentMillis - previousInvert >= invertInterval) {
    previousInvert = currentMillis;
    if (inverted) {
      display.normalDisplay();
      inverted = false;
    } else {
      display.invertDisplay();
      inverted = true;
    }
  }

  if (currentMillis - previousOled >= oledInterval) {
    previousOled = currentMillis;

    switch (displaypage) {
      case 0:
        showFancyText( "CO2", String(Co2), "PM\n2.5", String(PM_TO_AQI_US(pm2_5)));
        displaypage = 1;
        break;
      case 1:
        String tempLabel = "°C";
        float tempToPrint = temp;
        if (inF) {
          tempLabel = "°F";
          tempToPrint = (temp * 9 / 5) + 32;
        }

        showFancyText( tempLabel, String(tempToPrint, 1), "Hum", String(hum) + "%");
        displaypage = 0;
        break;
    }
  }
}

void showTextRectangle(String ln1, String ln2, boolean small) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  if (small) {
    display.setFont(ArialMT_Plain_16);
  } else {
    display.setFont(ArialMT_Plain_24);
  }

  display.drawString(32, 16, ln1);
  display.drawString(32, 38, ln2);
  display.display();
}

void printFancyLabel(String text, bool top) {
  // display.getStringWidth() appears broken
  if (text.indexOf("%") == -1 &&
      (text.length() <= 3 ||
       (text.length() <= 4 && text.indexOf(".") != -1))) {
    display.setFont(ArialMT_Plain_24);

    if (top) {
      display.drawString(96, 12, text);
    } else {
      display.drawString(96, 37, text);
    }
  } else {
    display.setFont(ArialMT_Plain_16);

    if (top) {
      display.drawString(97, 14, text);
    } else {
      display.drawString(97, 39, text);
    }
  }

}

void showFancyText(String label1, String data1, String label2, String data2) {
  display.clear();

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(32, 14, label1);
  display.drawString(32, 40, label2);

  display.setTextAlignment(TEXT_ALIGN_RIGHT);

  printFancyLabel(data1, true);
  printFancyLabel(data2, false);

  display.display();
}

void getURL(String URL) {
  //Serial.println(URL);
  WiFiClient client;
  HTTPClient http;
  http.begin(client, URL);
  http.addHeader("content-type", "application/json");
  int httpCode = http.GET();
  String response = http.getString();
  //Serial.println("Hubitat: " + httpCode);
  //Serial.println("Hubitat: " + response);
  http.end();
}

void sendToHubitat() {
  if (currentMillis - previoussendToHubitat >= sendToHubitatInterval) {
    previoussendToHubitat += sendToHubitatInterval;

    const String urlBase = "http://10.0.40.10/apps/api/727/devices/";
    const String device = "821/";
    // Yeah, it's public on github. Good luck doing anything with it.
    const String token = "?access_token=24c92325-da8b-4017-ba30-4bb678b9dd8b";

    if (WiFi.status() == WL_CONNECTED) {
      // TODO POST would be nice
      getURL(urlBase + device + "setCarbonDioxide/" + String(Co2) + token);
      getURL(urlBase + device + "setPM1/" + String(pm1) + token);
      getURL(urlBase + device + "setPM2_5/" + String(pm2_5) + token);
      getURL(urlBase + device + "setPM10/" + String(pm10) + token);
      getURL(urlBase + device + "setAQI_PM2_5/" + String(PM_TO_AQI_US(pm2_5)) + token);
      getURL(urlBase + device + "setTemperature/" + String((temp * 9 / 5) + 32) + token);
      getURL(urlBase + device + "setRelativeHumidity/" + String(hum) + token);
      Serial.println("Sent to Hubitat");
    }
    else {
      Serial.println("WiFi Disconnected");
    }
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
      //Serial.println(payload);
      String POSTURL = APIROOT + "sensors/airgradient:" + String(ESP.getChipId(), HEX) + "/measures";
      //Serial.println(POSTURL);
      WiFiClient client;
      HTTPClient http;
      http.begin(client, POSTURL);
      http.addHeader("content-type", "application/json");
      int httpCode = http.POST(payload);
      String response = http.getString();
      //Serial.println("AirGradient API: " + httpCode);
      //Serial.println("AirGradient API: " + response);
      http.end();
      Serial.println("Sent to AirGradient API");
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
  wifiManager.setConnectTimeout(10);
  wifiManager.setConfigPortalTimeout(30);
  showTextRectangle("WiFi", String(ESP.getChipId(), HEX), true);
  if (!wifiManager.autoConnect((const char * ) HOTSPOT.c_str())) {
    showTextRectangle("Offline", "mode", true);
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
