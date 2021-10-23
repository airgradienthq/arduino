/*

  The MIT License (MIT)

  Portions Copyright (c) 2020-2021 AirGradient Co. Ltd. (https://www.airgradient.com/diy/)
  Portions Copyright (c) 2021 Jordan Jones (https://github.com/kashalls/)
  Portions Copyright (c) 2021 Bradan J. Wolbeck (https://www.compaqdisc.com/)

  For more information, please see LICENSE.md

*/

//
// Includes
#include <AirGradient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>

#include <Wire.h>
#include "SSD1306Wire.h"

//
// Config Start

// Change to true if you would like to recieve sensor data regardless of the error state. (Not Reccomended)
const bool ignore_metric_errors = true;

const char* deviceId = "AirGradient";

const bool hasPM = true;
const bool hasCO2 = true;
const bool hasSHT = true;

const char* ssid = "PleaseChangeMe";
const char* password = "PleaseChangeMe";

const int port = 9926;

long lastUpdate;

//The frequency of measurement updates
const int updateFrequency = 5000; 

int counter = 0;

// Config End
//
AirGradient ag = AirGradient();
SSD1306Wire display(0x3c, SDA, SCL);
ESP8266WebServer server(port);
//

/* Global vars */
char pm2_reading[12];
char co2_reading[12];
char atmp_reading[12];
char rh_reading[12];
char wifi_dbm_reading[12];

/* Representation of a metric response */
typedef struct metric_resp {
  const char* message;
  bool is_error;
} metric_resp_t;

/* Metric fetch function type */
typedef metric_resp_t (*metric_fetch_t)(void);

/* Valid metric types */
typedef enum metric_type {
  METRIC_TYPE_COUNTER,
  METRIC_TYPE_GUAGE,
  METRIC_TYPE_HISTOGRAM,
  METRIC_TYPE_SUMMARY,
  METRIC_TYPE_UNTYPED
} metric_type_t;

/* Strings for those metric types */
const char* METRIC_TYPES[] = {
  "counter",
  "guage",
  "histogram",
  "summary",
  "untyped"
};
/**
 * Note: Since only the `guage` type is used, these could be dropped.
 * But to keep things flexible, I've elected to leave them in.
 * /

/* Representation of a single metric */
typedef struct metric {
  metric_type_t type;
  char* id;
  char* desc;
  metric_fetch_t fetch;
} metric_t;

void setup() {
  Serial.begin(9600);

  // Init Display.
  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(),HEX),true);

  // Enable enabled sensors.
  if (hasPM) ag.PMS_Init();
  if (hasCO2) ag.CO2_Init();
  if (hasSHT) ag.TMP_RH_Init(0x44);

  // Setup and wait for wifi.
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Trying to connect to the network...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    showTextRectangle(ssid, "Connecting", true);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());

  server.on("/", HandleRoot);
  server.on("/metrics", HandleRoot);
  server.onNotFound(HandleNotFound);

  showTextRectangle("Listening To", WiFi.localIP().toString() + ":" + String(port),true);
  delay(2000);

  server.begin();
  Serial.println("HTTP server started at ip " + WiFi.localIP().toString() + ":" + String(port));
}

void loop() {
  long t = millis();

  server.handleClient();
  updateScreen(t);
}

metric_resp_t get_pm2() {
  // Early return if the sensor isn't enabled.
  if (!hasPM) return { "(disabled)", true };
  int val = ag.getPM2_Raw();
  itoa(val, pm2_reading, 10);
  Serial.print("PM2: ");
  Serial.println(val);
  return { pm2_reading, val <= 0 };
}

metric_resp_t get_co2() {
  // Early return if the sensor isn't enabled.
  if (!hasCO2) return { "(disabled)", true };

  int val = ag.getCO2_Raw();
  itoa(val, co2_reading, 10);
  Serial.print("CO2: ");
  Serial.println(val);
  return { co2_reading, val == -1 };
}

metric_resp_t get_atmp() {
  // Early return if the sensor isn't enabled.
  if (!hasSHT) return { "(disabled)", true };

  TMP_RH stat = ag.periodicFetchData();
  dtostrf(stat.t, 4, 2, atmp_reading);
  itoa(stat.rh, rh_reading, 10);
  Serial.print("T: ");
  Serial.println(stat.t);
  Serial.print("RH: ");
  Serial.print(stat.rh);
  return { atmp_reading, stat.error != 0 };
}

metric_resp_t get_rhum() {
  // Early return if the sensor isn't enabled.
  if (!hasSHT) return { "(disabled)", true };

  //  TMP_RH stat = ag.periodicFetchData();
  // itoa(stat.rh, rh_reading, 10);
  return { rh_reading, true };
}

metric_resp_t get_wifi_dbm() {
  ltoa(WiFi.RSSI(), wifi_dbm_reading, 10);
  return { wifi_dbm_reading, false };
}

size_t get_metric_text(metric_t metric, char* optdata, char* buf, size_t sz_max) {
  metric_resp_t resp = metric.fetch();
  int res;

  if ( resp.is_error && !ignore_metric_errors ) {
    res = snprintf(
      buf, sz_max, "# ERROR %s encountered an error, or the result was out of range `%s`\n\n",
      metric.id, resp.message
    );
  } else {
    res = snprintf(
      buf, sz_max, "# HELP %s %s\n# TYPE %s %s\n%s%s %s\n\n",
      metric.id, metric.desc,
      metric.id, METRIC_TYPES[metric.type],
      metric.id, optdata, resp.message
    );
  }

  if (!( res >= 0 && res <= sz_max )) {
    Serial.println(F("[get_metric_text()] formatting error!"));
    return -1;
  }

  return res;
}

size_t generate_ident(char* buf, size_t sz_max) {
  int res = snprintf( buf, sz_max, "{id=\"%s\",mac=\"%s\"}", deviceId != nullptr ? deviceId : "(null)", WiFi.macAddress().c_str() );

  if (!( res >= 0 && res <= sz_max )) {
    Serial.println(F("[generate_ident()] formatting error!"));
    return -1;
  }

  return res;
}

String GenerateMetrics() {
  metric_t metrics[] = {
    { METRIC_TYPE_GUAGE, "pm02", "PM2.5 Particulate Matter Concentration (ug/m^3)", get_pm2 },
    { METRIC_TYPE_GUAGE, "rc02", "CO2 Concentration (ppm)", get_co2 },
    { METRIC_TYPE_GUAGE, "atmp", "Ambient Temperature (*C)", get_atmp },
    { METRIC_TYPE_GUAGE, "rhum", "Relative Humidity (%)", get_rhum },
    { METRIC_TYPE_GUAGE, "wifi", "WiFi Signal Strength (dBm)", get_wifi_dbm }
  };

  String response = "";

  const size_t IDENT_MAX = 96;
  char ibuf[IDENT_MAX];
  generate_ident(ibuf, IDENT_MAX);

  for (int i = 0; i < sizeof(metrics) / sizeof(metrics[0]); i++) {
    const size_t METRIC_MAX = 256;
    char mbuf[METRIC_MAX];
    get_metric_text(metrics[i], ibuf, mbuf, METRIC_MAX);
    response += String(mbuf);
  }

  return response;
}

void HandleRoot() {
  server.send(200, "text/plain; version=0.0.4", GenerateMetrics() );
}
void HandleNotFound() {
  const size_t MAX_ERR = 100;
  char buf[MAX_ERR];
  int res = snprintf(buf, MAX_ERR, "<h1>404: File Not Found</h1>\r\n\r\n<p>URI: %s</p>\r\n<p>Method: %s</p>", server.uri().c_str(), (server.method() == HTTP_GET) ? "GET" : "POST");

  if (!( res >= 0 && res <= MAX_ERR )) {
    Serial.println(F("[HandleNotFound()] formatting error!"));
    return;
  }

  server.send(404, "text/html", String(buf));
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
        if (hasPM) {
          int stat = ag.getPM2_Raw();
          showTextRectangle("PM2",String(stat), true);
        }
        break;
      case 1:
        if (hasCO2) {
          int stat = ag.getCO2_Raw();
          showTextRectangle("CO2", String(stat), true);
        }
        break;
      case 2:
        if (hasSHT) {
          TMP_RH stat = ag.periodicFetchData();
          showTextRectangle("ATMP", String(stat.t), true);
        }
        break;
      case 3:
        if (hasSHT) {
          TMP_RH stat = ag.periodicFetchData();
          showTextRectangle("RHUM", String(stat.rh) + "%", true);
        }
        break;
    }
    counter++;
    if (counter > 3) counter = 0;
    lastUpdate = millis();
  }
}