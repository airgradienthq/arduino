/*
This is the code for the AirGradient DIY CO2 Traffic light with an ESP8266 Microcontroller.

For build instructions please visit https://www.airgradient.com/diy-co2-traffic-light/

Compatible with the following sensors:
SenseAir S8 (CO2 Sensor)

Please install ESP8266 board manager (tested with version 3.0.0)

Please install the following libraries:
"Adafruit NeoMatrix" Library  (tested with 1.2.0)
"WifiManager by tzapu, tablatronix" tested with Version 2.0.3-alpha

Kits with all required components are available at https://www.airgradient.com/diyshop/

If you have any questions please visit our forum at https://forum.airgradient.com/

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/schools/

MIT License
*/

// Adafruit_NeoMatrix example for single NeoPixel Shield.
// Scrolls 'Howdy' across the matrix in a portrait (vertical) orientation.
// Adafruit_NeoMatrix example for single NeoPixel Shield.
// Scrolls 'Howdy' across the matrix in a portrait (vertical) orientation.

#include <AirGradient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

AirGradient ag = AirGradient();
#define PIN D8

int co2 = 0;
String text = "AirGradient CO2";
// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI=true;
int greenToOrange = 800;
int orangeToRed = 1200;

// change if you want to send the data to another server
String APIROOT = "http://hw.airgradient.com/";

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB            + NEO_KHZ800);

void setup() {
  Serial.begin(9600);
  ag.CO2_Init();
  matrix.begin();
  matrix.setRotation(1); // change rotation
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.setTextColor(matrix.Color(70,130,180));

  Serial.println("Chip ID: "+String(ESP.getChipId(),HEX));
  if (connectWIFI) connectToWifi();
  delay(2000);
}

void loop() {
  showText();
}

int x    = matrix.width();

void showText() {
  Serial.println("in loop");
  matrix.fillScreen(0);
  matrix.setCursor(x, 0);
  matrix.print(String(text));
  if(--x < -100) {
    x = matrix.width();
    Serial.println("end text");
    co2 = ag.getCO2_Raw();
    text = String(co2)+"ppm";
      if (co2>350) matrix.setTextColor(matrix.Color(0, 255, 0));
      if (co2>greenToOrange) matrix.setTextColor(matrix.Color(255, 90, 0));
      if (co2>orangeToRed) matrix.setTextColor(matrix.Color(255,0, 0));

    // send payload
    String payload = "{\"wifi\":" + String(WiFi.RSSI()) + ",";
    payload = payload + "\"rco2\":" + String(co2);
    payload = payload + "}";

    if (connectWIFI) {
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

    delay(6000);
  }

  matrix.show();
  delay(100);
}

// Wifi Manager
void connectToWifi(){
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-"+String(ESP.getChipId(),HEX);
  wifiManager.setTimeout(120);
  if(!wifiManager.autoConnect((const char*)HOTSPOT.c_str())) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      ESP.restart();
      delay(5000);
  }
  }
