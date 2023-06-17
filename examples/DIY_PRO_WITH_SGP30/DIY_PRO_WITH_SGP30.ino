/*
This is the code for the AirGradient DIY PRO Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

Build Instructions: https://www.airgradient.com/open-airgradient/instructions/diy-pro/

Kits (including a pre-soldered version) are available: https://www.airgradient.com/open-airgradient/kits/

IMPORTANT: If you use SGP30 modules, you need to remove the Pull up resistors to make it work.
See https://www.airgradient.com/open-airgradient/instructions/tvoc-on-airgradient-diy-sensor/

If you want to use TVOC we strongly recommend you use the AirGradient SGP4x module available in our online shop.

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.11-beta
“U8g2” by oliver tested with version 2.32.15
“SGP30” by Rob Tilaart tested with Version 0.1.5

Configuration:
Please set in the code below the configuration parameters.

If you have any questions please visit our forum at https://forum.airgradient.com/

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/


#include <AirGradient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include "SGP30.h"
#include <U8g2lib.h>

AirGradient ag = AirGradient();
SGP30 SGP;

// Display bottom right
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Replace above if you have display on top left
//U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);


// CONFIGURATION START

//set to the endpoint you would like to use
String APIROOT = "http://hw.airgradient.com/";

// set to true to switch from Celcius to Fahrenheit
boolean inF = false;

// set to true if you want to connect to wifi. You have 60 seconds to connect. Then it will go into an offline mode.
boolean connectWIFI=true;

// CONFIGURATION END


unsigned long currentMillis = 0;

const int oledInterval = 5000;
unsigned long previousOled = 0;

const int sendToServerInterval = 10000;
unsigned long previoussendToServer = 0;

const int tvocInterval = 1000;
unsigned long previousTVOC = 0;
int TVOC = 0;

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

void setup()
{
  Serial.begin(115200);
  u8g2.setBusClock(100000);
  u8g2.begin();
  updateOLED();

    if (connectWIFI) {
    connectToWifi();
  }

  updateOLED2("Warming up the", "sensors.", "");

  Serial.println(SGP.begin());
  SGP.GenericReset();

  ag.CO2_Init();
  ag.PMS_Init();
  ag.TMP_RH_Init(0x44);
}


void loop()
{
  currentMillis = millis();
  updateTVOC();
  updateOLED();
  updateCo2();
  updatePm25();
  updateTempHum();
  sendToServer();
}

void updateTVOC()
{
    if (currentMillis - previousTVOC >= tvocInterval) {
      previousTVOC += tvocInterval;
      SGP.measure(true);
      TVOC = SGP.getTVOC();
      Serial.println(String(TVOC));
    }
}

void updateCo2()
{
    if (currentMillis - previousCo2 >= co2Interval) {
      previousCo2 += co2Interval;
      Co2 = ag.getCO2_Raw();
      Serial.println(String(Co2));
    }
}

void updatePm25()
{
    if (currentMillis - previousPm25 >= pm25Interval) {
      previousPm25 += pm25Interval;
      pm25 = ag.getPM2_Raw();
      Serial.println(String(pm25));
    }
}

void updateTempHum()
{
    if (currentMillis - previousTempHum >= tempHumInterval) {
      previousTempHum += tempHumInterval;
      TMP_RH result = ag.periodicFetchData();
      temp = result.t;
      hum = result.rh;
      Serial.println(String(temp));
    }
}

void updateOLED() {
   if (currentMillis - previousOled >= oledInterval) {
     previousOled += oledInterval;

    String ln3;
    String ln1 = "PM:" + String(pm25) +  " CO2:" + String(Co2);
    String ln2 = "AQI:" + String(PM_TO_AQI_US(pm25)) + " TVOC:" + String(TVOC);

      if (inF) {
        ln3 = "F:" + String((temp* 9 / 5) + 32) + " H:" + String(hum)+"%";
        } else {
        ln3 = "C:" + String(temp) + " H:" + String(hum)+"%";
       }
     updateOLED2(ln1, ln2, ln3);
   }
}

void updateOLED2(String ln1, String ln2, String ln3) {
      char buf[9];
          u8g2.firstPage();
          u8g2.firstPage();
          do {
          u8g2.setFont(u8g2_font_t0_16_tf);
          u8g2.drawStr(1, 10, String(ln1).c_str());
          u8g2.drawStr(1, 30, String(ln2).c_str());
          u8g2.drawStr(1, 50, String(ln3).c_str());
            } while ( u8g2.nextPage() );
}

void sendToServer() {
   if (currentMillis - previoussendToServer >= sendToServerInterval) {
     previoussendToServer += sendToServerInterval;

      String payload = "{\"wifi\":" + String(WiFi.RSSI())
      + (Co2 < 0 ? "" : ", \"rco2\":" + String(Co2))
      + (pm25 < 0 ? "" : ", \"pm02\":" + String(pm25))
      + (TVOC < 0 ? "" : ", \"tvoc\":" + String(TVOC))
      + ", \"atmp\":" + String(temp)
      + (hum < 0 ? "" : ", \"rhum\":" + String(hum))
      + "}";

      if(WiFi.status()== WL_CONNECTED){
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
   String HOTSPOT = "AG-" + String(ESP.getChipId(), HEX);
   updateOLED2("60s to connect", "to Wifi Hotspot", HOTSPOT);
   wifiManager.setTimeout(60);
   if (!wifiManager.autoConnect((const char * ) HOTSPOT.c_str())) {
     updateOLED2("booting into", "offline mode", "");
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
