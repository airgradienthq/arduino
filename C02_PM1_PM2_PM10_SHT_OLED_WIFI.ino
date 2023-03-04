/**
 * This sketch connects an AirGradient DIY sensor to a WiFi network, and runs a
 * tiny HTTP server to serve air quality metrics to Prometheus.
 */

/*
  PM1 and PM10 reporting for Plantower PMS5003 PM2.5 sensor enabled.
  Workaround for glitchy CO2 and PM sensors reporting included.
  For using this .ino you have to install improved AirGradient libraries, which supports PM1 and PM10 reporting: 
  https://github.com/d3vilh/airgradient-improved
*/


#include <AirGradient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>

#include <Wire.h>
#include "SSD1306Wire.h"

AirGradient ag = AirGradient();

// Config ----------------------------------------------------------------------

// Optional.
const char* deviceId = "livingroom";

// set to 'F' to switch display from Celcius to Fahrenheit
char temp_display = 'C';

// Hardware options for AirGradient DIY sensor.
const bool hasPM1 = true;
const bool hasPM2 = true;
const bool hasPM10 = true;
const bool hasCO2 = true;
const bool hasSHT = true;

// WiFi and IP connection info.
const char* ssid = "CY-Capsule";
const char* password = "GagaZush";
const int port = 9926;

// Uncomment the line below to configure a static IP address.
// #define staticip
#ifdef staticip
IPAddress static_ip(192, 168, 88, 6);
IPAddress gateway(192, 168, 88, 1);
IPAddress subnet(255, 255, 255, 0);
#endif

// The frequency of measurement updates.
const int updateFrequency = 5000;

// For housekeeping.
long lastUpdate;
int counter = 0;
int stat_prev_pm1 = 0;
int stat_prev_pm2 = 0;
int stat_prev_pm10 = 0;
int stat_prev_co = 0;


// Config End ------------------------------------------------------------------

SSD1306Wire display(0x3c, SDA, SCL);
ESP8266WebServer server(port);

void setup() {
  Serial.begin(9600);

  // Init Display.
  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(),HEX),true);

  // Enable enabled sensors.
  if (hasPM1) ag.PMS_Init();
  if (hasCO2) ag.CO2_Init();
  if (hasSHT) ag.TMP_RH_Init(0x44);

  // Set static IP address if configured.
  #ifdef staticip
  WiFi.config(static_ip,gateway,subnet);
  #endif

  // Set WiFi mode to client (without this it may try to act as an AP).
  WiFi.mode(WIFI_STA);
  
  // Configure Hostname
  if ((deviceId != NULL) && (deviceId[0] == '\0')) {
    Serial.printf("No Device ID is Defined, Defaulting to board defaults");
  }
  else {
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
  server.on("/", HandleRoot);
  server.on("/metrics", HandleRoot);
  server.onNotFound(HandleNotFound);

  server.begin();
  Serial.println("HTTP server started at ip " + WiFi.localIP().toString() + ":" + String(port));
  showTextRectangle("Listening To", WiFi.localIP().toString() + ":" + String(port),true);
}

void loop() {
  long t = millis();

  server.handleClient();
  updateScreen(t);
}

String GenerateMetrics() {
  String message = "";
  String idString = "{id=\"" + String(deviceId) + "\",mac=\"" + WiFi.macAddress().c_str() + "\"}";

  if (hasPM1) {
    int statf_pm1 = 0;
    int stat = ag.getPM1_Raw();
       if (stat > 0 && stat <= 10000) {
         statf_pm1 = stat;
         stat_prev_pm1 = statf_pm1; // saving not glitchy value
       } else {
         statf_pm1 = stat_prev_pm1; // using previous not glitchy value if curent value is glitchy
       }
    message += "# HELP pm01 Particulate Matter PM1 value\n";
    message += "# TYPE pm01 gauge\n";
    message += "pm01";
    message += idString;
    message += String(statf_pm1);
    message += "\n";
  }

  if (hasPM2) {
    int statf_pm2 = 0;
    int stat = ag.getPM2_Raw();
       if (stat > 0 && stat <= 10000) {
         statf_pm2 = stat;
         stat_prev_pm2 = statf_pm2; // saving not glitchy value
       } else {
         statf_pm2 = stat_prev_pm2; // using previous not glitchy value if curent value is glitchy
       }
    message += "# HELP pm02 Particulate Matter PM2.5 value\n";
    message += "# TYPE pm02 gauge\n";
    message += "pm02";
    message += idString;
    message += String(statf_pm2);
    message += "\n";
  }

  if (hasPM10) {
    int statf_pm10 = 0;
    int stat = ag.getPM10_Raw();
       if (stat > 0 && stat <= 10000) {
         statf_pm10 = stat;
         stat_prev_pm10 = statf_pm10; // saving not glitchy value
       } else {
         statf_pm10 = stat_prev_pm10; // using previous not glitchy value if curent value is glitchy
       }
    message += "# HELP pm10 Particulate Matter PM10 value\n";
    message += "# TYPE pm10 gauge\n";
    message += "pm10";
    message += idString;
    message += String(statf_pm10);
    message += "\n";
  }

  if (hasCO2) {
    int statf_co = 0;
    int stat = ag.getCO2_Raw();
       if (stat >= 0 && stat <= 10000) {
         statf_co = stat;
         stat_prev_co = statf_co; // saving not glitchy value
       } else {
         statf_co = stat_prev_co; // using previous not glitchy value if curent value is glitchy
       }
    message += "# HELP rco2 CO2 value, in ppm\n";
    message += "# TYPE rco2 gauge\n";
    message += "rco2";
    message += idString;
    message += String(statf_co);
    message += "\n";
  }

  if (hasSHT) {
    TMP_RH stat = ag.periodicFetchData();
    message += "# HELP atmp Temperature, in degrees Celsius\n";
    message += "# TYPE atmp gauge\n";
    message += "atmp";
    message += idString;
    // Dirty Temp adjust (-4 degrees)
    message += String(stat.t - 4);
    message += "\n";

    message += "# HELP rhum Relative humidity, in percent\n";
    message += "# TYPE rhum gauge\n";
    message += "rhum";
    message += idString;
    message += String(stat.rh);
    message += "\n";
  }

  return message;
  delay(5000);
}

void HandleRoot() {
  server.send(200, "text/plain", GenerateMetrics() );
}

void HandleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/html", message);
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

void updateScreen(long now) {
  if ((now - lastUpdate) > updateFrequency) {
    // Take a measurement at a fixed interval.
    switch (counter) {
      case 0:
        if (hasPM1) {
          int statf_pm1 = 0;
          int stat = ag.getPM1_Raw();
          if (stat > 0 && stat <= 10000) {
           statf_pm1 = stat;
           stat_prev_pm1 = statf_pm1; // saving not glitchy value
          } else {
            statf_pm1 = stat_prev_pm1; // using previous not glitchy value if curent value is glitchy
          }
          showTextRectangle("PM1",String(statf_pm1),false);
        }
        break;
      case 1:
        if (hasPM2) {
          int statf_pm2 = 0;
          int stat = ag.getPM2_Raw();
          if (stat > 0 && stat <= 10000) {
           statf_pm2 = stat;
           stat_prev_pm2 = statf_pm2; // saving not glitchy value
          } else {
            statf_pm2 = stat_prev_pm2; // using previous not glitchy value if curent value is glitchy
          }
          showTextRectangle("PM2.5",String(statf_pm2),false);
        }
        break;
      case 2:
        if (hasPM10) {
          int statf_pm10 = 0;
          int stat = ag.getPM10_Raw();
          if (stat > 0 && stat <= 10000) {
           statf_pm10 = stat;
           stat_prev_pm10 = statf_pm10; // saving not glitchy value
          } else {
            statf_pm10 = stat_prev_pm10; // using previous not glitchy value if curent value is glitchy
          }
          showTextRectangle("PM10",String(statf_pm10),false);
        }
        break;
      case 3:
        if (hasCO2) {
          int statf_co = 0;
          int stat = ag.getCO2_Raw();
          if (stat >= 0 && stat <= 10000) {
           statf_co = stat;
           stat_prev_co = statf_co; // saving not glitchy value
          } else {
            statf_co = stat_prev_co; // using previous not glitchy value if curent value is glitchy
          }
          showTextRectangle("CO2", String(statf_co), false);
        }
        break;
      case 4:
        if (hasSHT) {
          TMP_RH stat = ag.periodicFetchData();
          if (temp_display == 'F' || temp_display == 'f') {
            showTextRectangle("TMP", String((stat.t * 9 / 5) + 32, 1) + "F", false);
          } else {
            showTextRectangle("TMP", String(stat.t - 4, 1) + "C", false); // dirty temp adjust
          }
        }
        break;
      case 5:
        if (hasSHT) {
          TMP_RH stat = ag.periodicFetchData();
          showTextRectangle("HUM", String(stat.rh) + "%", false);
        }
        break;
    }
    counter++;
    if (counter > 5) counter = 0;
    lastUpdate = millis();
    delay(5000);
  }
}
