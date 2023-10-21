/*
This is the code for the AirGradient DIY BASIC Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

Build Instructions: https://www.airgradient.com/open-airgradient/instructions/diy/

Kits (including a pre-soldered version) are available: https://www.airgradient.com/open-airgradient/kits/

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.11-beta
“U8g2” by oliver tested with version 2.32.15
"Arduino-SHT" by Johannes Winkelmann Version 1.2.2

Configuration:
Please set in the code below the configuration parameters.

If you have any questions please visit our forum at https://forum.airgradient.com/

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/

MIT License

*/

/*
Based on examples\DIY_BASIC\DIY_BASIC.ino

Arduino IDE:
Select LOLIN(WEMOS) D1 R2 & mini

A fatal esptool.py error occurred: Cannot configure port, something went wrong. Original message: PermissionError(13, 'A device attached to the system is not functioning.', None, 31)esptool.py v3.0
> Install older CH340 driver. https://oemdrivers.com/usb-ch340Y: CH34x_Install_Windows_v3_4.zip

Be sure to update the calibration parameters
*/


#include "AirGradient.h"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SHTSensor.h>
#include <U8g2lib.h>
#include <WiFiClient.h>
#include <WiFiManager.h>

AirGradient ag = AirGradient();
SHTSensor sht;

U8G2_SSD1306_64X48_ER_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //for DIY BASIC


// CONFIGURATION START

//set to the endpoint you would like to use
String APIROOT = "http://hw.airgradient.com/";

// set to true to switch from Celcius to Fahrenheit
boolean inF = true;

// PM2.5 in US AQI (default ug/m3)
boolean inUSAQI = true;

// set to true if you want to connect to wifi. You have 60 seconds to connect. Then it will go into an offline mode.
boolean connectWIFI=true;

// CONFIGURATION END


unsigned long currentMillis = 0;

const int oledInterval = 3000;
unsigned long previousOled = 0;

const int invertInterval = 3600000;
unsigned long previousInvert = 0;
bool inverted = false;

const int sendToHubitatInterval = 30000;
unsigned long previoussendToHubitat = 0;

const int sendToServerInterval = 30000;
unsigned long previoussendToServer = 0;

const int co2Interval = 25000;
unsigned long previousCo2 = 0;
int Co2 = 0;

unsigned int badReadingCount = 0;

const int pmInterval = 20000;
unsigned long previousPm = 0;
int pm1 = 0;
int pm2_5 = 0;
int pm10 = 0;

const int tempHumInterval = 15000;
unsigned long previousTempHum = 0;
float temp = 0;
int hum = 0;
long val;

// Calibration values
float temp_offset_c = -3.2;
int hum_offset = 14;

void setup()
{
  Serial.begin(115200);
  sht.init();
  sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM);
  u8g2.setBusClock(100000);
  u8g2.begin();

  // brightness, 0 - 255
  u8g2.setContrast(75);

  updateOLED();

    if (connectWIFI) {
    connectToWifi();
  }
  updateOLED2("Warm Up", "Serial#", String(ESP.getChipId(), HEX));
  ag.CO2_Init();
  ag.PMS_Init();
  //ag.TMP_RH_Init(0x44);
}


void loop()
{
  currentMillis = millis();
  updateOLED();
  updateCo2();
  updatePm();
  updateTempHum();
  //sendToHubitat();
  //sendToServer();
}

void updateCo2()
{
    if (currentMillis - previousCo2 >= co2Interval) {
      previousCo2 += co2Interval;
      Co2 = ag.getCO2_Raw();
      Serial.println(String(Co2));
    }
}

void updatePm()
{
    if (currentMillis - previousPm >= pmInterval) {
      previousPm += pmInterval;
      pm1 = ag.getPM1_Raw();
      pm2_5 = ag.getPM2_Raw();
      pm10 = ag.getPM10_Raw();
      Serial.println("PM1: " + String(pm1) + " PM 2.5: " + String(pm2_5) + " PM10: " + String(pm10));
    }
}

void updateTempHum()
{
    if (currentMillis - previousTempHum >= tempHumInterval) {
      previousTempHum += tempHumInterval;
    if (sht.readSample()) {
      Serial.print("SHT:\n");
      Serial.print("  RH: ");
      Serial.print(sht.getHumidity(), 2);
      Serial.print("\n");
      Serial.print("  T:  ");
      Serial.print(sht.getTemperature(), 2);
      Serial.print("\n");

      if (((sht.getTemperature() >= 0.01) || (sht.getTemperature() <= -0.01)) && sht.getHumidity() > 0) {
        temp = sht.getTemperature() + temp_offset_c;
        hum = sht.getHumidity() + hum_offset;
      } else {
        Serial.println("Invalid temp or humidity: " + String(sht.getTemperature()) + " " + sht.getHumidity());

        if ( ++badReadingCount >= 10 ) {
          Serial.println("Reset due to 10 invalid readings");
          ESP.restart();
        }
      }
      Serial.println("Calibrated Temp: " + String(temp) + " Humidity: " + hum);
    }
  }
}

void updateOLED() {
   if (currentMillis - previousOled >= oledInterval) {
     previousOled += oledInterval;

    String ln1;
    String ln2;
    String ln3;

    if (inUSAQI){
       ln1 = "AQI:" + String(PM_TO_AQI_US(pm2_5)) ;
    } else {
       ln1 = "PM: " + String(pm2_5) +"ug" ;
    }

    ln2 = "CO2:" + String(Co2);

      if (inF) {
        ln3 = String((temp* 9 / 5) + 32).substring(0,4) + " " + String(hum)+"%";
        } else {
        ln3 = String(temp).substring(0,4) + " " + String(hum)+"%";
       }
     updateOLED2(ln1, ln2, ln3);
   }
}

void updateOLED2(String ln1, String ln2, String ln3) {
  // OLED wear leveling
  if (currentMillis - previousInvert >= invertInterval) {
    previousInvert = currentMillis;
    if (inverted) {
      inverted = false;
    } else {
      inverted = true;
    }
  }
  
  u8g2.firstPage();

  do {
    // OLED wear leveling
    if (inverted) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, 0, 64, 48);
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(0);
      u8g2.drawBox(0, 0, 64, 48);
      u8g2.setDrawColor(1);
    }

    u8g2.setFont(u8g2_font_t0_16_tf);
    u8g2.drawStr(1, 10, String(ln1).c_str());
    u8g2.drawStr(1, 28, String(ln2).c_str());
    u8g2.drawStr(1, 46, String(ln3).c_str());
  } while ( u8g2.nextPage() );
}

void getURL(String URL) {
  //Serial.println(URL);
  WiFiClient client;
  HTTPClient http;
  http.begin(client, URL);
  http.addHeader("content-type", "application/json");
  int httpCode = http.GET();
  String response = http.getString();
  //Serial.println("Hubitat: " + httpCode);
  //Serial.println("Hubitat: " + response);
  http.end();
}

void sendToHubitat() {
  if (currentMillis - previoussendToHubitat >= sendToHubitatInterval) {
    previoussendToHubitat += sendToHubitatInterval;

    const String urlBase = "http://10.0.40.10/apps/api/727/devices/";
    const String device = "821/";
    // Yeah, it's public on github. Good luck doing anything with it.
    const String token = "?access_token=24c92325-da8b-4017-ba30-4bb678b9dd8b";

    if (WiFi.status() == WL_CONNECTED) {
      // TODO POST would be nice
      getURL(urlBase + device + "setCarbonDioxide/" + String(Co2) + token);
      getURL(urlBase + device + "setPM1/" + String(pm1) + token);
      getURL(urlBase + device + "setPM2_5/" + String(pm2_5) + token);
      getURL(urlBase + device + "setPM10/" + String(pm10) + token);
      getURL(urlBase + device + "setAQI_PM2_5/" + String(PM_TO_AQI_US(pm2_5)) + token);
      getURL(urlBase + device + "setTemperature/" + String((temp * 9 / 5) + 32) + token);
      getURL(urlBase + device + "setRelativeHumidity/" + String(hum) + token);
      Serial.println("Sent to Hubitat");
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  }
}

void sendToServer() {
   if (currentMillis - previoussendToServer >= sendToServerInterval) {
     previoussendToServer += sendToServerInterval;

      String payload = "{\"wifi\":" + String(WiFi.RSSI())
      + (Co2 < 0 ? "" : ", \"rco2\":" + String(Co2))
      + (pm2_5 < 0 ? "" : ", \"pm02\":" + String(pm2_5))
      + ", \"atmp\":" + String(temp)
      + (hum < 0 ? "" : ", \"rhum\":" + String(hum))
      + "}";

      if(WiFi.status()== WL_CONNECTED){
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
      else {
        Serial.println("WiFi Disconnected");
      }
   }
}

// Wifi Manager
 void connectToWifi() {
   WiFiManager wifiManager;
   //WiFi.disconnect(); //to delete previous saved hotspot
   String HOTSPOT = "AG-" + String(ESP.getChipId(), HEX);
   updateOLED2("Connect", "Wifi AG-", String(ESP.getChipId(), HEX));
   delay(2000);
   wifiManager.setConnectTimeout(10);
   wifiManager.setConfigPortalTimeout(30);
   if (!wifiManager.autoConnect((const char * ) HOTSPOT.c_str())) {
     updateOLED2("Booting", "offline", "mode");
     Serial.println("failed to connect and hit timeout");
     delay(6000);
   }
}

// Calculate PM2.5 US AQI
int PM_TO_AQI_US(int pm02) {
  if (pm02 <= 12.0) return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4) return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4) return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4) return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4) return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4) return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4) return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else return 500;
};
