#include "PMS.h"

#include <HardwareSerial.h>

#include <Wire.h>

#include "s8_uart.h"

#include <HTTPClient.h>

#include <WiFiManager.h>

#include <Adafruit_NeoPixel.h>

#include <EEPROM.h>

#include "SHTSensor.h"

#include <SensirionI2CSgp41.h>

#include <NOxGasIndexAlgorithm.h>

#include <VOCGasIndexAlgorithm.h>

#include <U8g2lib.h>

#define DEBUG true

#define I2C_SDA 7
#define I2C_SCL 6

HTTPClient client;

Adafruit_NeoPixel pixels(11, 10, NEO_GRB + NEO_KHZ800);
SensirionI2CSgp41 sgp41;
VOCGasIndexAlgorithm voc_algorithm;
NOxGasIndexAlgorithm nox_algorithm;
SHTSensor sht;

PMS pms1(Serial0);

PMS::DATA data1;

S8_UART * sensor_S8;
S8_sensor sensor;

// time in seconds needed for NOx conditioning
uint16_t conditioning_s = 10;

// for peristent saving and loading
int addr = 4;
byte value;

// Display bottom right
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

String APIROOT = "http://hw.airgradient.com/";

// set to true to switch from Celcius to Fahrenheit
boolean inF = false;

// PM2.5 in US AQI (default ug/m3)
boolean inUSAQI = false;

// Display Position
boolean displayTop = true;

// use RGB LED Bar
boolean useRGBledBar = true;

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
int TVOC = 0;
int NOX = 0;

const int co2Interval = 5000;
unsigned long previousCo2 = 0;
int Co2 = 0;

const int pmInterval = 5000;
unsigned long previousPm = 0;
int pm25 = 0;
int pm01 = 0;
int pm10 = 0;
int pm03PCount = 0;

const int tempHumInterval = 2500;
unsigned long previousTempHum = 0;
float temp = 0;
int hum = 0;

int buttonConfig = 0;
int lastState = LOW;
int currentState;
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;

void setup() {
  if (DEBUG) {
    Serial.begin(115200);
    // see https://github.com/espressif/arduino-esp32/issues/6983
    Serial.setTxTimeoutMs(0); // <<<====== solves the delay issue
  }

  Wire.begin(I2C_SDA, I2C_SCL);
  pixels.begin();
  pixels.clear();

  Serial1.begin(9600, SERIAL_8N1, 0, 1);
  Serial0.begin(9600);
  u8g2.begin();

  updateOLED2("Warming Up", "Serial Number:", String(getNormalizedMac()));
  sgp41.begin(Wire);

  sht.init(Wire);
  sht.setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM);

  //init Watchdog
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  sensor_S8 = new S8_UART(Serial1);

  EEPROM.begin(512);
  delay(500);

  // push button
  pinMode(9, INPUT_PULLUP);

  buttonConfig = String(EEPROM.read(addr)).toInt();
  if (buttonConfig > 3) buttonConfig = 0;
  delay(400);
  setConfig();
  Serial.println("buttonConfig: " + String(buttonConfig));
  updateOLED2("Press Button", "Now for", "Config Menu");
  delay(2000);
  currentState = digitalRead(9);
  if (currentState == LOW) {
    updateOLED2("Entering", "Config Menu", "");
    delay(3000);
    lastState = HIGH;
    setConfig();
    inConf();
  }

  countdown(3);

  if (connectWIFI) {
    connectToWifi();
  }
  sendPing();
  updateOLED2("Warming Up", "Serial Number:", String(getNormalizedMac()));
}

void loop() {
  currentMillis = millis();
  updateTVOC();
  updateOLED();
  updateCo2();
  updatePm();
  updateTempHum();
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
    previousTVOC += tvocInterval;
    TVOC = voc_algorithm.process(srawVoc);
    NOX = nox_algorithm.process(srawNox);
    Serial.println(String(TVOC));
  }
}

void updateCo2() {
  if (currentMillis - previousCo2 >= co2Interval) {
    previousCo2 += co2Interval;
    Co2 = sensor_S8 -> get_co2();
    Serial.println(String(Co2));
    setRGBledCO2color(Co2);
  }
}

void updatePm() {
  if (currentMillis - previousPm >= pmInterval) {
    previousPm += pmInterval;
    if (pms1.readUntil(data1, 2000)) {
      pm01 = data1.PM_AE_UG_1_0;
      pm25 = data1.PM_AE_UG_2_5;
      pm10 = data1.PM_AE_UG_10_0;
      pm03PCount = data1.PM_RAW_0_3;
    }
  }
}

void updateTempHum() {
  if (currentMillis - previousTempHum >= tempHumInterval) {
    previousTempHum += tempHumInterval;

    if (sht.readSample()) {
      temp = sht.getTemperature();
      hum = sht.getHumidity();
    } else {
      Serial.print("Error in readSample()\n");
    }
  }
}

void updateOLED() {
  if (currentMillis - previousOled >= oledInterval) {
    previousOled += oledInterval;

    String ln3;
    String ln1;

    if (inUSAQI) {
      ln1 = "AQI:" + String(PM_TO_AQI_US(pm25)) + " CO2:" + String(Co2);
    } else {
      ln1 = "PM:" + String(pm25) + " CO2:" + String(Co2);
    }

    String ln2 = "TVOC:" + String(TVOC) + " NOX:" + String(NOX);

    if (inF) {
      ln3 = "F:" + String((temp * 9 / 5) + 32) + " H:" + String(hum) + "%";
    } else {
      ln3 = "C:" + String(temp) + " H:" + String(hum) + "%";
    }
    updateOLED2(ln1, ln2, ln3);
  }
}

void inConf() {
  setConfig();
  currentState = digitalRead(9);

  if (currentState) {
    Serial.println("currentState: high");
  } else {
    Serial.println("currentState: low");
  }

  if (lastState == HIGH && currentState == LOW) {
    pressedTime = millis();
  } else if (lastState == LOW && currentState == HIGH) {
    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;
    if (pressDuration < 1000) {
      buttonConfig = buttonConfig + 1;
      if (buttonConfig > 3) buttonConfig = 0;
    }
  }

  if (lastState == LOW && currentState == LOW) {
    long passedDuration = millis() - pressedTime;
    if (passedDuration > 4000) {
      updateOLED2("Saved", "Release", "Button Now");
      delay(1000);
      updateOLED2("Rebooting", "in", "5 seconds");
      delay(5000);
      EEPROM.write(addr, char(buttonConfig));
      EEPROM.commit();
      delay(1000);
      ESP.restart();
    }

  }
  lastState = currentState;
  delay(100);
  inConf();
}

void setConfig() {
  if (buttonConfig == 0) {
    updateOLED2("Temp. in C", "PM in ug/m3", "Long Press Saves");
    u8g2.setDisplayRotation(U8G2_R0);
    inF = false;
    inUSAQI = false;
  }
  if (buttonConfig == 1) {
    updateOLED2("Temp. in C", "PM in US AQI", "Long Press Saves");
    u8g2.setDisplayRotation(U8G2_R0);
    inF = false;
    inUSAQI = true;
  } else if (buttonConfig == 2) {
    updateOLED2("Temp. in F", "PM in ug/m3", "Long Press Saves");
    u8g2.setDisplayRotation(U8G2_R0);
    inF = true;
    inUSAQI = false;
  } else if (buttonConfig == 3) {
    updateOLED2("Temp. in F", "PM in US AQI", "Long Press Saves");
    u8g2.setDisplayRotation(U8G2_R0);
    inF = true;
    inUSAQI = true;
  }
}

void switchLED(boolean ledON) {
  if (ledON) {
    digitalWrite(10, HIGH);
  } else {
    digitalWrite(10, LOW);
  }
}

void sendPing() {
  String payload = "{\"wifi\":" + String(WiFi.RSSI()) +
    ", \"boot\":" + loopCount +
    "}";
}

void updateOLED2(String ln1, String ln2, String ln3) {
  char buf[9];
  u8g2.firstPage();
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_t0_16_tf);
    u8g2.drawStr(1, 10, String(ln1).c_str());
    u8g2.drawStr(1, 30, String(ln2).c_str());
    u8g2.drawStr(1, 50, String(ln3).c_str());
  } while (u8g2.nextPage());
}

void sendToServer() {
  if (currentMillis - previoussendToServer >= sendToServerInterval) {
    previoussendToServer += sendToServerInterval;
    String payload = "{\"wifi\":" + String(WiFi.RSSI()) +
      (Co2 < 0 ? "" : ", \"rco2\":" + String(Co2)) +
      (pm01 < 0 ? "" : ", \"pm01\":" + String(pm01)) +
      (pm25 < 0 ? "" : ", \"pm02\":" + String(pm25)) +
      (pm10 < 0 ? "" : ", \"pm10\":" + String(pm10)) +
      (pm03PCount < 0 ? "" : ", \"pm003_count\":" + String(pm03PCount)) +
      (TVOC < 0 ? "" : ", \"tvoc_index\":" + String(TVOC)) +
      (NOX < 0 ? "" : ", \"nox_index\":" + String(NOX)) +
      ", \"atmp\":" + String(temp) +
      (hum < 0 ? "" : ", \"rhum\":" + String(hum)) +
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
      Serial.println(response);
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

void setRGBledCO2color(int co2Value) {
  if (co2Value < 800) setRGBledColor('g');
  if (co2Value >= 800 && co2Value < 1000) setRGBledColor('y');
  if (co2Value >= 1000 && co2Value < 1500) setRGBledColor('o');
  if (co2Value >= 1500 && co2Value < 2000) setRGBledColor('r');
  if (co2Value >= 2000 && co2Value < 3000) setRGBledColor('p');
  if (co2Value >= 3000 && co2Value < 10000) setRGBledColor('b');
}

void setRGBledColor(char color) {
  if (useRGBledBar) {
    //pixels.clear();
    switch (color) {
    case 'g':
      for (int i = 0; i < 11; i++) {
        pixels.setPixelColor(i, pixels.Color(0, 255, 0));
        delay(100);
        pixels.show();
      }
      break;
    case 'y':
      for (int i = 0; i < 11; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 255, 0));
        delay(100);
        pixels.show();
      }
      break;
    case 'o':
      for (int i = 0; i < 11; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 128, 0));
        delay(100);
        pixels.show();
      }
      break;
    case 'r':
      for (int i = 0; i < 11; i++) {
        pixels.setPixelColor(i, pixels.Color(255, 0, 0));
        delay(100);
        pixels.show();
      }
      break;
    case 'p':
      for (int i = 0; i < 11; i++) {
        pixels.setPixelColor(i, pixels.Color(153, 0, 153));
        delay(100);
        pixels.show();
      }
      break;
    case 'b':
      for (int i = 0; i < 11; i++) {
        pixels.setPixelColor(i, pixels.Color(102, 0, 0));
        delay(100);
        pixels.show();
      }
      break;
    default:
      // if nothing else matches, do the default
      // default is optional
      break;
    }
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
