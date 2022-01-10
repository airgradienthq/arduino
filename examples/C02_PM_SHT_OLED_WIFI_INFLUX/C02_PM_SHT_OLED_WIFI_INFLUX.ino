/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

For build instructions please visit https://www.airgradient.com/diy/

Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)
SenseAir S8 (CO2 Sensor)
SHT30/31 (Temperature/Humidity Sensor)

Please install ESP8266 board manager (tested with version 3.0.0)

The codes needs the following libraries installed:
"WifiManager by tzapu, tablatronix" tested with Version 2.0.3-alpha
"ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse, Fabrice Weinberg" tested with Version 4.1.0

Configuration:
Please set in the code below which sensor you are using and if you want to connect it to WiFi.

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/schools/

MIT License
*/

#include <AirGradient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <InfluxDbClient.h>

#include "SSD1306Wire.h"

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

#define DISPLAY_DELAY 3000
// Number of times to run full loop before resending to influx
#define INFLUX_SEND_PERIOD 37
static unsigned long current_loop = INFLUX_SEND_PERIOD;  // initialize such that a reading is due the first time through loop()


// set sensors that you do not use to false
const boolean hasPM=true;
const boolean hasCO2=true;
const boolean hasSHT=true;
const float shtTempOffset=-2.4;

// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI=true;

// InfluxDB server url, e.g. http://192.168.1.48:8086 (don't use localhost, always server name or ip address)
#define INFLUXDB_URL "hostname"
// InfluxDB database name
#define INFLUXDB_DB_NAME "db_name"
#define INFLUXDB_USER "db_user"
#define INFLUXDB_PASSWORD "db_password"
#define HOSTNAME "air_quality"

// Single InfluxDB instance
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);

void setup(){
  Serial.begin(9600);

  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(),HEX),true);

  if (hasPM) ag.PMS_Init();
  if (hasCO2) ag.CO2_Init();
  if (hasSHT) ag.TMP_RH_Init(0x44);

  if (connectWIFI) connectToWifi();
  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);
  delay(2000);
}

void loop(){
  boolean shouldPushToInflux = false;
  current_loop += 1;
  Serial.print("Current loop: ");
  Serial.println(current_loop);
  if (current_loop >= INFLUX_SEND_PERIOD) {
    current_loop = 0;
    shouldPushToInflux = true;
    Serial.println("Reset to 0");
  }

  if (hasPM) {
    const int PM2 = ag.getPM2_Raw();
    if (shouldPushToInflux) write_data_int_point("particulate_matter_2", PM2);

    showTextRectangle("PM2", String(PM2), false);
    delay(DISPLAY_DELAY);
  }

  if (hasCO2) {
    const int CO2 = ag.getCO2_Raw();
    if (shouldPushToInflux) write_data_int_point("carbon_dioxide", CO2);

    showTextRectangle("CO2", String(CO2), false);
    delay(DISPLAY_DELAY);
  }

  if (hasSHT) {
    TMP_RH result = ag.periodicFetchData();
    if (shouldPushToInflux) write_data_float_point("temperature", result.t + shtTempOffset);
    if (shouldPushToInflux) write_data_float_point("humidity", result.rh);

    showTextRectangle(String(result.t + shtTempOffset), String(result.rh)+"%", false);
    delay(DISPLAY_DELAY);
  }
}

void write_data_int_point(const String name, const int value) {
  if (connectWIFI){
    Point pointDevice = Point(name);
    pointDevice.addTag("host", HOSTNAME);
    pointDevice.addField("value", value);
    // Print what are we exactly writing
    Serial.print("Writing: ");
    Serial.println(client.pointToLineProtocol(pointDevice));

    // Write point
    if (!client.writePoint(pointDevice)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }

    // Check server connection
    if (client.validateConnection()) {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(client.getServerUrl());
    } else {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(client.getLastErrorMessage());
    }
  }
}

void write_data_float_point(const String name, const float value) {
  if (connectWIFI){
    Point pointDevice = Point(name);
    pointDevice.addTag("host", HOSTNAME);
    pointDevice.addField("value", value);
    // Print what are we exactly writing
    Serial.print("Writing: ");
    Serial.println(client.pointToLineProtocol(pointDevice));

    // Write point
    if (!client.writePoint(pointDevice)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    }

    // Check server connection
    if (client.validateConnection()) {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(client.getServerUrl());
    } else {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(client.getLastErrorMessage());
    }
  }
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
  display.drawString(32, 16, ln1);
  display.drawString(32, 36, ln2);
  display.display();
}

// Wifi Manager
void connectToWifi(){
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-" + String(ESP.getChipId(), HEX);

  wifiManager.setTimeout(120);

  if(!wifiManager.autoConnect((const char*)HOTSPOT.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }
}
