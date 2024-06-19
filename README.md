AirGradient Arduino Library for ESP8266 (Wemos D1 MINI) and ESP32 (ESP32-C3 Mini)
=====================================================================================================

This is the code for the AirGradient open-source indoor and outdoor air quality monitors with ESP8266 / ESP32-C3 Microcontrollers.

More information on the air quality monitors and kits are available here:
Indoor Monitor: [https://www.airgradient.com/indoor/](https://www.airgradient.com/indoor/)
Outdoor Monitor: [https://www.airgradient.com/outdoor/](https://www.airgradient.com/outdoor/)

This library supports the following sensor modules:
- Plantower PMS5003
- Plantower PMS5003T
- SenseAir S8
- Sensirion SGP41
- Sensirion SHT40

## Important information

Make sure you have exactly the versions of libraries and boards installed as described in the comment section of the example files.

If you have an older version of the AirGradient PCB not mentioned in the example files, please downgrade this library to version 2.4.15 to support these legacy boards.

## Help & Support

If you have any questions or problems, check out [our forum](https://forum.airgradient.com/). 

## Documentation

Local server API documentation is available in [/docs/local-server.md](/docs/local-server.md) and AirGradient server API on [https://api.airgradient.com/public/docs/api/v1/](https://api.airgradient.com/public/docs/api/v1/).

## The following libraries have been integrated into this library for ease of use

- [Adafruit BusIO](https://github.com/adafruit/Adafruit_BusIO)
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
- [Adafruit SH110X](https://github.com/adafruit/Adafruit_SH110X)
- [Adafruit SSD1306 Wemos Mini OLED](https://github.com/stblassitude/Adafruit_SSD1306_Wemos_OLED)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Sensirion Gas Index Algorithm](https://github.com/Sensirion/arduino-gas-index-algorithm)
- [Sensirion Core](https://github.com/Sensirion/arduino-core/)
- [Sensirion I2C SGP41](https://github.com/Sensirion/arduino-i2c-sgp41)
- [Sensirion I2C SHT](https://github.com/Sensirion/arduino-sht)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [Arduino_JSON](https://github.com/arduino-libraries/Arduino_JSON)
- [PubSubClient](https://github.com/knolleary/pubsubclient)

## License
CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License
