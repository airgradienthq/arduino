/*
Important: This code is only for the DIY OUTDOOR OPEN AIR Presoldered Kit with the ESP-C3.

It is a high quality outdoor air quality sensor with dual PM2.5 modules and can send data over Wifi.

Kits are available: https://www.airgradient.com/open-airgradient/kits/

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.11-beta
“pms” by Markusz Kakl version 1.1.0 (needs to be patched for 5003T model)

For built instructions and how to patch the PMS library: https://www.airgradient.com/open-airgradient/instructions/diy-open-air-presoldered-v11/

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

PMS pms2(Serial1);
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

bool ledOn = false;

int loopCount = 0;

int averageCount = 10;

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

void IRAM_ATTR isr() {
  debugln("pushed");
}

void postToServer(String postfix, int pm25,float tem, float hum) {
    String payload = "{\"wifi\":" + String(WiFi.RSSI()) 
    + ", \"pm02\":" + String(pm25) 
    + ", \"atmp\":" + String(tem/10)
    + ", \"rhum\":" + String(hum/10)
    + ", \"boot\":" + loopCount 
    + "}";
    
    if(WiFi.status()== WL_CONNECTED){
      switchLED(true);
      String url = APIROOT + "sensors/airgradient:" + getNormalizedMac() + postfix + "/measures";
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


// select board LOLIN C3 mini to flash
void setup() {
    if (DEBUG) {
      Serial.begin(115200);
      // see https://github.com/espressif/arduino-esp32/issues/6983
      Serial.setTxTimeoutMs(0);   // <<<====== solves the delay issue
    }
    
    debug("starting ...");

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
    switchLED(false);
}

void resetWatchdog() {
    digitalWrite(2, HIGH); 
    delay(20);
    digitalWrite(2, LOW);
}

void loop() {

  if(WiFi.status()== WL_CONNECTED) {
       
    if (pms1.readUntil(data1, 2000)) {
      pm1Value=pm1Value+data1.PM_AE_UG_2_5;
      temp_pm1=temp_pm1+data1.AMB_TMP;
      hum_pm1=hum_pm1+data1.AMB_HUM;
      debugln("PMS 1 measurement "+String(pm1Position)+": "+String(data1.PM_AE_UG_2_5)+"ug/m3");
      pm1Position++;
        if (pm1Position==averageCount) {
          pm1Value = pm1Value / averageCount;
          temp_pm1 = temp_pm1 / averageCount;
          hum_pm1 = hum_pm1 / averageCount;
          postToServer("-1", pm1Value, temp_pm1,hum_pm1);
          pm1Position=0;
          pm1Value=0;
        }    
    } else {
       debugln("PMS 1 does't respond");
    }

    if (pms2.readUntil(data2, 2000)) {
      pm2Value=pm2Value+data2.PM_AE_UG_2_5;
      temp_pm2=temp_pm2+data2.AMB_TMP;
      hum_pm2=hum_pm2+data2.AMB_HUM;
      debugln("PMS 2 measurement "+String(pm2Position)+": "+String(data2.PM_AE_UG_2_5)+"ug/m3");
      pm2Position++;
        if (pm2Position==averageCount) {
          pm2Value = pm2Value / averageCount;
          temp_pm2 = temp_pm2 / averageCount;
          hum_pm2 = hum_pm2 / averageCount;
          postToServer("-2", pm2Value, temp_pm2, hum_pm2);
          pm2Position=0;
          pm2Value=0;
        }    
    } else {
       debugln("PMS 2 does't respond");
    }

  }

countdown(2);
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
