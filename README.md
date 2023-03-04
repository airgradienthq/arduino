Improved version of AirGradient Arduino Library for ESP8266 (Wemos D1 MINI)
===========================================================================

This is improved fork of official [airgradienthq/arduino](https://github.com/airgradienthq/arduino) repo this version **enables** Plantower PMS5003 sensor capabilities  
**to report PM1, PM2.5 and PM10 measurments** which are not used by AirGradient project by default.  
  
![AirGradient Monitoring Dashboard in Grafana picture 1](/images/air-gradient_1.png)  
![AirGradient Monitoring Dashboard in Grafana picture 2](/images/air-gradient_2.png)

This fork also fixes:  
* PMS5003 bug. AirGradient sensor PM2.5 reports -1 value
* CO2 (RCO2) sensor bug. AirGradient sensor for CO2 reports -1 value
* CO2 (RCO2) sensor bug. AirGradient sensor for CO2 reports 65278 value
* add temperature adjustment value

Thats how this bugs looks on the graphs:  
![AirGradient Monitoring Dashboard in Grafana picture 2](/images/air-gradient_3.png)  
If you have same kind of reporting from your AirGradient - go with this version.  

Installation
------------
Installation is very simple.

1. Install Arduino IDE from brew(mac) or [from official site](https://www.arduino.cc/en/software):
   ```shell
   brew install --cask arduino
   ```
2. Open Arduino IDE
3. Open the `Preferences` window
4. Add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to `Additional Boards Manager URL`.
5. Then go to `Tools` > `Board menu` and open `Boards Manager...`.
6. Search for the `esp8266` board and install it's latest version **3.1.0** *(04.03.2023)*.
7. Select `LOLIN(WEMOS) D1 R2 Mini` from the `Tools` > `Board` > `ESP8266 menu`.
8. Go to `Tools` > `Manage Libraries...` and install following libraries:
   * `AirGradient` the latest version **2.4.0** *(04.03.2023)*
   * `ESP8266 and ESP32 OLED driver for SSD1306` by ThingPulse version **4.3.0**  *(04.03.2023)*
   * `WifiManager` by tablatronix *(optional for 04.03.23)*
9. Choose `C02_PM1_PM2_PM10_SHT_OLED_WIFI.ino` from the `File` > `Open` menu.
10. Setup WiFi SSID and password under `// WiFi and IP connection info.` file section.
11. Setup your AirGradient sensor ID under `// AirGradient sensor ID.` file section.
12. You can optionally adjust temrerature value under `// Dirty Temp adjust (-4 degrees)` in lines 212 and 322.

Then flash updated `C02_PM1_PM2_PM10_SHT_OLED_WIFI.ino` as new AirGradient board firmware.

AirGradient Arduino Library for ESP8266 (Wemos D1 MINI)
=====================================================================================================

Build your own low cost air quality sensor with optional display measuring PM2.5, CO2, Temperature and Humidity. 

This library makes it easy to read the sensor data from the Plantower PMS5003 PM2.5 sensor, the Senseair S8 and the SHT30/31 Temperature and Humidity sensor. Visit our DIY section for detailed build instructions and PCB layout.

https://www.airgradient.com/open-airgradient/instructions/
