/*
Important: This code is only for the AirGradient DIY OUTDOOR.

Build Instructions: https://www.airgradient.com/open-airgradient/instructions/diy-outdoor/

Kits (including a pre-soldered version) are available: https://www.airgradient.com/open-airgradient/kits/

Configuration:
Install required libraries
Patch PMS library to accept temperature and humidity from PMS5003T

If you have any questions please visit our forum at https://forum.airgradient.com/

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/

License: CC BY-NC 4.0 Attribution-NonCommercial 4.0 International
*/


#include "PMS.h"
#include "SoftwareSerial.h"
#include <Wire.h>

#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

SoftwareSerial softSerial(D6, D5);
SoftwareSerial soft2(D3, D7);


PMS pms(softSerial);
PMS pms2(soft2);
PMS::DATA data;
PMS::DATA data2;

float pm1Value=0;
int pm1Position = 0;

float pm2Value=0;
int pm2Position = 0;

float temp_pm1 = 0;
float hum_pm1 = 0;

float temp_pm2 = 0;
float hum_pm2 = 0;

unsigned long currentMillis = 0;

const int pm1Interval = 5000;
unsigned long previousPm1 = 0;

const int pm2Interval = 5000;
unsigned long previousPm2 = 0;

String APIROOT = "http://hw.airgradient.com/";

void setup()
{
  Serial.begin(115200);
  softSerial.begin(9600);
  soft2.begin(9600);
  Wire.begin();
  pinMode(D7, OUTPUT);
  connectToWifi();
}


void loop()
{
  currentMillis = millis();
  updatePm1();
  updatePm2();
}


void updatePm1()
{
    if (currentMillis - previousPm1 >= pm1Interval) {
      digitalWrite(D7, HIGH);
      delay(400);
      digitalWrite(D7, LOW);
      Serial.println("updatePm1: "+String(pm1Position));
      previousPm1 += pm1Interval;
        pms.requestRead();
        if (pms.readUntil(data)){
          Serial.println("success read");
            int pm1 = data.PM_AE_UG_2_5;
            temp_pm1 = data.AMB_TMP;
            hum_pm1 = data.AMB_HUM;
            Serial.print("PMS 1: PM 2.5 (ug/m3): ");
            Serial.println(pm1);
            Serial.print("PMS 1: Temp: ");
            Serial.println(temp_pm1);
            Serial.print("PMS 1: Hum: ");
            Serial.println(hum_pm1);
            Serial.println();
            delay(1000);
            pm1Value=pm1Value+pm1;
            pm1Position++;
            if (pm1Position==20) {
              sendToServerPM1(pm1Value);
              pm1Position=0;
              pm1Value=0;
            }
              }
    }
}

void updatePm2()
{
    if (currentMillis - previousPm2 >= pm2Interval) {
      Serial.println("updatePm2: "+String(pm2Position));
      previousPm2 += pm2Interval;
      pms2.requestRead();
          if (pms2.readUntil(data2)){
            int pm2 = data2.PM_AE_UG_2_5;
            temp_pm2 = data2.AMB_TMP ;
            hum_pm2 = data2.AMB_HUM;
            Serial.print("PMS 2: PM 2.5 (ug/m3): ");
            Serial.println(pm2);
            Serial.print("PMS 2: Temp: ");
            Serial.println(temp_pm2);
            Serial.print("PMS 2: Hum: ");
            Serial.println(hum_pm2);
            Serial.println();
            delay(1000);
        pm2Value=pm2Value+pm2;
            pm2Position++;
            if (pm2Position==20) {
              sendToServerPM2(pm2Value);
              pm2Position=0;
              pm2Value=0;
            }
              }
    }
}

void sendToServerPM1(float pm1Value) {
      String payload = "{\"wifi\":" + String(WiFi.RSSI())
      + ", \"pm02\":" + String(pm1Value/20)
      + ", \"atmp\":" + String(temp_pm1/10)
      + ", \"rhum\":" + String(hum_pm1/10)
      + "}";

      if(WiFi.status()== WL_CONNECTED){
        digitalWrite(D7, HIGH);
        delay(300);
        Serial.println(payload);
        String POSTURL = APIROOT + "sensors/airgradient:" + String(ESP.getChipId(), HEX) + "-1/measures";
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
        digitalWrite(D7, LOW);
      }
      else {
        Serial.println("WiFi Disconnected");
      }
}


void sendToServerPM2(float pm2Value) {
      String payload = "{\"wifi\":" + String(WiFi.RSSI())
      + ", \"pm02\":" + String(pm2Value/20)
      + ", \"atmp\":" + String(temp_pm2/10)
      + ", \"rhum\":" + String(hum_pm2/10)
      + "}";

      if(WiFi.status()== WL_CONNECTED){
        digitalWrite(D7, HIGH);
        delay(300);
        Serial.println(payload);
        String POSTURL = APIROOT + "sensors/airgradient:" + String(ESP.getChipId(), HEX) + "-2/measures";
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
        digitalWrite(D7, LOW);
      }
      else {
        Serial.println("WiFi Disconnected");
      }
}


// Wifi Manager
 void connectToWifi() {
   WiFiManager wifiManager;
   //WiFi.disconnect(); //to delete previous saved hotspot
   String HOTSPOT = "AIRGRADIENT-" + String(ESP.getChipId(), HEX);
   wifiManager.setTimeout(60);
   if (!wifiManager.autoConnect((const char * ) HOTSPOT.c_str())) {
     Serial.println("failed to connect and hit timeout");
     delay(6000);
   }
}
