/*
This code has been modified from the AirGradient DIY examples to include additional display details,
remote/OTA updates from Arduino IDE, a JSON parsing/serializing library, and defined delay/retry intervals.
Note that this does not have a full exponential backoff (TBD) nor TLS (TLS on the 8266 should be run 
"overclocked" at 80MHz which causes the chip and sensor to run hot, which may affect stability and
the room temperature readings)

January 2021, init99


Configuration: See the BEING CONFIGS block below


Original notes follow:

This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

For build instructions please visit https://www.airgradient.com/diy/

Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)
SenseAir S8 (CO2 Sensor)
SHT30/31 (Temperature/Humidity Sensor)

Dependent Libraries:
The codes needs the following libraries installed:
ESP8266 board with standard libraries
WifiManager by tzar, tablatronix tested with Version 2.0.3-alpha
ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse, Fabrice Weinberg tested with Version 4.1.0


If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/schools/

MIT License
*/

#include <Arduino.h>
#include <AirGradient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <Wire.h>
#include "SSD1306Wire.h"

#include <ArduinoJson.h>

#include <list>

#include <WiFiUdp.h>

#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

// BEGIN FUNCTION DEFS
void updateAndPoll();
float convertCelsiusToFahrenheit(float celsius);
void showFiveLineText(String ln1, String ln2, String ln3, String ln4, String ln5);
void showPlainText(String ln1);
void showTwoLineText(String ln1, String ln2);
void log(String source, String message);
void connectToWifi();

// END FUNCTION DEFS

// BEGIN CONSTANTS

// Update this value every version. 
String MY_VERSION = "1.02";

const int MILLIS = 1000;

const int HTTP_SUCCESS = 200;

const String PM25_UNIT = "µg/m3";
const String CO2_UNIT = "ppm";
const String TMP_C_UNIT = "°C";
const String TMP_F_UNIT = "°F";
const String TMPRH_UNIT = "%";

const int MIN_CO2 = 0;
const int MAX_CO2 = 10000;
const int MIN_PM2 = 0;
const int MAX_PM2 = 5000; 
const int MIN_TMP = -100;
const int MAX_TMP = 1000;
const int MIN_RH = 0;
const int MAX_RH = 150;

// How often to change display
const int DISPLAY_CHANGE_MILLIS = 4 * MILLIS;

// How often to poll the AG backend and reread (update) the sensors
unsigned long POLL_MILLIS = 15 * 1000;
unsigned long UPDATE_MILLIS = 5 * 1000;

// Max delay to wait to send if we can't reach the backend.
unsigned long MAX_POLL_DELAY_MILLIS = POLL_MILLIS * 4;

// How many times have we failed to send
int NUM_FAILED_POLL_ATTEMPTS = 0;

// Max minutes of no poll or update that will force a reboot in an attempt to reconnect/fix any issue
unsigned long MAX_NO_POLL_NO_UPDATE_MILLIS = 1000 * 60 * 30;

// Last poll/update timestamp (in epoch/seconds)
unsigned long LAST_POLL_TS = 0;
unsigned long LAST_UPDATE_TS = 0;

// How long to run before rebooting. 
  // Note that this needs to stay under max of millis() (unsigned long) which is 4294967295
  // or about 49.7 days
  // This could be handled more gracefully e.g. 
  // https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/
  // but I am taking a simpler approach. I have attempted to use NTPClient and encountered a (common) bug where
  // some ntp replies are parsed as the year 2036. When this happens, polling and updating are disrupted.
  // https://github.com/arduino-libraries/NTPClient/issues/84
  // So I've chosen to stay within the bounds of millis() instead.
// 45 days 
unsigned long MAX_RUNTIME_MILLIS = 3888000000;

// END CONSTANTS

// BEGIN DEFS

typedef enum {
  PM25,
  CO2,
  TMPRH,
} Sensors_t;


typedef struct READINGS {
  String temp_c;
  String temp_f;
  String rel_hum;
  String co2;
  String pm25;
  String rco2_lbl;
  String pm02_lbl;
  String pi02_lbl;
};

// END DEFS

// BEGIN GLOBAL OBJECTS

struct READINGS reads;

std::list<Sensors_t> enabled_sensors;

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

// END GLOBAL OBJECTS

// BEGIN CONFIGS

// set to false if you want to display Fahrenheit, true to display Celsius
boolean displayCelsius=false;

// set to true if only the TMPRH sensor will be used, and only temp will be displayed 
// For example, if using a remote/waterproof temp probe to read temp 
boolean onlyTemp=false;

// change if you want to send the data to another server
String APIROOT = "http://hw.airgradient.com/";
String POSTURL = APIROOT + "sensors/airgradient:" + String(ESP.getChipId(),HEX) + "/measures";

// Password used in WiFiManager AP, and in OTA updates
String PASSWORD = "changeThisPassword";

// END CONFIGS


void setup() {
  Serial.begin(9600);

  display.init();

  reads.temp_c = "";
  reads.temp_f = "";
  reads.rel_hum = "";
  reads.co2 = "";
  reads.pm25 = "";
  reads.rco2_lbl = "";
  reads.pm02_lbl = "";
  reads.pi02_lbl = "";

  if(onlyTemp) {
    MY_VERSION = MY_VERSION + "t";
  }

  showFiveLineText("Welcome to","Air", " Gradient","DIY vers.", MY_VERSION);
  delay(DISPLAY_CHANGE_MILLIS);
  showFiveLineText("ID: ","",String(ESP.getChipId(),HEX),"","");
  delay(DISPLAY_CHANGE_MILLIS);
  
  
  // Initialize sensors
  if(!onlyTemp) {
    ag.PMS_Init();
    log("SETUP", "PSM Initialized");
    ag.CO2_Init();
    log("SETUP", "CO2 Initialized");
  }

  ag.TMP_RH_Init(0x44);
  log("SETUP", "TMP_RH Initialized");


  String sensor_status = "";

  // Check values
  // NOTE: without PM sensor attached, getting 0 reading... 
  // Difficult to test for presence; at least we check for wild readings.
  if(!onlyTemp) {
    int pm2 = ag.getPM2_Raw();
    if ((pm2 < MIN_PM2) || (pm2 > MAX_PM2)) {
      log("SETUP", "ERROR: PM2 value error.");
      sensor_status = sensor_status + "PM2.5:Err ";
    } else {
      log("SETUP", "PM2 OK");
      enabled_sensors.push_back(PM25);
      reads.pm25 = String(pm2);
      sensor_status = sensor_status + "PM2.5:OK ";
    }

    int co2 = ag.getCO2_Raw();
    if ((co2 < MIN_CO2) || (co2 > MAX_CO2)) {
      log("SETUP", "ERROR: CO2 value error.");
      sensor_status = sensor_status + "CO2:Err ";
    } else {
      log("SETUP", "CO2 OK");
      enabled_sensors.push_back(CO2);
      reads.co2 = String(co2);
      sensor_status = sensor_status + "CO2:OK ";
    }    
  }

  TMP_RH result = ag.periodicFetchData();

  if ((result.t < MIN_TMP) || (result.t > MAX_TMP) || (result.rh < MIN_RH) || (result.rh > MAX_RH)) {
    log("SETUP", "ERROR: TMP_RH value error.");
    sensor_status = sensor_status + "Tmp:Err ";
  } else {
    log("SETUP", "TMP_RH OK");
    enabled_sensors.push_back(TMPRH);
    reads.temp_c = String(result.t);
    reads.temp_f = String(convertCelsiusToFahrenheit(result.t));
    reads.rel_hum = String(result.rh);
    sensor_status = sensor_status + "Tmp:OK ";
  }

  showPlainText(sensor_status);
  delay(DISPLAY_CHANGE_MILLIS);

  log("SETUP", "Connecting to wifi");
  connectToWifi(); 
  showPlainText("WiFi:OK");
  delay(DISPLAY_CHANGE_MILLIS);
  delay(2000);


  // OTA Setup for on-network push from ArduinoIDE with simple password
  // The following code has been borrowed from the Arduino ESP8266 boilerplat
  // available via File>Examples>Examples for Lolin(Wemos) D1 R2 & Mini > ArduinoOTA > BasicOTA
  // As set up here, the device will appear on a local network after a few minutes,
  // using mDNS, visible to the Arduino IDE under Tools > Port > Network Ports
  // As set here, a password is required. Also to make things easier,
  // MY_VERSION is appended to the device hostname along with the ESP chip ID, 
  // in the case there are multiple AG sensors on a network.
  // NOTE: Size of program can't get too big (e.g. <40%) to leave room for a new copy to be 
  // held in memory when overwriting old copy via OTA.
  // For more details: 
  // https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html#arduino-ide

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  String hostname_s = "AIRGRADIENT-"+MY_VERSION+"-"+String(ESP.getChipId(),HEX);
  ArduinoOTA.setHostname((const char*)hostname_s.c_str());

  // No authentication by default
  ArduinoOTA.setPassword((const char*)PASSWORD.c_str());

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start OTA updating " + type);
    showPlainText("Starting OTA update");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
    showPlainText("Update Done. Restarting.");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));      
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

 
  
}


void loop() {
  // log("loop", "Top");

  if(millis() > MAX_RUNTIME_MILLIS) {
    // Reboot
    log("loop", "Rebooting due to MAX_RUNTIME_MILLIS being exceeded.");
    delay(3000);
    ESP.restart();
  }

  // Check for OTA updates from a local Arduino IDE
  ArduinoOTA.handle();

  // Check serial for a command to reset the stored Wifi information
  // If you connect over serial and send wifi_reset, it will clear out the 
  // stored Wifi SSID and password, and reboot, forcing it to set up the 
  // AP mode, where you can reconfigure wifi
  String command;
  if(Serial.available()) {
    command = Serial.readStringUntil('\n');
    if(command.equals("wifi_reset")){
      log("loop", "Got wifi_reset. Resetting wifi stored credentials and rebooting.");
      showPlainText("Wifi resetting...");
      delay(DISPLAY_CHANGE_MILLIS);
      WiFi.disconnect();
      delay(3000);
      ESP.restart();
    } else {
      log("loop", "Got uknown command: ");
      log("loop", command);

    }
  }

  // Check if Wifi is (still) connected. If not, go to connectToWifi
  if(WiFi.status() != WL_CONNECTED) {
    log("loop", "Lost wifi connectivity. Going to connectToWifi");
    connectToWifi(); 
    showPlainText("WiFi:OK");
    delay(DISPLAY_CHANGE_MILLIS);
    delay(2000);
  }

  updateAndPoll();

  std::list<Sensors_t>::iterator it;
    for(it=enabled_sensors.begin(); it!=enabled_sensors.end(); it++) {
      switch(*it) {
        case(CO2): 
          showFiveLineText("CO2",reads.co2,CO2_UNIT,reads.rco2_lbl,"");
          delay(DISPLAY_CHANGE_MILLIS);
          break;
        case(PM25):
          showFiveLineText("PM2.5",reads.pm25,PM25_UNIT,reads.pi02_lbl,"");
          delay(DISPLAY_CHANGE_MILLIS);
          break;
        case(TMPRH):
          {
            String temp_disp;
            if(displayCelsius) {
              temp_disp = reads.temp_c+TMP_C_UNIT;
            } else {
              temp_disp = reads.temp_f+TMP_F_UNIT;
            }
            if(onlyTemp) {
              showTwoLineText("Temp.",temp_disp);
            } else {
              showFiveLineText("Temp.",temp_disp,"","Rel. Hum.",reads.rel_hum+TMPRH_UNIT);
            }
            delay(DISPLAY_CHANGE_MILLIS);
          }
          break;
        default: break;
      }
      updateAndPoll();
    }

}

void updateAndPoll() {
  // create outgoing payload
  DynamicJsonDocument outbound(108);

  // create incoming payload and filter. Filter keeps only those fields marked true
  DynamicJsonDocument filter(144);
  filter["pm02_clr"] = true;
  filter["pm02_lbl"] = true;
  filter["pm02_idx"] = true;
  filter["pi02_clr"] = true;
  filter["pi02_lbl"] = true;
  filter["pi02_idx"] = true;
  filter["rco2_clr"] = true;
  filter["rco2_lbl"] = true;
  filter["rco2_idx"] = true;

  DynamicJsonDocument inbound(384);
  
  unsigned long current_time = millis();

  log("updateAndPoll", "Current time: ");
  log("updateAndPoll", String(current_time));
  log("updateAndPoll", "LAST_UPDATE_TS and LAST_POLL_TS");
  log("updateAndPoll", String(LAST_UPDATE_TS));
  log("updateAndPoll", String(LAST_POLL_TS));
  log("updateAndPoll", "Next Update at: ");
  log("updateAndPoll", String(LAST_UPDATE_TS + UPDATE_MILLIS));
  log("updateAndPoll", "Next Poll at: ");
  log("updateAndPoll", String(LAST_POLL_TS + POLL_MILLIS + std::min(MAX_POLL_DELAY_MILLIS,(NUM_FAILED_POLL_ATTEMPTS * POLL_MILLIS))));
  log("updateAndPoll", "NUM_FAILED_POLL_ATTEMPTS");
  log("updateAndPoll", String(NUM_FAILED_POLL_ATTEMPTS));
  
  if (current_time > (LAST_UPDATE_TS + UPDATE_MILLIS)) {
    log("updateAndPoll", "Updating.");

    std::list<Sensors_t>::iterator it;
    for(it=enabled_sensors.begin(); it!=enabled_sensors.end(); it++) {
      switch(*it) {
        case(CO2): 
          {
            int co2 = ag.getCO2_Raw();
            if ((co2 < MIN_CO2) || (co2 > MAX_CO2)) {
              log("updateAndPoll", "CO2 Error. Rebooting.");
              showPlainText("CO2 Error. Rebooting.");
              delay(10000);
              ESP.restart();
            } else {
              reads.co2 = String(co2);
            }
          }
          break;
        case(PM25): 
          {
            int pm2 = ag.getPM2_Raw();
            if ((pm2 < MIN_PM2) || (pm2 > MAX_PM2)) {
              log("updateAndPoll", "PM2 Error. Rebooting");
              showPlainText("PM2 Error. Rebooting.");
              delay(10000);
              ESP.restart();
            } else {
              reads.pm25 = String(pm2); 
            }
          }
          break;
        case(TMPRH): 
          {
            TMP_RH result = ag.periodicFetchData();
            if ((result.t < MIN_TMP) || (result.t > MAX_TMP) || (result.rh < MIN_RH) || (result.rh > MAX_RH)) {
              log("updateAndPoll", "TMP RH Error. Rebooting");
              showPlainText("TMP RH Error. Rebooting.");
              delay(10000);
              ESP.restart();
            } else {
              reads.temp_c = String(result.t);
              reads.temp_f = String(convertCelsiusToFahrenheit(result.t));
              reads.rel_hum = String(result.rh);
            }
          }
          break;
        default: break;
      }
    }

    LAST_UPDATE_TS = millis();
  }

  current_time = millis();
  if (current_time > (LAST_POLL_TS + POLL_MILLIS + std::min(MAX_POLL_DELAY_MILLIS,(NUM_FAILED_POLL_ATTEMPTS * POLL_MILLIS)))) {
    log("updateAndPoll", "Polling.");
    outbound["wifi"] = WiFi.RSSI();
    outbound["atmp"] = reads.temp_c;
    outbound["rhum"] = reads.rel_hum;
    if(!onlyTemp) {
      outbound["pm02"] = reads.pm25;
      outbound["rco2"] = reads.co2;
    }

    HTTPClient http;
    http.begin(POSTURL);
    http.addHeader("content-type", "application/json");
    String payload;
    serializeJson(outbound, payload);
    log("updateAndPoll", "Payload: ");
    log("updateAndPoll", payload);
    int httpCode = http.POST(payload);
    log("updateAndPoll", "Got HTTP Response: ");
    log("updateAndPoll", String(httpCode));
    if(httpCode != HTTP_SUCCESS) {
      log("updateAndPoll", "Failed to send, got error:");
      log("updateAndPoll", String(httpCode));
      NUM_FAILED_POLL_ATTEMPTS++;
      reads.rco2_lbl = "";
      reads.pm02_lbl = "";
      reads.pi02_lbl = "";
    } else {
        NUM_FAILED_POLL_ATTEMPTS = 0;
        log("updateAndPoll", "Send successful.");
        LAST_POLL_TS = millis();
        String response = http.getString();
        http.end();
        log("updateAndPoll", response);
        DeserializationError error = deserializeJson(inbound, response, DeserializationOption::Filter(filter));
        if(error) {
          log("updateAndPoll", "DeserializationError:");
          log("updateAndPoll", error.f_str());
          reads.rco2_lbl = "";
          reads.pm02_lbl = "";
          reads.pi02_lbl = "";
        } else {
            reads.rco2_lbl = inbound["rco2_lbl"].as<String>();
            if(reads.rco2_lbl == "null") {
              reads.rco2_lbl = "";
            }
            reads.pm02_lbl = inbound["pm02_lbl"].as<String>();
            reads.pi02_lbl = inbound["pi02_lbl"].as<String>();
        }
      }
    }

// If it has been more than 30 minutes since we have polled or updated, reboot
  if(((millis() - LAST_POLL_TS) > MAX_NO_POLL_NO_UPDATE_MILLIS) || ((millis() - LAST_UPDATE_TS) > MAX_NO_POLL_NO_UPDATE_MILLIS)) {
    log("updateAndPoll", "Too long since last update or poll. Current millis, LAST_POLL_TS, LAST_UPDATE_TS, MAX_NO_POLL_NO_UPDATE_MILLIS:");
    log("updateAndPoll", String(millis()));
    log("updateAndPoll", String(LAST_POLL_TS));
    log("updateAndPoll", String(LAST_UPDATE_TS));
    log("updateAndPoll", String(MAX_NO_POLL_NO_UPDATE_MILLIS));
    log("updateAndPoll", "Rebooting...");
    showPlainText("Err updating or connecting. Rebooting...");
    delay(3000);
    ESP.restart();
  }

}



float convertCelsiusToFahrenheit(float celsius) {
  return (celsius * 9.0) / 5.0 + 32;
}


// DISPLAY ix 64x48

void showFiveLineText(String ln1, String ln2, String ln3, String ln4, String ln5) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(32, 0, ln1);
  display.drawString(32, 8, ln2);
  display.drawString(32, 16, ln3);
  display.drawString(32, 24, ln4);
  display.drawString(32, 32, ln5);
  display.display();
  log("showFiveLineText",ln1+" "+ln2+" "+ln3+" "+ln4+" "+ln5 + "<EOM>");
}

void showPlainText(String ln1) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawStringMaxWidth(32, 0, 64, ln1);
  display.display();
  log("showPlainText", ln1+"<EOM>");
}

void showTwoLineText(String ln1, String ln2) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(32, 0, ln1);  
  display.setFont(ArialMT_Plain_16);
  display.drawString(32, 24, ln2);  
  // display.setFont(ArialMT_Plain_10);
  // display.drawString(32, 32, ln5);
  display.display();
  log("showTwoLineText",ln1+" "+ln2+"<EOM>");
}

void log(String source, String message) {
  Serial.println("["+source+"] "+message);
}

// Wifi Manager
void connectToWifi() {
  log("connectToWifi", "Starting.");
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-v"+MY_VERSION+"-"+String(ESP.getChipId(),HEX);
  // Try to connect for 30s
  wifiManager.setConnectTimeout(30);
  // Leave the config portal up for 2m
  wifiManager.setConfigPortalTimeout(120);
  // Original setTimeout deprecated
  // wifiManager.setTimeout(120);
  showPlainText("Setting up wifi...");
  if(!wifiManager.autoConnect((const char*)HOTSPOT.c_str(), (const char*)PASSWORD.c_str())) {
    log("connectToWifi","failed to connect to wifi and hit timeout");
    showPlainText("WiFi: Error");
    WiFi.printDiag(Serial);
    delay(3000);
    ESP.restart();
    delay(5000);
  } else {
    log("connectToWifi", "Connected. Details: ");
    WiFi.printDiag(Serial);
  }
}
