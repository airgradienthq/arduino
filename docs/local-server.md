## Local Server API

From [firmware version 3.0.10](firmwares) onwards, the AirGradient ONE and Open Air monitors have below API available. 

#### Discovery

The monitors run a mDNS discovery. So within the same network, the monitor can be accessed through:

http://airgradient_{{serialnumber}}.local


The following requests are possible:

#### Get Current Air Quality (GET)

With the path "/measures/current" you can get the current air quality data.

http://airgradient_ecda3b1eaaaf.local/measures/current

“ecda3b1eaaaf” being the serial number of your monitor.

You get the following response:
```json 
{
  "wifi": -46,
  "serialno": "ecda3b1eaaaf",
  "rco2": 447,
  "pm01": 3,
  "pm02": 7,
  "pm10": 8,
  "pm003Count": 442,
  "atmp": 25.87,
  "atmpCompensated": 24.47,
  "rhum": 43,
  "rhumCompensated": 49,
  "tvocIndex": 100,
  "tvocRaw": 33051,
  "noxIndex": 1,
  "noxRaw": 16307,
  "boot": 6,
  "bootCount": 6,
  "ledMode": "pm",
  "firmware": "3.1.3",
  "model": "I-9PSL"
}
```

| Properties       | Type   | Explanation                                                        |
|------------------|--------|--------------------------------------------------------------------|
| `serialno`         | String | Serial Number of the monitor                                       |
| `wifi`             | Number | WiFi signal strength                                               |
| `pm01` | Number | PM1 in ug/m3                                       |
| `pm02` | Number | PM2.5 in ug/m3                                       |
| `pm10` | Number | PM10 in ug/m3                                       |
| `pm02Compensated` | Number | PM2.5 in ug/m3 with correction applied (from fw version 3.1.4 onwards)                                       |
| `rco2`             | Number | CO2 in ppm                                                         |
| `pm003Count`       | Number | Particle count per dL                                              |
| `atmp`             | Number | Temperature in Degrees Celsius                                     |
| `atmpCompensated`             | Number | Temperature in Degrees Celsius with correction applied                                      |
| `rhum`             | Number | Relative Humidity                                                  |
| `rhumCompensated`        | Number | Relative Humidity with correction applied                                                  |
| `tvocIndex`        | Number | Senisiron VOC Index                                                |
| `tvocRaw`         | Number | VOC raw value                                                      |
| `noxIndex`         | Number | Senisirion NOx Index                                               |
| `noxRaw`          | Number | NOx raw value                                                      |
| `boot`             | Number | Counts every measurement cycle. Low boot counts indicate restarts. |
| `bootCount`             | Number | Same as boot property. Required for Home Assistant compatability. Will be depreciated. |
| `ledMode`          | String | Current configuration of the LED mode                              |
| `firmware`  | String | Current firmware version                                           |
| `model`           | String | Current model name                                                 |

Compensated values apply correction algorithms to make the sensor values more accurate. Temperature and relative humidity correction is only applied on the outdoor monitor Open Air but the properties _compensated will still be send also for the indoor monitor AirGradient ONE.

#### Get Configuration Parameters (GET)
With the path "/config" you can get the current configuration.
```json 
{
  "country": "US",
  "pmStandard": "ugm3",
  "ledBarMode": "pm",
  "displayMode": "on",
  "abcDays": 30,
  "tvocLearningOffset": 12,
  "noxLearningOffset": 12,
  "mqttBrokerUrl": "",
  "temperatureUnit": "f",
  "configurationControl": "both",
  "postDataToAirGradient": true
}
```

#### Set Configuration Parameters (PUT)

Configuration parameters can be changed with a put request to the monitor, e.g.

Example to force CO2 calibration

 ```curl -X PUT -H "Content-Type: application/json" -d '{"co2CalibrationRequested":true}' http://airgradient_84fce612eff4.local/config ```

Example to set monitor to Celsius

 ```curl -X PUT -H "Content-Type: application/json" -d '{"temperatureUnit":"c"}' http://airgradient_84fce612eff4.local/config ```

 If you use command prompt on Windows, you need to escape the quotes:
 
  ``` -d "{\"param\":\"value\"}" ```

#### Avoiding Conflicts with Configuration on AirGradient Server
If the monitor is set up on the AirGradient dashboard, it will also receive configurations from there. In case you do not want this, please set `configurationControl` to `local`. In case you set it to `cloud` and want to change it to `local`, you need to make a factory reset. 

#### Configuration Parameters (GET/PUT)

| Properties              | Description                                            | Type    | Accepted Values                                                                                                                         | Example                                       |
|-------------------------|:-------------------------------------------------------|---------|-----------------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------|
| `country`                 | Country where the device is.                           | String  | Country code as [ALPHA-2 notation](https://www.iban.com/country-codes)                                                                  | {"country": "TH"}                             |
| `model`                   | Hardware identifier (only GET).                        | String  | I-9PSL-DE                                                                                                                               | {"model": "I-9PSL-DE"}                        |
| `pmStandard`              | Particle matter standard used on the display.          | String  | `ugm3`: ug/m3 <br> `us-aqi`: USAQI                                                                                                      | {"pmStandard": "ugm3"}                        |
| `ledBarMode`              | Mode in which the led bar can be set.                  | String  | `co2`: LED bar displays CO2 <br>`pm`: LED bar displays PM <br>`off`: Turn off LED bar                                                   | {"ledBarMode": "off"}                         |
| `displayBrightness`       | Brightness of the Display.                             | Number  | 0-100                                                                                                                                   | {"displayBrightness": 50}                     |
| `ledBarBrightness`        | Brightness of the LEDBar.                              | Number  | 0-100                                                                                                                                   | {"ledBarBrightness": 40}                      |
| `abcDays`                 | Number of days for CO2 automatic baseline calibration. | Number  | Maximum 200 days. Default 8 days.                                                                                                       | {"abcDays": 8}                                |
| `mqttBrokerUrl`           | MQTT broker URL.                                       | String  |                                                                                                                                         | {"mqttBrokerUrl": "mqtt://192.168.0.18:1883"} |
| `temperatureUnit`         | Temperature unit shown on the display.                 | String  | `c` or `C`: Degree Celsius °C <br>`f` or `F`: Degree Fahrenheit °F                                                                      | {"temperatureUnit": "c"}                      |
| `configurationControl`    | The configuration source of the device.                | String  | `both`: Accept local and cloud configuration <br>`local`: Accept only local configuration  <br>`cloud`: Accept only cloud configuration | {"configurationControl": "both"}              |
| `postDataToAirGradient`   | Send data to AirGradient cloud.                        | Boolean | `true`: Enabled <br>`false`: Disabled                                                                                                   | {"postDataToAirGradient": true}               |
| `co2CalibrationRequested` | Can be set to trigger a calibration.                   | Boolean | `true`: CO2 calibration (400ppm) will be triggered                                                                                      | {"co2CalibrationRequested": true}             |
| `ledBarTestRequested`     | Can be set to trigger a test.                          | Boolean | `true` : LEDs will run test sequence                                                                                                    | {"ledBarTestRequested": true}                 |
| `noxLearningOffset`       | Set NOx learning gain offset.                          | Number  | 0-720 (default 12)                                                                                                                      | {"noxLearningOffset": 12}                     |
| `tvocLearningOffset`      | Set VOC learning gain offset.                          | Number  | 0-720 (default 12)                                                                                                                      | {"tvocLearningOffset": 12}                    |
| `offlineMode`             | Set monitor to run without WiFi.                       | Boolean |  `false`: Disabled (default) <br> `true`: Enabled                                                                                                              | {"offlineMode": true}                         |
