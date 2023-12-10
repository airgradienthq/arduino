/*
Important: This code is only for the AirGradient ONE Open Air Version with TVOC and CO2 sensor.

It is a high quality sensor measuring PM2.5, CO2, TVOC, NOx, Temperature and Humidity and can send data over Wifi.

Build Instructions: https://www.airgradient.com/open-airgradient/instructions/

Kits (including a pre-soldered version) are available: https://www.airgradient.com/

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.11-beta
"Sensirion I2C SGP41" by Sensation Version 0.1.0
"Sensirion Gas Index Algorithm" by Sensation Version 3.2.1
"Arduino-SHT" by Johannes Winkelmann Version 1.2.2
“pms” by Markusz Kakl version 1.1.0 (needs to be patched for 5003T model)

For built instructions and how to patch the PMS library: https://www.airgradient.com/open-airgradient/instructions/diy-open-air-presoldered-v11/

If you have any questions please visit our forum at https://forum.airgradient.com/

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include "PMS.h"
#include <HardwareSerial.h>
#include <Wire.h>
#include "s8_uart.h"
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <SensirionI2CSgp41.h>
#include <NOxGasIndexAlgorithm.h>
#include <VOCGasIndexAlgorithm.h>

#define DEBUG true

#define I2C_SDA 7
#define I2C_SCL 6

HTTPClient client;

SensirionI2CSgp41 sgp41;
VOCGasIndexAlgorithm voc_algorithm;
NOxGasIndexAlgorithm nox_algorithm;

PMS pms1(Serial0);

PMS::DATA data1;

S8_UART * sensor_S8;
S8_sensor sensor;

// time in seconds needed for NOx conditioning
uint16_t conditioning_s = 10;

String APIROOT = "http://hw.airgradient.com/";

// set to true to switch from Celcius to Fahrenheit
//boolean inF = false;

// PM2.5 in US AQI (default ug/m3)
//boolean inUSAQI = false;

// Display Position
//boolean displayTop = true;

// use RGB LED Bar
//boolean useRGBledBar = true;

// set to true if you want to connect to wifi. You have 60 seconds to connect. Then it will go into an offline mode.
boolean connectWIFI = true;

int loopCount = 0;

unsigned long currentMillis = 0;

const int oledInterval = 5000;
unsigned long previousOled = 0;

const int sendToServerInterval = 10000;
unsigned long previoussendToServer = 0;

const int tvocInterval = 1000;
unsigned long previousTVOC = 0;
int TVOC = -1;
int NOX = -1;

const int co2Interval = 5000;
unsigned long previousCo2 = 0;
int Co2 = 0;

const int pmInterval = 5000;
unsigned long previousPm = 0;
int pm25 = -1;
int pm01 = -1;
int pm10 = -1;
int pm03PCount = -1;
float temp;
int hum;

//const int tempHumInterval = 2500;
//unsigned long previousTempHum = 0;



void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    // see https://github.com/espressif/arduino-esp32/issues/6983
    Serial.setTxTimeoutMs(0); // <<<====== solves the delay issue
  }

  Wire.begin(I2C_SDA, I2C_SCL);

  Serial1.begin(9600, SERIAL_8N1, 0, 1);
  Serial0.begin(9600);

  sgp41.begin(Wire);

  //init Watchdog
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  sensor_S8 = new S8_UART(Serial1);

  delay(500);

  // push button
  pinMode(9, INPUT_PULLUP);

  countdown(3);

  if (connectWIFI) {
    WiFi.begin("airgradient", "cleanair");
    int retries = 0;
      while ((WiFi.status() != WL_CONNECTED) && (retries < 15)) {
        retries++;
        delay(500);
        Serial.print(".");
      }
if (retries > 14) {
    Serial.println(F("WiFi connection FAILED"));
    connectToWifi();
}
if (WiFi.status() == WL_CONNECTED) {
    sendPing();
    Serial.println(F("WiFi connected!"));
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}
  }

}

void loop() {
  currentMillis = millis();
  updateTVOC();
  updateCo2();
  updatePm();
  sendToServer();
}

void updateTVOC() {
  uint16_t error;
  char errorMessage[256];
  uint16_t defaultRh = 0x8000;
  uint16_t defaultT = 0x6666;
  uint16_t srawVoc = 0;
  uint16_t srawNox = 0;
  uint16_t defaultCompenstaionRh = 0x8000; // in ticks as defined by SGP41
  uint16_t defaultCompenstaionT = 0x6666; // in ticks as defined by SGP41
  uint16_t compensationRh = 0; // in ticks as defined by SGP41
  uint16_t compensationT = 0; // in ticks as defined by SGP41

  delay(1000);

  compensationT = static_cast < uint16_t > ((temp + 45) * 65535 / 175);
  compensationRh = static_cast < uint16_t > (hum * 65535 / 100);

  if (conditioning_s > 0) {
    error = sgp41.executeConditioning(compensationRh, compensationT, srawVoc);
    conditioning_s--;
  } else {
    error = sgp41.measureRawSignals(compensationRh, compensationT, srawVoc,
      srawNox);
  }

  if (currentMillis - previousTVOC >= tvocInterval) {
previousTVOC = currentMillis;
if (error) {
   TVOC = -1;
    NOX = -1;
    //Serial.println(String(TVOC));
} else {
   TVOC = voc_algorithm.process(srawVoc);
    NOX = nox_algorithm.process(srawNox);
    //Serial.println(String(TVOC));
}
  }
}

void updateCo2() {
  if (currentMillis - previousCo2 >= co2Interval) {
    previousCo2 = currentMillis;
    Co2 = sensor_S8 -> get_co2();
    //Serial.println(String(Co2));
  }
}

void updatePm() {
  if (currentMillis - previousPm >= pmInterval) {
    previousPm = currentMillis;
    if (pms1.readUntil(data1, 2000)) {
      pm01 = data1.PM_AE_UG_1_0;
      pm25 = data1.PM_AE_UG_2_5;
      pm10 = data1.PM_AE_UG_10_0;
      pm03PCount = data1.PM_RAW_0_3;
      temp = data1.AMB_TMP;
      hum = data1.AMB_HUM;
    } else {
      pm01 = -1;
      pm25 = -1;
      pm10 = -1;
      pm03PCount = -1;
      temp = -10001;
      hum = -10001;
    }
  }
}


void sendPing() {
  String payload = "{\"wifi\":" + String(WiFi.RSSI()) +
    ", \"boot\":" + loopCount +
    "}";
}


void sendToServer() {
  if (currentMillis - previoussendToServer >= sendToServerInterval) {
    previoussendToServer = currentMillis;
    String payload = "{\"wifi\":" + String(WiFi.RSSI()) +
      (Co2 < 0 ? "" : ", \"rco2\":" + String(Co2)) +
      (pm01 < 0 ? "" : ", \"pm01\":" + String(pm01)) +
      (pm25 < 0 ? "" : ", \"pm02\":" + String(pm25)) +
      (pm10 < 0 ? "" : ", \"pm10\":" + String(pm10)) +
      (pm03PCount < 0 ? "" : ", \"pm003_count\":" + String(pm03PCount)) +
      (TVOC < 0 ? "" : ", \"tvoc_index\":" + String(TVOC)) +
      (NOX < 0 ? "" : ", \"nox_index\":" + String(NOX)) +
      ", \"atmp\":" + String(temp/10) +
      (hum < 0 ? "" : ", \"rhum\":" + String(hum/10)) +
      ", \"boot\":" + loopCount +
      "}";

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(payload);
      String POSTURL = APIROOT + "sensors/airgradient:" + String(getNormalizedMac()) + "/measures";
      Serial.println(POSTURL);
      WiFiClient client;
      HTTPClient http;
      http.begin(client, POSTURL);
      http.addHeader("content-type", "application/json");
      int httpCode = http.POST(payload);
      String response = http.getString();
      Serial.println(httpCode);
      //Serial.println(response);
      http.end();
      resetWatchdog();
      loopCount++;
    } else {
      Serial.println("WiFi Disconnected");
    }
  }
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
  Serial.println("Watchdog reset");
  digitalWrite(2, HIGH);
  delay(20);
  digitalWrite(2, LOW);
}

// Wifi Manager
void connectToWifi() {
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AG-" + String(getNormalizedMac());
  wifiManager.setTimeout(180);
  if (!wifiManager.autoConnect((const char * ) HOTSPOT.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(6000);
  }

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

String getNormalizedMac() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}





