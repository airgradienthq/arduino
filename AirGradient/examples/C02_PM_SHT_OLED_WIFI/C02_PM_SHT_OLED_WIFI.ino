#include <AirGradient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

AirGradient ag = AirGradient();
#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);

String APIROOT = "http://hw.airgradient.com/";

void setup(){
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  ag.PMS_Init();
  ag.C02_Init();
  ag.TMP_RH_Init(0x44); //check for SHT sensor with address 0x44
  showTextRectangle("Init", String(ESP.getChipId(),HEX),"AirGradient");
  connectToWifi();
  delay(2000);
}

void loop(){
  int PM2 = ag.getPM2();
  int CO2 = ag.getC02();
  TMP_RH result = ag.periodicFetchData();
  showTextRectangle(String(result.t)+"c "+String(result.rh)+"%", "PM2: "+ String(PM2), "CO2: "+String(CO2)+"");
  
  // send payload
  String payload = "{\"pm02\":" + String(PM2) +  ",\"wifi\":" + String(WiFi.RSSI()) +  ",\"rco2\":" + String(CO2) + ",\"atmp\":" + String(result.t) +   ",\"rhum\":" + String(result.rh) + "}";
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
  
  delay(15000);
}

// DISPLAY
void showTextRectangle(String ln1, String ln2, String ln3) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(32,8);
  display.println(ln1);
  display.setTextSize(1);
  display.setCursor(32,16);
  display.println(ln2);
  display.setTextSize(1);
  display.setCursor(32,24);
  display.println(ln3);
  display.display();
}

// Wifi Manager
void connectToWifi(){
  WiFiManager wifiManager;
  //chWiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-"+String(ESP.getChipId(),HEX);
  wifiManager.setTimeout(120);
  if(!wifiManager.autoConnect((const char*)HOTSPOT.c_str())) {
      //Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
  } 
    
}
