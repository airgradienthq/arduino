# AirGradient Prometheus Arduino Server for ESP8266

Build your own low cost air quality sensor with optional display measuring PM2.5, CO2, Temperature and Humidity.

This sketch makes it easy for prometheus to read the sensor data from the Plantower PMS5003 PM2.5 sensor, the Senseair S8 and the SHT30/31 Temperature and Humidity sensor. Visit our DIY section for detailed build instructions and PCB layout.

https://www.airgradient.com/diy/

## How To Use

By default, all 3 sensors are enabled. If you are planning to not use one of them, make sure you disable the correct sensor in the sketch by changing `true` to `false`.

Make sure you change your SSID and password, that way you can connect to it from your prometheus server. You may want to assign it a static ip from your router.

Verify that your ESP8266 is working by using curl to verify the response. It should look something like this:

```sh
$ curl http://your-ip:9926/metrics
# HELP pm02 PM2.5 Particulate Matter Concentration (ug/m^3)
# TYPE pm02 gauge
pm02{id="AirGradient",mac="00:11:22:33:44:55"} 6

# HELP rc02 CO2 Concentration (ppm)
# TYPE rc02 gauge
rco2{id="AirGradient",mac="00:11:22:33:44:55"} 862

# HELP atmp Ambient Temperature (*C)
# TYPE atmp gauge
atmp{id="AirGradient",mac="00:11:22:33:44:55"} 31.6

# HELP rhum Relative Humidity (%)
# TYPE rhum gauge
rhum{id="AirGradient",mac="00:11:22:33:44:55"} 38

# HELP wifi WiFi Signal Strength (dBm)
# TYPE wifi guage
wifi{id="AirGradient",mac="00:11:22:33:44:55"} -69
```

If a sensor is disabled or non-functional, it's respone will look like the following:

```sh
# ERROR pm02 encountered an error, or the result was out of range `0`
# ERROR rc02 encountered an error, or the result was out of range `(disabled)`
```

## License

See [LICENSE.md](LICENSE.md).

## Authors

[AirGradientHQ](https://github.com/airgradienthq/),
[Jordan Jones](https://github.com/kashalls),
[Bradan J. Wolbeck](https://www.compaqdisc.com/)