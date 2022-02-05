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

If you have any questions please visit our forum at https://forum.airgradient.com/

Configuration:
Please set in the code below which sensor you are using and if you want to connect it to WiFi.
You can also switch PM2.5 from ug/m3 to US AQI and Celcius to Fahrenheit

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/schools/

Kits with all required components are available at https://www.airgradient.com/diyshop/

MIT License
*/

#include "Metrics/MetricGatherer.h"
#include "Sensors/Particle/PMSXSensor.h"
#include "Sensors/Temperature/SHTXSensor.h"
#include "Sensors/CO2/SensairS8Sensor.h"
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include "SSD1306Wire.h"
#include "AQI/AQICalculator.h"

using namespace AirGradient;


SSD1306Wire display(0x3c, SDA, SCL);

//screen refresh
const uint16_t screenUpdateFrequencyMs = 5000;

// set sensors that you do not use to false
boolean hasPM = true;
boolean hasCO2 = true;
boolean hasSHT = true;

// set to true to switch PM2.5 from ug/m3 to US AQI
boolean inUSaqi = false;

// set to true to switch from Celcius to Fahrenheit
boolean inF = false;

// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI = false;

// change if you want to send the data to another server
String APIROOT = "http://hw.airgradient.com/";

uint8_t counter = 0;

auto metrics = std::make_shared<MetricGatherer>();
auto aqiCalculator = std::make_unique<AQICalculator>(metrics);
Ticker updateScreenTicker;
Ticker sendPayloadTicker;

void setup() {
    Serial.begin(9600);

    display.init();
    display.flipScreenVertically();
    showTextRectangle("Init", String(ESP.getChipId(), HEX), true);

    if (hasPM) metrics->addSensor(std::make_unique<PMSXSensor>());
    if (hasCO2) metrics->addSensor(std::make_unique<SensairS8Sensor>());
    if (hasSHT) metrics->addSensor(std::make_unique<SHTXSensor>());

    if (connectWIFI) connectToWifi();

    metrics->begin();
    aqiCalculator->begin();
    updateScreenTicker.attach_ms_scheduled(screenUpdateFrequencyMs, updateScreen);

    if (connectWIFI) {
        sendPayloadTicker.attach_scheduled(20, sendPayload);
    }

}

void loop() {


}

void sendPayload() {
    auto data = metrics->getData();
    auto sensorType = metrics->getMeasurements();

    // create payload
    String payload = "{\"wifi\":" + String(WiFi.RSSI()) + ",";

    //Check for particle sensor
    if (!(sensorType & Measurement::Particle)) {
        auto PM2 = data.PARTICLE_DATA.PM_2_5;
        payload = payload + "\"pm02\":" + String(PM2);
    }

    //Check for CO2 sensor
    if (!(sensorType & Measurement::CO2)) {
        if (!payload.endsWith(",")) payload = payload + ",";
        auto CO2 = data.GAS_DATA.CO2;
        payload = payload + "\"rco2\":" + String(CO2);
    }

    if (!(sensorType & Measurement::Temperature) {
        if (!payload.endsWith(",")) payload = payload + ",";
        payload = payload + "\"atmp\":" + String(data.TMP);
    }

    if (!(sensorType & Measurement::Humidity) {
        if (!payload.endsWith(",")) payload = payload + ",";
        payload = payload + "\"rhum\":" + String(data.HUM);
    }

    payload = payload + "}";
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

// DISPLAY
void showTextRectangle(const String &ln1, const String &ln2, boolean small) {
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

void updateScreen() {
    auto data = metrics->getData();
    auto sensorType = metrics->getMeasurements();
    // Take a measurement at a fixed interval.
    switch (counter) {

        case 0:
            if (!(sensorType & Measurement::Particle)) {
                showTextRectangle("PM2", String(data.PARTICLE_DATA.PM_2_5), false);
                break;
            }
        case 1:
            if (!(sensorType & Measurement::CO2)) {
                showTextRectangle("CO2", String(data.GAS_DATA.CO2), false);
                break;
            }

        case 2:
            if (!(sensorType & Measurement::Temperature)) {
                if (inF) {
                    showTextRectangle("TMP", String((data.TMP * 1.8) + 32, 1) + "F", false);
                    break;
                }
                showTextRectangle("TMP", String(data.TMP, 1) + "C", false);
                break;
            }

        case 3:
            if (!(sensorType & Measurement::Humidity)) {
                showTextRectangle("HUM", String(data.HUM, 1) + "%", false);
                break;
            }

        case 4:
            if (!(sensorType & Measurement::Particle)) {
                auto aqi = aqiCalculator->isAQIAvailable() ? String(aqiCalculator->getAQI(), 1) : "N/A";
                showTextRectangle("AQI", aqi, false);
                break;
            }
    }

    counter = ++counter % 5;
}

// Wifi Manager
void connectToWifi() {
    WiFiManager wifiManager;
    //WiFi.disconnect(); //to delete previous saved hotspot
    String HOTSPOT = "AIRGRADIENT-" + String(ESP.getChipId(), HEX);
    wifiManager.setTimeout(120);
    if (!wifiManager.autoConnect((const char *) HOTSPOT.c_str())) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        ESP.restart();
        delay(5000);
    }

}