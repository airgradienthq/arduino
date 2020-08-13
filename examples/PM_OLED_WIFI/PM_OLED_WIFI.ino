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
  showTextRectangle("Init", String(ESP.getChipId(),HEX),true);

  ag.PMS_Init();

  connectToWifi();
  delay(2000);
}

void loop(){
  int PM = ag.getPM2_Raw();
  showTextRectangle("PM2",String(PM),false);

  // send payload
  String payload = "{\"pm02\":" + String(PM) +  ",\"wifi\":" + String(WiFi.RSSI()) +  "}";
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

  delay(30000);
}

// DISPLAY
void showTextRectangle(String ln1, String ln2, boolean small) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (small) {
    display.setFont(ArialMT_Plain_16);
  } else {
    display.setFont(ArialMT_Plain_24);
  }
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
