/**
 * This sketch connects an AirGradient DIY sensor to a WiFi network, and runs a
 * tiny HTTP server to serve air quality metrics to Prometheus.
 */

#include "main.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <Wire.h>
#include "SSD1306Wire.h"
#include "Metrics/MetricGatherer.h"
#include "Sensors/Particle/PMSXSensor.h"
#include "Sensors/Temperature/SHTXSensor.h"
#include "Sensors/CO2/SensairS8Sensor.h"
#include "Sensors/Time/BootTimeSensor.h"
#include "AQI/AQICalculator.h"
#include "Prometheus/PrometheusServer.h"

using namespace AirGradient;

// Config ----------------------------------------------------------------------

//screen refresh
const int screenUpdateFrequencyMs = 5000;

//Id of the device for Prometheus
const char * deviceId = "";

//Wifi information
const char* ssid = "";
const char* password = "";
const uint16_t port = 9925;

#ifdef staticip
IPAddress static_ip(192, 168, 42, 20);
IPAddress gateway(192, 168, 42, 1);
IPAddress subnet(255, 255, 255, 0);
#endif

const char* ntp_server = "pool.ntp.org";

// For housekeeping.
uint8_t counter = 0;

// Config End ------------------------------------------------------------------

SSD1306Wire display(0x3c, SDA, SCL);

auto metrics = std::make_shared<MetricGatherer>();
auto aqiCalculator = std::make_shared<AQICalculator>(metrics);
auto server = std::make_unique<PrometheusServer>(port, deviceId, metrics, aqiCalculator);
Ticker updateScreenTicker;


void setup() {
    Serial.begin(9600);

    metrics->addSensor(std::make_unique<PMSXSensor>())
            .addSensor(std::make_unique<SHTXSensor>())
            .addSensor(std::make_unique<SensairS8Sensor>())
            .addSensor(std::make_unique<BootTimeSensor>(ntp_server));

    // Init Display.
    display.init();
    display.flipScreenVertically();
    showTextRectangle("Init", String(EspClass::getChipId(), HEX), true);

    // Set static IP address if configured.
#ifdef staticip
    WiFi.config(static_ip, gateway, subnet);
#endif

    // Set WiFi mode to client (without this it may try to act as an AP).
    WiFi.mode(WIFI_STA);

    // Configure Hostname
    if ((deviceId != NULL) && (deviceId[0] == '\0')) {
        Serial.printf("No Device ID is Defined, Defaulting to board defaults");
    } else {
        wifi_station_set_hostname(deviceId);
        WiFi.setHostname(deviceId);
    }

    // Setup and wait for WiFi.
    WiFi.begin(ssid, password);
    Serial.println("");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        showTextRectangle("Trying to", "connect...", true);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Hostname: ");
    Serial.println(WiFi.hostname());

    metrics->begin();

    server->begin();

    showTextRectangle("Listening To", WiFi.localIP().toString() + ":" + String(port), true);
    updateScreenTicker.attach_ms_scheduled(screenUpdateFrequencyMs, updateScreen);
}

void loop() {
    server->handleRequests();
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
    auto sensorType = metrics->getSensorTypes();
    // Take a measurement at a fixed interval.
    switch (counter) {

        case 0:
            if (!(sensorType & SensorType::Particle)) {
                showTextRectangle("PM2", String(data.PARTICLE_DATA.PM_2_5), false);
                break;
            }

        case 1:
            if (!(sensorType & SensorType::CO2)) {
                showTextRectangle("CO2", String(data.GAS_DATA.CO2), false);
                break;
            }

        case 2:
            if (!(sensorType & SensorType::Temperature)) {
                showTextRectangle("TMP", String(data.TMP, 1) + "C", false);
                break;
            }

        case 3:
            if (!(sensorType & SensorType::Humidity)) {
                showTextRectangle("HUM", String(data.HUM, 1) + "%", false);
                break;
            }

        case 4:
            if (!(sensorType & SensorType::Particle)) {
                auto aqi = aqiCalculator->isAQIAvailable() ? String(aqiCalculator->getAQI(), 1) : "N/A";
                showTextRectangle("AQI", aqi, false);
                break;
            }

    }

    counter = ++counter % 5;
}