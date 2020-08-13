#include <AirGradient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <Wire.h>
#include "SSD1306Wire.h"

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

String APIROOT = "http://hw.airgradient.com/";

void setup(){
  Serial.begin(9600);

  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(),HEX));

  ag.PMS_Init();
  ag.CO2_Init();
  ag.TMP_RH_Init(0x44);

  connectToWifi();
  delay(2000);
}

void loop(){
  int PM2 = ag.getPM2_Raw();
  int CO2 = ag.getCO2_Raw();
  TMP_RH result = ag.periodicFetchData();

  showTextRectangle(String(result.t),String(result.rh)+"%");
  delay(2000);
  showTextRectangle("PM2",String(PM2));
  delay(2000);
  showTextRectangle("CO2",String(CO2));
  delay(2000);

  // send payload
  String payload = "{\"pm02\":" + String(ag.getPM2()) +  ",\"wifi\":" + String(WiFi.RSSI()) +  ",\"rco2\":" + String(ag.getCO2()) + ",\"atmp\":" + String(result.t) +   ",\"rhum\":" + String(result.rh) + "}";
  Serial.println(payload);
  String POSTURL = APIROOT + "sensors/airgradient:" + String(ESP.getChipId(),HEX) + "/measures";
  Serial.println(POSTURL);
  HTTPClient http;
  http.begin(POSTURL);
  http.addHeader("content-type", "application/json");
  int httpCode = http.POST(payload);
  String response = http.getString();
  Serial.println(httpCode);
  Serial.println(response);
  http.end();

  delay(2000);
}

// DISPLAY
void showTextRectangle(String ln1, String ln2) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(32, 12, ln1);
  display.drawString(32, 36, ln2);
  display.display();
}

// Wifi Manager
void connectToWifi(){
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-"+String(ESP.getChipId(),HEX);
  wifiManager.setTimeout(120);
  if(!wifiManager.autoConnect((const char*)HOTSPOT.c_str())) {
      //Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
  }

}
