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

Please make sure you have esp8266 board manager installed. Tested with
version 3.1.2.

Set board to "LOLIN(WEMOS) D1 R2 & mini"

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
#define SENSOR_TEMP_HUM_UPDATE_INTERVAL 2000          /** ms */
#define DISPLAY_DELAY_SHOW_CONTENT_MS 2000            /** ms */
#define FIRMWARE_CHECK_FOR_UPDATE_MS (60 * 60 * 1000) /** ms */

static AirGradient ag(DIY_BASIC);
static Configuration configuration(Serial);
static AgApiClient apiClient(Serial, configuration);
static Measurements measurements;
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

static int getCO2FailCount = 0;
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

  /** Example set custom API root URL */
  // apiClient.setApiRoot("https://example.custom.api");

  /** Init sensor */
  boardInit();

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

        apiClient.fetchServerConfiguration();
        configSchedule.update();
        if (apiClient.isFetchConfigureFailed()) {
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

  /** Auto reset watchdog timer if offline mode or postDataToAirGradient */
  if (configuration.isOfflineMode() ||
      (configuration.isPostDataToAirGradient() == false)) {
    watchdogFeedSchedule.run();
  }

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
  int value = ag.s8.getCo2();
  if (utils::isValidCO2(value)) {
    measurements.CO2 = value;
    getCO2FailCount = 0;
    Serial.printf("CO2 (ppm): %d\r\n", measurements.CO2);
  } else {
    getCO2FailCount++;
    Serial.printf("Get CO2 failed: %d\r\n", getCO2FailCount);
    if (getCO2FailCount >= 3) {
      measurements.CO2 = utils::getInvalidCO2();
    }
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
  Serial.println();
  Serial.println("Offline mode or isPostToAirGradient = false: watchdog reset");
  Serial.println();
}

static bool sgp41Init(void) {
  ag.sgp41.setNoxLearningOffset(configuration.getNoxLearningOffset());
  ag.sgp41.setTvocLearningOffset(configuration.getTvocLearningOffset());
  if (ag.sgp41.begin(Wire)) {
    Serial.println("Init SGP41 success");
    configuration.hasSensorSGP = true;
    return true;
  } else {
    Serial.println("Init SGP41 failuire");
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
    String payload = measurements.toString(true, fwMode, wifiConnector.RSSI(),
                                           &ag, &configuration);
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
  if (apiClient.sendPing(wifiConnector.RSSI(), measurements.bootCount)) {
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
    } else if (apiClient.isFetchConfigureFailed()) {
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
  measurements.TVOC = ag.sgp41.getTvocIndex();
  measurements.TVOCRaw = ag.sgp41.getTvocRaw();
  measurements.NOx = ag.sgp41.getNoxIndex();
  measurements.NOxRaw = ag.sgp41.getNoxRaw();

  Serial.println();
  Serial.printf("TVOC index: %d\r\n", measurements.TVOC);
  Serial.printf("TVOC raw: %d\r\n", measurements.TVOCRaw);
  Serial.printf("NOx index: %d\r\n", measurements.NOx);
  Serial.printf("NOx raw: %d\r\n", measurements.NOxRaw);
}

static void updatePm(void) {
  if (ag.pms5003.isFailed() == false) {
    measurements.pm01_1 = ag.pms5003.getPm01Ae();
    measurements.pm25_1 = ag.pms5003.getPm25Ae();
    measurements.pm10_1 = ag.pms5003.getPm10Ae();
    measurements.pm03PCount_1 = ag.pms5003.getPm03ParticleCount();

    Serial.println();
    Serial.printf("PM1 ug/m3: %d\r\n", measurements.pm01_1);
    Serial.printf("PM2.5 ug/m3: %d\r\n", measurements.pm25_1);
    Serial.printf("PM10 ug/m3: %d\r\n", measurements.pm10_1);
    Serial.printf("PM0.3 Count: %d\r\n", measurements.pm03PCount_1);
    Serial.printf("PM firmware version: %d\r\n", ag.pms5003.getFirmwareVersion());
    ag.pms5003.resetFailCount();
  } else {
    ag.pms5003.updateFailCount();
    Serial.printf("PMS read failed %d times\r\n", ag.pms5003.getFailCount());
    if (ag.pms5003.getFailCount() >= PMS_FAIL_COUNT_SET_INVALID) {
      measurements.pm01_1 = utils::getInvalidPmValue();
      measurements.pm25_1 = utils::getInvalidPmValue();
      measurements.pm10_1 = utils::getInvalidPmValue();
      measurements.pm03PCount_1 = utils::getInvalidPmValue();
    }

    if(ag.pms5003.getFailCount() >= ag.pms5003.getFailCountMax()) {
      Serial.printf("PMS failure count reach to max set %d, restarting...", ag.pms5003.getFailCountMax());
      ESP.restart();
    }
  }
}

static void sendDataToServer(void) {
  /** Ignore send data to server if postToAirGradient disabled */
  if (configuration.isPostDataToAirGradient() == false ||
      configuration.isOfflineMode()) {
    return;
  }

  String syncData = measurements.toString(false, fwMode, wifiConnector.RSSI(),
                                          &ag, &configuration);
  if (apiClient.postToServer(syncData)) {
    ag.watchdog.reset();
    Serial.println();
    Serial.println(
        "Online mode and isPostToAirGradient = true: watchdog reset");
    Serial.println();
  }

  measurements.bootCount++;
}

static void tempHumUpdate(void) {
  delay(100);
  if (ag.sht.measure()) {
    measurements.Temperature = ag.sht.getTemperature();
    measurements.Humidity = ag.sht.getRelativeHumidity();

    Serial.printf("Temperature in C: %0.2f\r\n", measurements.Temperature);
    Serial.printf("Relative Humidity: %d\r\n", measurements.Humidity);
    Serial.printf("Temperature compensated in C: %0.2f\r\n",
                  measurements.Temperature);
    Serial.printf("Relative Humidity compensated: %d\r\n",
                  measurements.Humidity);

    // Update compensation temperature and humidity for SGP41
    if (configuration.hasSensorSGP) {
      ag.sgp41.setCompensationTemperatureHumidity(measurements.Temperature,
                                                  measurements.Humidity);
    }
  } else {
    Serial.println("SHT read failed");
    measurements.Temperature = utils::getInvalidTemperature();
    measurements.Humidity = utils::getInvalidHumidity();
  }
}
