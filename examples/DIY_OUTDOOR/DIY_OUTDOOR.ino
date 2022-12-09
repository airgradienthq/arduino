#include "PMS.h"
#include "SoftwareSerial.h"
#include <Wire.h>

#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#include "ClosedCube_SHT31D.h"

SoftwareSerial softSerial(D6, D5); //(RX, TX)
SoftwareSerial soft2(D3, D7); //(RX, TX)

ClosedCube_SHT31D sht3xd;

PMS pms(softSerial);
PMS pms2(soft2);
PMS::DATA data;

int pm1 = 0;
int pm2 = 0;
float temp = 0;
int hum = 0;

float temp_pm1 = 0;
float hum_pm1 = 0;

float temp_pm2 = 0;
float hum_pm2 = 0;

String APIROOT = "http://hw.airgradient.com/";

void setup()
{
  Serial.begin(9600);
  Serial.println("hhi");
  softSerial.begin(9600);
  soft2.begin(9600);
  Wire.begin();
  sht3xd.begin(0x44);
  pinMode(D7, OUTPUT);
  
  connectToWifi();
}

void loop()
{

 if (digitalRead(D8) == HIGH) {
    digitalWrite(D7, HIGH);
  }
  else {
    digitalWrite(D7, LOW);
  }
  
  if (pms.read(data)){
    pm1 = data.PM_AE_UG_2_5;
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
  }
  if (pms2.read(data)){
    pm2 = data.PM_AE_UG_2_5;
    temp_pm2 = data.AMB_TMP ;
    hum_pm2 = data.AMB_HUM;
    Serial.print("PMS 2: PM 2.5 (ug/m3): ");
    Serial.println(pm2);
        Serial.print("PMS 2: Temp: ");
    Serial.println(temp_pm2);
    Serial.print("PMS 2: Hum: ");
    Serial.println(hum_pm2);
    Serial.println();
    delay(1000);

    temp = sht3xd.readTempAndHumidity(SHT3XD_REPEATABILITY_LOW, SHT3XD_MODE_CLOCK_STRETCH, 50).t;
    hum = sht3xd.readTempAndHumidity(SHT3XD_REPEATABILITY_LOW, SHT3XD_MODE_CLOCK_STRETCH, 50).rh;
    
    Serial.println(temp);
    Serial.println(hum);

    sendToServer();
    delay(1000);
    sendToServerPM1();
    delay(1000);
    sendToServerPM2();

  }

}



void sendToServer() {
      String payload = "{\"wifi\":" + String(WiFi.RSSI())
      + ", \"pm02\":" + String((pm1+pm2)/2)
      + ", \"atmp\":" + String(temp)
      + ", \"rhum\":" + String(hum)
      + "}";

      if(WiFi.status()== WL_CONNECTED){
        digitalWrite(D7, HIGH);
        delay(300);
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
        digitalWrite(D7, LOW);
      }
      else {
        Serial.println("WiFi Disconnected");
      }
}

void sendToServerPM1() {
      String payload = "{\"wifi\":" + String(WiFi.RSSI())
      + ", \"pm02\":" + String(pm1)
      + ", \"atmp\":" + String(temp_pm1/10)
      + ", \"rhum\":" + String(hum_pm1/10)
      + "}";

      if(WiFi.status()== WL_CONNECTED){
        digitalWrite(D7, HIGH);
        delay(300);
        Serial.println(payload);
        String POSTURL = APIROOT + "sensors/airgradient:" + String(ESP.getChipId(), HEX) + "pm1/measures";
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


void sendToServerPM2() {
      String payload = "{\"wifi\":" + String(WiFi.RSSI())
      + ", \"pm02\":" + String(pm2)
      + ", \"atmp\":" + String(temp_pm2/10)
      + ", \"rhum\":" + String(hum_pm2/10)
      + "}";

      if(WiFi.status()== WL_CONNECTED){
        digitalWrite(D7, HIGH);
        delay(300);
        Serial.println(payload);
        String POSTURL = APIROOT + "sensors/airgradient:" + String(ESP.getChipId(), HEX) + "pm2/measures";
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
