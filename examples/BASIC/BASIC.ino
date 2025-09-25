/*
This is the code for the AirGradient DIY BASIC Air Quality Monitor with an D1
ESP8266 Microcontroller.

It is an air quality monitor for PM2.5, CO2, Temperature and Humidity with a
small display and can send data over Wifi.

Open source air quality monitors and kits are available:
Indoor Monitor: https://www.airgradient.com/indoor/
Outdoor Monitor: https://www.airgradient.com/outdoor/

Build Instructions:
https://www.airgradient.com/documentation/diy-v4/

Compile Instructions:
https://github.com/airgradienthq/arduino/blob/master/docs/howto-compile.md

Configuration parameters, e.g. Celsius / Fahrenheit or PM unit (US AQI vs ug/m3)
can be set through the AirGradient dashboard.

If you have any questions please visit our forum at
https://forum.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include "AgApiClient.h"
#include "AgConfigure.h"
#include "AgSchedule.h"
#include "AgWiFiConnector.h"
#include "LocalServer.h"
#include "OpenMetrics.h"
#include "MqttClient.h"
#include <AirGradient.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

#define LED_BAR_ANIMATION_PERIOD 100                  /** ms */
#define DISP_UPDATE_INTERVAL 2500                     /** ms */
#define SERVER_CONFIG_SYNC_INTERVAL 60000             /** ms */
#define SERVER_SYNC_INTERVAL 60000                    /** ms */
#define MQTT_SYNC_INTERVAL 60000                      /** ms */
#define SENSOR_CO2_CALIB_COUNTDOWN_MAX 5              /** sec */
#define SENSOR_TVOC_UPDATE_INTERVAL 1000              /** ms */
#define SENSOR_CO2_UPDATE_INTERVAL 4000               /** ms */
#define SENSOR_PM_UPDATE_INTERVAL 2000                /** ms */
#define SENSOR_TEMP_HUM_UPDATE_INTERVAL 6000          /** ms */
#define DISPLAY_DELAY_SHOW_CONTENT_MS 2000            /** ms */

static AirGradient ag(DIY_BASIC);
static Configuration configuration(Serial);
static AgApiClient apiClient(Serial, configuration);
static Measurements measurements(configuration);
static OledDisplay oledDisplay(configuration, measurements, Serial);
static StateMachine stateMachine(oledDisplay, Serial, measurements,
                                 configuration);
static WifiConnector wifiConnector(oledDisplay, Serial, stateMachine,
                                   configuration);
static OpenMetrics openMetrics(measurements, configuration, wifiConnector,
                               apiClient);
static LocalServer localServer(Serial, openMetrics, measurements, configuration,
                               wifiConnector);
static MqttClient mqttClient(Serial);

static AgFirmwareMode fwMode = FW_MODE_I_BASIC_40PS;

static String fwNewVersion;

static void boardInit(void);
static void failedHandler(String msg);
static void configurationUpdateSchedule(void);
static void appDispHandler(void);
static void oledDisplaySchedule(void);
static void updateTvoc(void);
static void updatePm(void);
static void sendDataToServer(void);
static void tempHumUpdate(void);
static void co2Update(void);
static void mdnsInit(void);
static void initMqtt(void);
static void factoryConfigReset(void);
static void wdgFeedUpdate(void);
static bool sgp41Init(void);
static void wifiFactoryConfigure(void);
static void mqttHandle(void);
static int calculateMaxPeriod(int updateInterval);
static void setMeasurementMaxPeriod();

AgSchedule dispLedSchedule(DISP_UPDATE_INTERVAL, oledDisplaySchedule);
AgSchedule configSchedule(SERVER_CONFIG_SYNC_INTERVAL,
                          configurationUpdateSchedule);
AgSchedule agApiPostSchedule(SERVER_SYNC_INTERVAL, sendDataToServer);
AgSchedule co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Update);
AgSchedule pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, updatePm);
AgSchedule tempHumSchedule(SENSOR_TEMP_HUM_UPDATE_INTERVAL, tempHumUpdate);
AgSchedule tvocSchedule(SENSOR_TVOC_UPDATE_INTERVAL, updateTvoc);
AgSchedule watchdogFeedSchedule(60000, wdgFeedUpdate);
AgSchedule mqttSchedule(MQTT_SYNC_INTERVAL, mqttHandle);

void setup() {
  /** Serial for print debug message */
  Serial.begin(115200);
  delay(100); /** For bester show log */

  /** Print device ID into log */
  Serial.println("Serial nr: " + ag.deviceId());

  /** Initialize local configure */
  configuration.begin();

  /** Init I2C */
  Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin());
  delay(1000);

  configuration.setAirGradient(&ag);
  oledDisplay.setAirGradient(&ag);
  stateMachine.setAirGradient(&ag);
  wifiConnector.setAirGradient(&ag);
  apiClient.setAirGradient(&ag);
  openMetrics.setAirGradient(&ag);
  localServer.setAirGraident(&ag);
  measurements.setAirGradient(&ag);

  /** Example set custom API root URL */
  // apiClient.setApiRoot("https://example.custom.api");

  /** Init sensor */
  boardInit();
  setMeasurementMaxPeriod();

  // Uncomment below line to print every measurements reading update
  // measurements.setDebug(true);

  /** Connecting wifi */
  bool connectToWifi = false;

  connectToWifi = !configuration.isOfflineMode();
  if (connectToWifi) {
    apiClient.begin();

    if (wifiConnector.connect()) {
      if (wifiConnector.isConnected()) {
        mdnsInit();
        localServer.begin();
        initMqtt();
        sendDataToAg();

        if (configuration.getConfigurationControl() !=
            ConfigurationControl::ConfigurationControlLocal) {
          apiClient.fetchServerConfiguration();
        }
        configSchedule.update();
        if (apiClient.isFetchConfigurationFailed()) {
          if (apiClient.isNotAvailableOnDashboard()) {
            stateMachine.displaySetAddToDashBoard();
            stateMachine.displayHandle(
                AgStateMachineWiFiOkServerOkSensorConfigFailed);
          } else {
            stateMachine.displayClearAddToDashBoard();
          }
          delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
        }
      } else {
        if (wifiConnector.isConfigurePorttalTimeout()) {
          oledDisplay.showRebooting();
          delay(2500);
          oledDisplay.setText("", "", "");
          ESP.restart();
        }
      }
    }
  }
  /** Set offline mode without saving, cause wifi is not configured */
  if (wifiConnector.hasConfigurated() == false) {
    Serial.println("Set offline mode cause wifi is not configurated");
    configuration.setOfflineModeWithoutSave(true);
  }

  /** Show display Warning up */
  String sn = "SN:" + ag.deviceId();
  oledDisplay.setText("Warming Up", sn.c_str(), "");

  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

  Serial.println("Display brightness: " +
                 String(configuration.getDisplayBrightness()));
  oledDisplay.setBrightness(configuration.getDisplayBrightness());

  appDispHandler();
}

void loop() {
  /** Handle schedule */
  dispLedSchedule.run();
  configSchedule.run();
  agApiPostSchedule.run();

  if (configuration.hasSensorS8) {
    co2Schedule.run();
  }
  if (configuration.hasSensorPMS1) {
    pmsSchedule.run();
    ag.pms5003.handle();
  }
  if (configuration.hasSensorSHT) {
    tempHumSchedule.run();
  }
  if (configuration.hasSensorSGP) {
    tvocSchedule.run();
  }

  watchdogFeedSchedule.run();

  /** Check for handle WiFi reconnect */
  wifiConnector.handle();

  /** factory reset handle */
  // factoryConfigReset();

  /** check that local configura changed then do some action */
  configUpdateHandle();

  localServer._handle();

  if (configuration.hasSensorSGP) {
    ag.sgp41.handle();
  }

  MDNS.update();

  mqttSchedule.run();
  mqttClient.handle();
}

static void co2Update(void) {
  if (!configuration.hasSensorS8) {
    // Device don't have S8 sensor
    return;
  }

  int value = ag.s8.getCo2();
  if (utils::isValidCO2(value)) {
    measurements.update(Measurements::CO2, value);
  } else {
    measurements.update(Measurements::CO2, utils::getInvalidCO2());
  }
}

static void mdnsInit(void) {
  Serial.println("mDNS init");
  if (!MDNS.begin(localServer.getHostname().c_str())) {
    Serial.println("Init mDNS failed");
    return;
  }

  MDNS.addService("_airgradient", "_tcp", 80);
  MDNS.addServiceTxt("_airgradient", "_tcp", "model",
                     AgFirmwareModeName(fwMode));
  MDNS.addServiceTxt("_airgradient", "_tcp", "serialno", ag.deviceId());
  MDNS.addServiceTxt("_airgradient", "_tcp", "fw_ver", ag.getVersion());
  MDNS.addServiceTxt("_airgradient", "_tcp", "vendor", "AirGradient");

  MDNS.announce();
}

static void initMqtt(void) {
  String mqttUri = configuration.getMqttBrokerUri();
  if (mqttUri.isEmpty()) {
    Serial.println(
        "MQTT is not configured, skipping initialization of MQTT client");
    return;
  }

  if (mqttClient.begin(mqttUri)) {
    Serial.println("Successfully connected to MQTT broker");
  } else {
    Serial.println("Connection to MQTT broker failed");
  }
}

static void wdgFeedUpdate(void) {
  ag.watchdog.reset();
  Serial.println("External watchdog feed!");
}

static bool sgp41Init(void) {
  ag.sgp41.setNoxLearningOffset(configuration.getNoxLearningOffset());
  ag.sgp41.setTvocLearningOffset(configuration.getTvocLearningOffset());
  if (ag.sgp41.begin(Wire)) {
    Serial.println("Init SGP41 success");
    configuration.hasSensorSGP = true;
    return true;
  } else {
    Serial.println("Init SGP41 failure");
    configuration.hasSensorSGP = false;
  }
  return false;
}

static void wifiFactoryConfigure(void) {
  WiFi.persistent(true);
  WiFi.begin("airgradient", "cleanair");
  WiFi.persistent(false);
  oledDisplay.setText("Configure WiFi", "connect to", "\'airgradient\'");
  delay(2500);
  oledDisplay.setText("Rebooting...", "", "");
  delay(2500);
  oledDisplay.setText("", "", "");
  ESP.restart();
}

static void mqttHandle(void) {
  if(mqttClient.isConnected() == false) {
    mqttClient.connect(String("airgradient-") + ag.deviceId());
  }

  if (mqttClient.isConnected()) {
    String payload = measurements.toString(true, fwMode, wifiConnector.RSSI());
    String topic = "airgradient/readings/" + ag.deviceId();
    if (mqttClient.publish(topic.c_str(), payload.c_str(), payload.length())) {
      Serial.println("MQTT sync success");
    } else {
      Serial.println("MQTT sync failure");
    }
  }
}

static void sendDataToAg() {
  /** Change oledDisplay and led state */
  stateMachine.displayHandle(AgStateMachineWiFiOkServerConnecting);

  delay(1500);
  if (apiClient.sendPing(wifiConnector.RSSI(), measurements.bootCount())) {
    stateMachine.displayHandle(AgStateMachineWiFiOkServerConnected);
  } else {
    stateMachine.displayHandle(AgStateMachineWiFiOkServerConnectFailed);
  }
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
}

void dispSensorNotFound(String ss) {
  oledDisplay.setText("Sensor", ss.c_str(), "not found");
  delay(2000);
}

static void boardInit(void) {
  /** Display init */
  oledDisplay.begin();

  /** Show boot display */
  Serial.println("Firmware Version: " + ag.getVersion());

  if (ag.isBasic()) {
    oledDisplay.setText("DIY Basic", ag.getVersion().c_str(), "");
  } else {
    oledDisplay.setText("AirGradient ONE",
                        "FW Version: ", ag.getVersion().c_str());
  }

  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

  ag.watchdog.begin();

  /** Show message init sensor */
  oledDisplay.setText("Sensor", "init...", "");

  /** Init sensor SGP41 */
  configuration.hasSensorSGP = false;
  // if (sgp41Init() == false) {
  //   dispSensorNotFound("SGP41");
  // }

  /** Init SHT */
  if (ag.sht.begin(Wire) == false) {
    Serial.println("SHTx sensor not found");
    configuration.hasSensorSHT = false;
    dispSensorNotFound("SHT");
  }

  /** Init S8 CO2 sensor */
  if (ag.s8.begin(&Serial) == false) {
    Serial.println("CO2 S8 sensor not found");
    configuration.hasSensorS8 = false;
    dispSensorNotFound("S8");
  }

  /** Init PMS5003 */
  configuration.hasSensorPMS1 = true;
  configuration.hasSensorPMS2 = false;
  if (ag.pms5003.begin(&Serial) == false) {
    Serial.println("PMS sensor not found");
    configuration.hasSensorPMS1 = false;

    dispSensorNotFound("PMS");
  }

  /** Set S8 CO2 abc days period */
  if (configuration.hasSensorS8) {
    if (ag.s8.setAbcPeriod(configuration.getCO2CalibrationAbcDays() * 24)) {
      Serial.println("Set S8 AbcDays successful");
    } else {
      Serial.println("Set S8 AbcDays failure");
    }
  }

  localServer.setFwMode(fwMode);
}

static void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}

static void configurationUpdateSchedule(void) {
  if (configuration.isOfflineMode() ||
      configuration.getConfigurationControl() == ConfigurationControl::ConfigurationControlLocal) {
    Serial.println("Ignore fetch server configuration. Either mode is offline "
                   "or configurationControl set to local");
    apiClient.resetFetchConfigurationStatus();
    return;
  }

  if (apiClient.fetchServerConfiguration()) {
    configUpdateHandle();
  }
}

static void configUpdateHandle() {
  if (configuration.isUpdated() == false) {
    return;
  }

  stateMachine.executeCo2Calibration();

  String mqttUri = configuration.getMqttBrokerUri();
  if (mqttClient.isCurrentUri(mqttUri) == false) {
    mqttClient.end();
    initMqtt();
  }

  if (configuration.hasSensorSGP) {
    if (configuration.noxLearnOffsetChanged() ||
        configuration.tvocLearnOffsetChanged()) {
      ag.sgp41.end();

      int oldTvocOffset = ag.sgp41.getTvocLearningOffset();
      int oldNoxOffset = ag.sgp41.getNoxLearningOffset();
      bool result = sgp41Init();
      const char *resultStr = "successful";
      if (!result) {
        resultStr = "failure";
      }
      if (oldTvocOffset != configuration.getTvocLearningOffset()) {
        Serial.printf("Setting tvocLearningOffset from %d to %d hours %s\r\n",
                      oldTvocOffset, configuration.getTvocLearningOffset(),
                      resultStr);
      }
      if (oldNoxOffset != configuration.getNoxLearningOffset()) {
        Serial.printf("Setting noxLearningOffset from %d to %d hours %s\r\n",
                      oldNoxOffset, configuration.getNoxLearningOffset(),
                      resultStr);
      }
    }
  }

  if (configuration.isDisplayBrightnessChanged()) {
    oledDisplay.setBrightness(configuration.getDisplayBrightness());
  }

  appDispHandler();
}

static void appDispHandler(void) {
  AgStateMachineState state = AgStateMachineNormal;

  /** Only show display status on online mode. */
  if (configuration.isOfflineMode() == false) {
    if (wifiConnector.isConnected() == false) {
      state = AgStateMachineWiFiLost;
    } else if (apiClient.isFetchConfigurationFailed()) {
      state = AgStateMachineSensorConfigFailed;
      if (apiClient.isNotAvailableOnDashboard()) {
        stateMachine.displaySetAddToDashBoard();
      } else {
        stateMachine.displayClearAddToDashBoard();
      }
    } else if (apiClient.isPostToServerFailed()) {
      state = AgStateMachineServerLost;
    }
  }
  stateMachine.displayHandle(state);
}

static void oledDisplaySchedule(void) {

  appDispHandler();
}

static void updateTvoc(void) {
  if (!configuration.hasSensorSGP) {
    return;
  }

  measurements.update(Measurements::TVOC, ag.sgp41.getTvocIndex());
  measurements.update(Measurements::TVOCRaw, ag.sgp41.getTvocRaw());
  measurements.update(Measurements::NOx, ag.sgp41.getNoxIndex());
  measurements.update(Measurements::NOxRaw, ag.sgp41.getNoxRaw());
}

static void updatePm(void) {
  if (ag.pms5003.connected()) {
    measurements.update(Measurements::PM01, ag.pms5003.getPm01Ae());
    measurements.update(Measurements::PM25, ag.pms5003.getPm25Ae());
    measurements.update(Measurements::PM10, ag.pms5003.getPm10Ae());
    measurements.update(Measurements::PM03_PC, ag.pms5003.getPm03ParticleCount());
  } else {
    measurements.update(Measurements::PM01, utils::getInvalidPmValue());
    measurements.update(Measurements::PM25, utils::getInvalidPmValue());
    measurements.update(Measurements::PM10, utils::getInvalidPmValue());
    measurements.update(Measurements::PM03_PC, utils::getInvalidPmValue());
  }
}

static void sendDataToServer(void) {
  /** Increment bootcount when send measurements data is scheduled */
  int bootCount = measurements.bootCount() + 1;
  measurements.setBootCount(bootCount);

  if (configuration.isOfflineMode() || !configuration.isPostDataToAirGradient()) {
    Serial.println("Skipping transmission of data to AG server. Either mode is offline "
                   "or post data to server disabled");
    return;
  }

  if (wifiConnector.isConnected() == false) {
    Serial.println("WiFi not connected, skipping data transmission to AG server");
    return;
  }

  String syncData = measurements.toString(false, fwMode, wifiConnector.RSSI());
  if (apiClient.postToServer(syncData)) {
    Serial.println();
    Serial.println("Online mode and isPostToAirGradient = true");
    Serial.println();
  }
}

static void tempHumUpdate(void) {
  if (ag.sht.measure()) {
    float temp = ag.sht.getTemperature();
    float rhum = ag.sht.getRelativeHumidity();

    measurements.update(Measurements::Temperature, temp);
    measurements.update(Measurements::Humidity, rhum);

    // Update compensation temperature and humidity for SGP41
    if (configuration.hasSensorSGP) {
      ag.sgp41.setCompensationTemperatureHumidity(temp, rhum);
    }
  } else {
    measurements.update(Measurements::Temperature, utils::getInvalidTemperature());
    measurements.update(Measurements::Humidity, utils::getInvalidHumidity());
    Serial.println("SHT read failed");
  }
}

/* Set max period for each measurement type based on sensor update interval*/
void setMeasurementMaxPeriod() {
  /// Max period for S8 sensors measurements
  measurements.maxPeriod(Measurements::CO2, calculateMaxPeriod(SENSOR_CO2_UPDATE_INTERVAL));
  /// Max period for SGP sensors measurements
  measurements.maxPeriod(Measurements::TVOC, calculateMaxPeriod(SENSOR_TVOC_UPDATE_INTERVAL));
  measurements.maxPeriod(Measurements::TVOCRaw, calculateMaxPeriod(SENSOR_TVOC_UPDATE_INTERVAL));
  measurements.maxPeriod(Measurements::NOx, calculateMaxPeriod(SENSOR_TVOC_UPDATE_INTERVAL));
  measurements.maxPeriod(Measurements::NOxRaw, calculateMaxPeriod(SENSOR_TVOC_UPDATE_INTERVAL));
  /// Max period for PMS sensors measurements
  measurements.maxPeriod(Measurements::PM25, calculateMaxPeriod(SENSOR_PM_UPDATE_INTERVAL));
  measurements.maxPeriod(Measurements::PM01, calculateMaxPeriod(SENSOR_PM_UPDATE_INTERVAL));
  measurements.maxPeriod(Measurements::PM10, calculateMaxPeriod(SENSOR_PM_UPDATE_INTERVAL));
  measurements.maxPeriod(Measurements::PM03_PC, calculateMaxPeriod(SENSOR_PM_UPDATE_INTERVAL));
  // Temperature and Humidity
  if (configuration.hasSensorSHT) {
    /// Max period for SHT sensors measurements
    measurements.maxPeriod(Measurements::Temperature,
                           calculateMaxPeriod(SENSOR_TEMP_HUM_UPDATE_INTERVAL));
    measurements.maxPeriod(Measurements::Humidity,
                           calculateMaxPeriod(SENSOR_TEMP_HUM_UPDATE_INTERVAL));
  } else {
    /// Temp and hum data retrieved from PMS5003T sensor
    measurements.maxPeriod(Measurements::Temperature,
                           calculateMaxPeriod(SENSOR_PM_UPDATE_INTERVAL));
    measurements.maxPeriod(Measurements::Humidity, calculateMaxPeriod(SENSOR_PM_UPDATE_INTERVAL));
  }
}

int calculateMaxPeriod(int updateInterval) {
  // 0.5 is 50% reduced interval for max period
  return (SERVER_SYNC_INTERVAL - (SERVER_SYNC_INTERVAL * 0.5)) / updateInterval;
}