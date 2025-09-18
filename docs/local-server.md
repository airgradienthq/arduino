## Local Server API

From [firmware version 3.0.10](firmwares) onwards, the AirGradient ONE and Open Air monitors have below API available.

### Discovery

The monitors run a mDNS discovery. So within the same network, the monitor can be accessed through:

http://airgradient_{{serialnumber}}.local


The following requests are possible:

### Get Current Air Quality (GET)

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
  "model": "I-9PSL",
  "monitorDisplayCompensatedValues": true
}
```

| Properties                        | Type    | Explanation                                                                            |
|-----------------------------------|---------|----------------------------------------------------------------------------------------|
| `serialno`                        | String  | Serial Number of the monitor                                                           |
| `wifi`                            | Number  | WiFi signal strength                                                                   |
| `pm01`                            | Number  | PM1.0 in ug/m3 (atmospheric environment)                                               |
| `pm02`                            | Number  | PM2.5 in ug/m3 (atmospheric environment)                                               |
| `pm10`                            | Number  | PM10 in ug/m3 (atmospheric environment)                                                |
| `pm02Compensated`                 | Number  | PM2.5 in ug/m3 with correction applied (from fw version 3.1.4 onwards)                 |
| `pm01Standard`                    | Number  | PM1.0 in ug/m3 (standard particle)                                                     |
| `pm02Standard`                    | Number  | PM2.5 in ug/m3 (standard particle)                                                     |
| `pm10Standard`                    | Number  | PM10 in ug/m3 (standard particle)                                                      |
| `rco2`                            | Number  | CO2 in ppm                                                                             |
| `pm003Count`                      | Number  | Particle count 0.3um per dL                                                            |
| `pm005Count`                      | Number  | Particle count 0.5um per dL                                                            |
| `pm01Count`                       | Number  | Particle count 1.0um per dL                                                            |
| `pm02Count`                       | Number  | Particle count 2.5um per dL                                                            |
| `pm50Count`                       | Number  | Particle count 5.0um per dL (only for indoor monitor)                                  |
| `pm10Count`                       | Number  | Particle count 10um per dL (only for indoor monitor)                                   |
| `atmp`                            | Number  | Temperature in Degrees Celsius                                                         |
| `atmpCompensated`                 | Number  | Temperature in Degrees Celsius with correction applied                                 |
| `rhum`                            | Number  | Relative Humidity                                                                      |
| `rhumCompensated`                 | Number  | Relative Humidity with correction applied                                              |
| `tvocIndex`                       | Number  | Senisiron VOC Index                                                                    |
| `tvocRaw`                         | Number  | VOC raw value                                                                          |
| `noxIndex`                        | Number  | Senisirion NOx Index                                                                   |
| `noxRaw`                          | Number  | NOx raw value                                                                          |
| `boot`                            | Number  | Counts every measurement cycle. Low boot counts indicate restarts.                     |
| `bootCount`                       | Number  | Same as boot property. Required for Home Assistant compatability. (deprecated soon!)   |
| `ledMode`                         | String  | Current configuration of the LED mode                                                  |
| `firmware`                        | String  | Current firmware version                                                               |
| `model`                           | String  | Current model name                                                                     |

Compensated values apply correction algorithms to make the sensor values more accurate. Temperature and relative humidity correction is only applied on the outdoor monitor Open Air but the properties _compensated will still be send also for the indoor monitor AirGradient ONE.

### Get Configuration Parameters (GET)

"/config" path returns the current configuration of the monitor.

```json
{
  "country": "TH",
  "pmStandard": "ugm3",
  "ledBarMode": "pm",
  "abcDays": 7,
  "tvocLearningOffset": 12,
  "noxLearningOffset": 12,
  "mqttBrokerUrl": "",
  "httpDomain": "",
  "temperatureUnit": "c",
  "configurationControl": "local",
  "postDataToAirGradient": true,
  "ledBarBrightness": 100,
  "displayBrightness": 100,
  "offlineMode": false,
  "model": "I-9PSL",
  "monitorDisplayCompensatedValues": true,
  "corrections": {
    "pm02": {
      "correctionAlgorithm": "epa_2021",
      "slr": {}
    }
  }
}
```

### Set Configuration Parameters (PUT)

Configuration parameters can be changed with a PUT request to the monitor, e.g.

Example to force CO2 calibration

 ```bash
 curl -X PUT -H "Content-Type: application/json" -d '{"co2CalibrationRequested":true}' http://airgradient_84fce612eff4.local/config
 ```

Example to set monitor to Celsius

 ```bash
 curl -X PUT -H "Content-Type: application/json" -d '{"temperatureUnit":"c"}' http://airgradient_84fce612eff4.local/config
 ```

 If you use command prompt on Windows, you need to escape the quotes:

  ``` -d "{\"param\":\"value\"}" ```

### Avoiding Conflicts with Configuration on AirGradient Server

If the monitor is set up on the AirGradient dashboard, it will also receive the configuration parameters from there. In case you do not want this, please set `configurationControl` to `local`. In case you set it to `cloud` and want to change it to `local`, you need to make a factory reset.

### Configuration Parameters (GET/PUT)

| Properties                        | Description                                                      | Type    | Accepted Values                                                                                                                         | Example                                         |
|-----------------------------------|:-----------------------------------------------------------------|---------|-----------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------|
| `country`                         | Country where the device is.                                     | String  | Country code as [ALPHA-2 notation](https://www.iban.com/country-codes)                                                                  | `{"country": "TH"}`                             |
| `model`                           | Hardware identifier (only GET).                                  | String  | I-9PSL-DE                                                                                                                               | `{"model": "I-9PSL-DE"}`                        |
| `pmStandard`                      | Particle matter standard used on the display.                    | String  | `ugm3`: ug/m3 <br> `us-aqi`: USAQI                                                                                                      | `{"pmStandard": "ugm3"}`                        |
| `ledBarMode`                      | Mode in which the led bar can be set.                            | String  | `co2`: LED bar displays CO2 <br>`pm`: LED bar displays PM <br>`off`: Turn off LED bar                                                   | `{"ledBarMode": "off"}`                         |
| `displayBrightness`               | Brightness of the Display.                                       | Number  | 0-100                                                                                                                                   | `{"displayBrightness": 50}`                     |
| `ledBarBrightness`                | Brightness of the LEDBar.                                        | Number  | 0-100                                                                                                                                   | `{"ledBarBrightness": 40}`                      |
| `abcDays`                         | Number of days for CO2 automatic baseline calibration.           | Number  | Maximum 200 days. Default 8 days.                                                                                                       | `{"abcDays": 8}`                                |
| `mqttBrokerUrl`                   | MQTT broker URL.                                                 | String  | Maximum 255 characters. Set value to empty string to disable mqtt connection.                                                           | `{"mqttBrokerUrl": "mqtt://192.168.0.18:1883"}` |
| `httpDomain`                      | Domain name for http request. (version > 3.3.2)                  | String  | Maximum 255 characters. Set value to empty string to set http domain to default airgradient                                             | `{"httpDomain": "sub.domain.com"}`              |
| `temperatureUnit`                 | Temperature unit shown on the display.                           | String  | `c` or `C`: Degree Celsius °C <br>`f` or `F`: Degree Fahrenheit °F                                                                      | `{"temperatureUnit": "c"}`                      |
| `configurationControl`            | The configuration source of the device.                          | String  | `both`: Accept local and cloud configuration <br>`local`: Accept only local configuration  <br>`cloud`: Accept only cloud configuration | `{"configurationControl": "both"}`              |
| `postDataToAirGradient`           | Send data to AirGradient cloud.                                  | Boolean | `true`: Enabled <br>`false`: Disabled                                                                                                   | `{"postDataToAirGradient": true}`               |
| `co2CalibrationRequested`         | Can be set to trigger a calibration.                             | Boolean | `true`: CO2 calibration (400ppm) will be triggered                                                                                      | `{"co2CalibrationRequested": true}`             |
| `ledBarTestRequested`             | Can be set to trigger a test.                                    | Boolean | `true` : LEDs will run test sequence                                                                                                    | `{"ledBarTestRequested": true}`                 |
| `noxLearningOffset`               | Set NOx learning gain offset.                                    | Number  | 0-720 (default 12)                                                                                                                      | `{"noxLearningOffset": 12}`                     |
| `tvocLearningOffset`              | Set VOC learning gain offset.                                    | Number  | 0-720 (default 12)                                                                                                                      | `{"tvocLearningOffset": 12}`                    |
| `monitorDisplayCompensatedValues` | Set the display show the PM value with/without compensate  value (only on 3.1.9) | Boolean | `false`: Without compensate (default) <br> `true`: with compensate                                                  | `{"monitorDisplayCompensatedValues": false }`   |
| `corrections`                     | Sets correction options to display and measurement values on local server response. (version >= 3.1.11)    | Object |  _see corrections section_             | _see corrections section_                         |


**Notes**

- `offlineMode` : the device will disable all network operation, and only show measurements on the display and ledbar; Read-Only; Change can be apply using reset button on boot.
- `disableCloudConnection` : disable every request to AirGradient server, means features like post data to AirGradient server, configuration from AirGradient server and automatic firmware updates are disabled. This configuration overrides `configurationControl` and `postDataToAirGradient`; Read-Only; Change can be apply from wifi setup webpage.

### Corrections

The `corrections` object allows configuring PM2.5, Temperature and Humidity correction algorithms and parameters locally. This affects both the display, local server response and open metrics values.

Example correction configuration:

```json
{
  "corrections": {
    "pm02": {
      "correctionAlgorithm": "<Option In String>",
      "slr": {
        "intercept": 0,
        "scalingFactor": 0,
        "useEpa2021": false
      }
    },
    "atmp": {
      "correctionAlgorithm": "<Option In String>",
      "slr": {
        "intercept": 0,
        "scalingFactor": 0
      }
    },
    "rhum": {
      "correctionAlgorithm": "<Option In String>",
      "slr": {
        "intercept": 0,
        "scalingFactor": 0
      }
    },
  }
}
```

#### PM 2.5

Field Name: `pm02`

| Algorithm | Value | Description | SLR required |
|------------|-------------|------|---------|
| Raw | `"none"` | No correction (default) | No |
| EPA 2021 | `"epa_2021"` | Use EPA 2021 correction factors on top of raw value | No |
| PMS5003_20240104 | `"slr_PMS5003_20240104"` | Correction for PMS5003 sensor batch 20240104| Yes |
| PMS5003_20231218 | `"slr_PMS5003_20231218"` | Correction for PMS5003 sensor batch 20231218| Yes |
| PMS5003_20231030 | `"slr_PMS5003_20231030"` | Correction for PMS5003 sensor batch 20231030| Yes |

**NOTES**:

- Set `useEpa2021` to `true` if want to apply EPA 2021 correction factors on top of SLR correction value, otherwise `false`
- `intercept` and `scalingFactor` values can be obtained from [this article](https://www.airgradient.com/blog/low-readings-from-pms5003/)
- If `configurationControl` is set to `local` (eg. when using Home Assistant), correction need to be set manually, see examples below

**Examples**:

- PMS5003_20231030

```bash
curl --location -X PUT 'http://airgradient_84fce612eff4.local/config' --header 'Content-Type: application/json' --data '{"corrections":{"pm02":{"correctionAlgorithm":"slr_PMS5003_20231030","slr":{"intercept":0,"scalingFactor":0.02838,"useEpa2021":true}}}}'
```

- PMS5003_20231218

```bash
curl --location -X PUT 'http://airgradient_84fce612eff4.local/config' --header 'Content-Type: application/json' --data '{"corrections":{"pm02":{"correctionAlgorithm":"slr_PMS5003_20231218","slr":{"intercept":0,"scalingFactor":0.03525,"useEpa2021":true}}}}'
```

- PMS5003_20240104

```bash
curl --location -X PUT 'http://airgradient_84fce612eff4.local/config' --header 'Content-Type: application/json' --data '{"corrections":{"pm02":{"correctionAlgorithm":"slr_PMS5003_20240104","slr":{"intercept":0,"scalingFactor":0.02896,"useEpa2021":true}}}}'
```

#### Temperature & Humidity

Field Name:
- Temperature: `atmp`
- Humidity: `rhum`

| Algorithm | Value | Description | SLR required |
|------------|-------------|------|---------|
| Raw | `"none"` | No correction (default) | No |
| AirGradient Standard Correction | `"ag_pms5003t_2024"` | Using standard airgradient correction (for outdoor monitor)| No |
| Custom | `"custom"` | custom corrections constant, set `intercept` and `scalingFactor` manually | Yes |

*Table above apply for both Temperature and Humidity*

**Example**

```bash
curl --location -X PUT 'http://airgradient_84fce612eff4.local/config' --header 'Content-Type: application/json' --data '{"corrections":{"atmp":{"correctionAlgorithm":"custom","slr":{"intercept":0.2,"scalingFactor":1.1}}}}'
```