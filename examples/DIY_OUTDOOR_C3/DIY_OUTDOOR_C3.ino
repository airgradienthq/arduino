/*
Important: This code is only for the DIY OUTDOOR OPEN AIR Presoldered Kit with the ESP-C3.

It is a high quality outdoor air quality sensor with dual PM2.5 modules and can send data over Wifi.

Kits are available: https://www.airgradient.com/open-airgradient/kits/

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.11-beta
“pms” by Markusz Kakl version 1.1.0 (needs to be patched for 5003T model)

For built instructions and how to patch the PMS library: https://www.airgradient.com/open-airgradient/instructions/diy-open-air-presoldered-v11/

Note that below code only works with both PM sensor modules connected.

If you have any questions please visit our forum at https://forum.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include "PMS.h"
#include <HardwareSerial.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <WiFiManager.h>

#define DEBUG true

HTTPClient client;

PMS pms1(Serial0);
PMS::DATA data1;

float pm1Value01=0;
float pm1Value25=0;
float pm1Value10=0;
float pm1PCount=0;
float pm1temp = 0;
float pm1hum = 0;

PMS pms2(Serial1);
PMS::DATA data2;

float pm2Value01=0;
float pm2Value25=0;
float pm2Value10=0;
float pm2PCount=0;
float pm2temp = 0;
float pm2hum = 0;

int countPosition = 0;
int targetCount = 20;


String APIROOT = "http://hw.airgradient.com/";

int loopCount = 0;


void IRAM_ATTR isr() {
  debugln("pushed");
}

// select board LOLIN C3 mini to flash
void setup() {
    if (DEBUG) {
      Serial.begin(115200);
      // see https://github.com/espressif/arduino-esp32/issues/6983
      Serial.setTxTimeoutMs(0);   // <<<====== solves the delay issue
    }

    debug("starting ...");
    debug("Serial Number: "+ getNormalizedMac());

    // default hardware serial, PMS connector on the right side of the C3 mini on the Open Air
    Serial0.begin(9600);

    // second hardware serial, PMS connector on the left side of the C3 mini on the Open Air
    Serial1.begin(9600, SERIAL_8N1, 0, 1);

    // led
    pinMode(10, OUTPUT);

    // push button
    pinMode(9, INPUT_PULLUP);
    attachInterrupt(9, isr, FALLING);

    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    // give the PMSs some time to start
    countdown(3);

    connectToWifi();
    sendPing();
    switchLED(false);
}

void loop() {
  if(WiFi.status()== WL_CONNECTED) {
     if (pms1.readUntil(data1, 2000) && pms2.readUntil(data2, 2000)) {
        pm1Value01=pm1Value01+data1.PM_AE_UG_1_0;
        pm1Value25=pm1Value25+data1.PM_AE_UG_2_5;
        pm1Value10=pm1Value10+data1.PM_AE_UG_10_0;
        pm1PCount=pm1PCount+data1.PM_RAW_0_3;
        pm1temp=pm1temp+data1.AMB_TMP;
        pm1hum=pm1hum+data1.AMB_HUM;
        pm2Value01=pm2Value01+data2.PM_AE_UG_1_0;
        pm2Value25=pm2Value25+data2.PM_AE_UG_2_5;
        pm2Value10=pm2Value10+data2.PM_AE_UG_10_0;
        pm2PCount=pm2PCount+data2.PM_RAW_0_3;
        pm2temp=pm2temp+data2.AMB_TMP;
        pm2hum=pm2hum+data2.AMB_HUM;
        countPosition++;
        if (countPosition==targetCount) {
          pm1Value01 = pm1Value01 / targetCount;
          pm1Value25 = pm1Value25 / targetCount;
          pm1Value10 = pm1Value10 / targetCount;
          pm1PCount = pm1PCount / targetCount;
          pm1temp = pm1temp / targetCount;
          pm1hum = pm1hum / targetCount;
          pm2Value01 = pm2Value01 / targetCount;
          pm2Value25 = pm2Value25 / targetCount;
          pm2Value10 = pm2Value10 / targetCount;
          pm2PCount = pm2PCount / targetCount;
          pm2temp = pm2temp / targetCount;
          pm2hum = pm2hum / targetCount;
          postToServer(pm1Value01, pm1Value25,pm1Value10,pm1PCount, pm1temp,pm1hum,pm2Value01, pm2Value25,pm2Value10,pm2PCount, pm2temp,pm2hum);

          countPosition=0;
          pm1Value01=0;
          pm1Value25=0;
          pm1Value10=0;
          pm1PCount=0;
          pm1temp=0;
          pm1hum=0;
          pm2Value01=0;
          pm2Value25=0;
          pm2Value10=0;
          pm2PCount=0;
          pm2temp=0;
          pm2hum=0;
        }
     }

  }

countdown(2);
}

void debug(String msg) {
  if (DEBUG)
  Serial.print(msg);
}

void debug(int msg) {
  if (DEBUG)
  Serial.print(msg);
}

void debugln(String msg) {
  if (DEBUG)
  Serial.println(msg);
}

void debugln(int msg) {
  if (DEBUG)
  Serial.println(msg);
}

void switchLED(boolean ledON) {
 if (ledON) {
     digitalWrite(10, HIGH);
  } else {
    digitalWrite(10, LOW);
  }
}

void sendPing(){
      String payload = "{\"wifi\":" + String(WiFi.RSSI())
    + ", \"boot\":" + loopCount
    + "}";
    sendPayload(payload);
}

void postToServer(int pm1Value01, int pm1Value25, int pm1Value10, int pm1PCount, float pm1temp, float pm1hum,int pm2Value01, int pm2Value25, int pm2Value10, int pm2PCount, float pm2temp, float pm2hum) {
    String payload = "{\"wifi\":" + String(WiFi.RSSI())
    + ", \"pm01\":" + String((pm1Value01+pm2Value01)/2)
    + ", \"pm02\":" + String((pm1Value25+pm2Value25)/2)
    + ", \"pm10\":" + String((pm1Value10+pm2Value10)/2)
    + ", \"pm003_count\":" + String((pm1PCount+pm2PCount)/2)
    + ", \"atmp\":" + String((pm1temp+pm2temp)/20)
    + ", \"rhum\":" + String((pm1hum+pm2hum)/20)
    + ", \"boot\":" + loopCount
     + ", \"channels\": {"
        + "\"1\":{"
        + "\"pm01\":" + String(pm1Value01)
         + ", \"pm02\":" + String(pm1Value25)
         + ", \"pm10\":" + String(pm1Value10)
         + ", \"pm003_count\":" + String(pm1PCount)
         + ", \"atmp\":" + String(pm1temp/10)
         + ", \"rhum\":" + String(pm1hum/10)
         + "}"
         + ", \"2\":{"
         + " \"pm01\":" + String(pm1Value01)
         + ", \"pm02\":" + String(pm2Value25)
         + ", \"pm10\":" + String(pm2Value10)
         + ", \"pm003_count\":" + String(pm2PCount)
         + ", \"atmp\":" + String(pm2temp/10)
         + ", \"rhum\":" + String(pm2hum/10)
         + "}"
      + "}"
    + "}";
    sendPayload(payload);
}

void sendPayload(String payload) {
      if(WiFi.status()== WL_CONNECTED){
      switchLED(true);
      String url = APIROOT + "sensors/airgradient:" + getNormalizedMac() + "/measures";
      debugln(url);
      debugln(payload);
      client.setConnectTimeout(5 * 1000);
      client.begin(url);
      client.addHeader("content-type", "application/json");
      int httpCode = client.POST(payload);
      debugln(httpCode);
      client.end();
      resetWatchdog();
      switchLED(false);
    }
    else {
      debug("post skipped, not network connection");
    }
    loopCount++;
}

void countdown(int from) {
  debug("\n");
  while (from > 0) {
    debug(String(from--));
    debug(" ");
    delay(1000);
  }
  debug("\n");
}

void resetWatchdog() {
    digitalWrite(2, HIGH);
    delay(20);
    digitalWrite(2, LOW);
}

// Wifi Manager
 void connectToWifi() {
   WiFiManager wifiManager;
   switchLED(true);
   //WiFi.disconnect(); //to delete previous saved hotspot
   String HOTSPOT = "AG-" + String(getNormalizedMac());
   wifiManager.setTimeout(180);


   if (!wifiManager.autoConnect((const char * ) HOTSPOT.c_str())) {
    switchLED(false);
     Serial.println("failed to connect and hit timeout");
     delay(6000);
   }

}

String getNormalizedMac() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}
