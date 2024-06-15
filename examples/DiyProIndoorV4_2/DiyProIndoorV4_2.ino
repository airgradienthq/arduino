#include "AgApiClient.h"
#include "AgConfigure.h"
#include "AgSchedule.h"
#include "AgWiFiConnector.h"
#include "LocalServer.h"
#include "OpenMetrics.h"
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

static AirGradient ag(DIY_PRO_INDOOR_V4_2);
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

static int pmFailCount = 0;
static uint32_t factoryBtnPressTime = 0;
static int getCO2FailCount = 0;
static AgFirmwareMode fwMode = FW_MODE_I_8PSL;

static bool ledBarButtonTest = false;
static String fwNewVersion;

static void boardInit(void);
static void failedHandler(String msg);
static void configurationUpdateSchedule(void);
static void appLedHandler(void);
static void appDispHandler(void);
static void oledDisplayLedBarSchedule(void);
static void updateTvoc(void);
static void updatePm(void);
static void sendDataToServer(void);
static void tempHumUpdate(void);
static void co2Update(void);
static void mdnsInit(void);
static void createMqttTask(void);
static void initMqtt(void);
static void factoryConfigReset(void);
static void wdgFeedUpdate(void);
static void ledBarEnabledUpdate(void);
static bool sgp41Init(void);
static void wifiFactoryConfigure(void);

AgSchedule dispLedSchedule(DISP_UPDATE_INTERVAL, oledDisplayLedBarSchedule);
AgSchedule configSchedule(SERVER_CONFIG_SYNC_INTERVAL,
                          configurationUpdateSchedule);
AgSchedule agApiPostSchedule(SERVER_SYNC_INTERVAL, sendDataToServer);
AgSchedule co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Update);
AgSchedule pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, updatePm);
AgSchedule tempHumSchedule(SENSOR_TEMP_HUM_UPDATE_INTERVAL, tempHumUpdate);
AgSchedule tvocSchedule(SENSOR_TVOC_UPDATE_INTERVAL, updateTvoc);
AgSchedule watchdogFeedSchedule(60000, wdgFeedUpdate);

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

  /** Init sensor */
  boardInit();

  /** Connecting wifi */
  bool connectToWifi = false;
  if (ag.isOne() || ag.getBoardType() == DIY_PRO_INDOOR_V4_2) {
    /** Show message confirm offline mode, should me perform if LED bar button
     * test pressed */
    if (ledBarButtonTest == false) {
      oledDisplay.setText(
          "Press now for",
          configuration.isOfflineMode() ? "online mode" : "offline mode", "");
      uint32_t startTime = millis();
      while (true) {
        if (ag.button.getState() == ag.button.BUTTON_PRESSED) {
          configuration.setOfflineMode(!configuration.isOfflineMode());

          oledDisplay.setText(
              "Offline Mode",
              configuration.isOfflineMode() ? " = True" : "  = False", "");
          delay(1000);
          break;
        }
        uint32_t periodMs = (uint32_t)(millis() - startTime);
        if (periodMs >= 3000) {
          Serial.println("Set for offline mode timeout");
          break;
        }

        delay(1);
      }
      connectToWifi = !configuration.isOfflineMode();
    } else {
      configuration.setOfflineModeWithoutSave(true);
    }
  } else {
    connectToWifi = true;
  }

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
          if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
            if (apiClient.isNotAvailableOnDashboard()) {
              stateMachine.displaySetAddToDashBoard();
              stateMachine.displayHandle(
                  AgStateMachineWiFiOkServerOkSensorConfigFailed);
            } else {
              stateMachine.displayClearAddToDashBoard();
            }
          }
          stateMachine.handleLeds(
              AgStateMachineWiFiOkServerOkSensorConfigFailed);
          delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
        } else {
          ledBarEnabledUpdate();
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
  if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
    oledDisplay.setText("Warming Up", "Serial Number:", ag.deviceId().c_str());
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

    Serial.println("Display brightness: " +
                   String(configuration.getDisplayBrightness()));
    oledDisplay.setBrightness(configuration.getDisplayBrightness());
  }

  appLedHandler();
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
  if (configuration.hasSensorPMS) {
    pmsSchedule.run();
    ag.pms5003.handle();
  }
  if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
    if (configuration.hasSensorSHT) {
      tempHumSchedule.run();
    }
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
  factoryConfigReset();

  /** check that local configura changed then do some action */
  configUpdateHandle();

  localServer._handle();

  if (ag.getBoardType() == DIY_PRO_INDOOR_V4_2) {
    ag.sgp41.handle();
  }
}

static void co2Update(void) {
  int value = ag.s8.getCo2();
  if (value >= 0) {
    measurements.CO2 = value;
    getCO2FailCount = 0;
    Serial.printf("CO2 (ppm): %d\r\n", measurements.CO2);
  } else {
    getCO2FailCount++;
    Serial.printf("Get CO2 failed: %d\r\n", getCO2FailCount);
    if (getCO2FailCount >= 3) {
      measurements.CO2 = -1;
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
}

static void createMqttTask(void) {
  // if (mqttTask) {
  //   vTaskDelete(mqttTask);
  //   mqttTask = NULL;
  //   Serial.println("Delete old MQTT task");
  // }

  // Serial.println("Create new MQTT task");
  // xTaskCreate(
  //     [](void *param) {
  //       for (;;) {
  //         delay(MQTT_SYNC_INTERVAL);

  //         /** Send data */
  //         if (mqttClient.isConnected()) {
  //           String payload = measurements.toString(
  //               true, fwMode, wifiConnector.RSSI(), ag, &configuration);
  //           String topic = "airgradient/readings/" + ag.deviceId();

  //           if (mqttClient.publish(topic.c_str(), payload.c_str(),
  //                                  payload.length())) {
  //             Serial.println("MQTT sync success");
  //           } else {
  //             Serial.println("MQTT sync failure");
  //           }
  //         }
  //       }
  //     },
  //     "mqtt-task", 1024 * 4, NULL, 6, &mqttTask);

  // if (mqttTask == NULL) {
  //   Serial.println("Creat mqttTask failed");
  // }
}

static void initMqtt(void) {
  // if (mqttClient.begin(configuration.getMqttBrokerUri())) {
  //   Serial.println("Connect to MQTT broker successful");
  //   createMqttTask();
  // } else {
  //   Serial.println("Connect to MQTT broker failed");
  // }
}

static void factoryConfigReset(void) {
  if (ag.button.getState() == ag.button.BUTTON_PRESSED) {
    if (factoryBtnPressTime == 0) {
      factoryBtnPressTime = millis();
    } else {
      uint32_t ms = (uint32_t)(millis() - factoryBtnPressTime);
      if (ms >= 2000) {
        // Show display message: For factory keep for x seconds
        if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
          oledDisplay.setText("Factory reset", "keep pressed", "for 8 sec");
        } else {
          Serial.println("Factory reset, keep pressed for 8 sec");
        }

        int count = 7;
        while (ag.button.getState() == ag.button.BUTTON_PRESSED) {
          delay(1000);
          if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {

            String str = "for " + String(count) + " sec";
            oledDisplay.setText("Factory reset", "keep pressed", str.c_str());
          } else {
            Serial.printf("Factory reset, keep pressed for %d sec\r\n", count);
          }
          count--;
          if (count == 0) {
            /** Stop MQTT task first */
            // if (mqttTask) {
            //   vTaskDelete(mqttTask);
            //   mqttTask = NULL;
            // }

            /** Reset WIFI */
            WiFi.enableSTA(true); // Incase offline mode
            WiFi.disconnect(true, true);

            /** Reset local config */
            configuration.reset();

            if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
              oledDisplay.setText("Factory reset", "successful", "");
            } else {
              Serial.println("Factory reset successful");
            }
            delay(3000);
            oledDisplay.setText("", "", "");
            ESP.restart();
          }
        }

        /** Show current content cause reset ignore */
        factoryBtnPressTime = 0;
        if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
          appDispHandler();
        }
      }
    }
  } else {
    if (factoryBtnPressTime != 0) {
      if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
        /** Restore last display content */
        appDispHandler();
      }
    }
    factoryBtnPressTime = 0;
  }
}

static void wdgFeedUpdate(void) {
  ag.watchdog.reset();
  Serial.println();
  Serial.println("Offline mode or isPostToAirGradient = false: watchdog reset");
  Serial.println();
}

static void ledBarEnabledUpdate(void) {
  if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
    int brightness = configuration.getLedBarBrightness();
    Serial.println("LED bar brightness: " + String(brightness));
    if ((brightness == 0) || (configuration.getLedBarMode() == LedBarModeOff)) {
      ag.ledBar.setEnable(false);
    } else {
      ag.ledBar.setBrightness(brightness);
      ag.ledBar.setEnable(configuration.getLedBarMode() != LedBarModeOff);
    }
    ag.ledBar.show();
  }
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
  WiFi.begin("airgradient", "cleanair");
  oledDisplay.setText("Configure WiFi", "connect to", "\'airgradient\'");
  delay(2500);
  oledDisplay.setText("Rebooting...", "", "");
  delay(2500);
  oledDisplay.setText("", "", "");
  ESP.restart();
}

static void sendDataToAg() {
  /** Change oledDisplay and led state */
  if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
    stateMachine.displayHandle(AgStateMachineWiFiOkServerConnecting);
  }
  stateMachine.handleLeds(AgStateMachineWiFiOkServerConnecting);

  /** Task handle led connecting animation */
  // xTaskCreate(
  //     [](void *obj) {
  //       for (;;) {
  //         // ledSmHandler();
  //         stateMachine.handleLeds();
  //         if (stateMachine.getLedState() !=
  //             AgStateMachineWiFiOkServerConnecting) {
  //           break;
  //         }
  //         delay(LED_BAR_ANIMATION_PERIOD);
  //       }
  //       vTaskDelete(NULL);
  //     },
  //     "task_led", 2048, NULL, 5, NULL);

  delay(1500);
  if (apiClient.sendPing(wifiConnector.RSSI(), measurements.bootCount)) {
    if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
      stateMachine.displayHandle(AgStateMachineWiFiOkServerConnected);
    }
    stateMachine.handleLeds(AgStateMachineWiFiOkServerConnected);
  } else {
    if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
      stateMachine.displayHandle(AgStateMachineWiFiOkServerConnectFailed);
    }
    stateMachine.handleLeds(AgStateMachineWiFiOkServerConnectFailed);
  }
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
  stateMachine.handleLeds(AgStateMachineNormal);
}

void dispSensorNotFound(String ss) {
  ss = ss + " not found";
  oledDisplay.setText("Sensor init", "Error:", ss.c_str());
  delay(2000);
}

static void boardInit(void) {
  /** Display init */
  oledDisplay.begin();

  /** Show boot display */
  Serial.println("Firmware Version: " + ag.getVersion());

  oledDisplay.setText("AirGradient ONE",
                      "FW Version: ", ag.getVersion().c_str());
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

  ag.ledBar.begin();
  ag.button.begin();
  ag.watchdog.begin();

  /** Run LED test on start up if button pressed */
  if (ag.isOne()) {
    oledDisplay.setText("Press now for", "LED test", "");
  } else if (ag.getBoardType() == DIY_PRO_INDOOR_V4_2) {
    oledDisplay.setText("Press now for", "factory WiFi", "configure");
  }

  ledBarButtonTest = false;
  uint32_t stime = millis();
  while (true) {
    if (ag.button.getState() == ag.button.BUTTON_PRESSED) {
      if (ag.isOne()) {
        ledBarButtonTest = true;
        stateMachine.executeLedBarPowerUpTest();
        break;
      }
      if (ag.getBoardType() == DIY_PRO_INDOOR_V4_2) {
        wifiFactoryConfigure();
      }
    }
    delay(1);
    uint32_t ms = (uint32_t)(millis() - stime);
    if (ms >= 3000) {
      break;
    }
    delay(1);
  }

  /** Check for button to reset WiFi connecto to "airgraident" after test LED
   * bar */
  if (ledBarButtonTest && ag.isOne()) {
    if (ag.button.getState() == ag.button.BUTTON_PRESSED) {
      wifiFactoryConfigure();
    }
  }
  ledBarEnabledUpdate();

  /** Show message init sensor */
  oledDisplay.setText("Sensor", "initializing...", "");

  /** Init sensor SGP41 */
  if (sgp41Init() == false) {
    dispSensorNotFound("SGP41");
  }

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
  configuration.hasSensorPMS1 = false;
  configuration.hasSensorPMS2 = false;
  if (ag.pms5003.begin(&Serial) == false) {
    Serial.println("PMS sensor not found");
    configuration.hasSensorPMS = false;

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

  localServer.setFwMode(FW_MODE_I_8PSL);
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

  // String mqttUri = configuration.getMqttBrokerUri();
  // if (mqttClient.isCurrentUri(mqttUri) == false) {
  //   mqttClient.end();
  //   initMqtt();
  // }

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

  if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
    if (configuration.isLedBarBrightnessChanged()) {
      if (configuration.getLedBarBrightness() == 0) {
        ag.ledBar.setEnable(false);
      } else {
        if (configuration.getLedBarMode() != LedBarMode::LedBarModeOff) {
          ag.ledBar.setEnable(true);
        }
        ag.ledBar.setBrightness(configuration.getLedBarBrightness());
      }
      ag.ledBar.show();
    }

    // if (configuration.isLedBarModeChanged()) {
    //   if (configuration.getLedBarBrightness() == 0) {
    //     ag.ledBar.setEnable(false);
    //   } else {
    //     if (configuration.getLedBarMode() == LedBarMode::LedBarModeOff) {
    //       ag.ledBar.setEnable(false);
    //     } else {
    //       ag.ledBar.setEnable(true);
    //       ag.ledBar.setBrightness(configuration.getLedBarBrightness());
    //     }
    //   }
    //   ag.ledBar.show();
    // }

    if (configuration.isDisplayBrightnessChanged()) {
      oledDisplay.setBrightness(configuration.getDisplayBrightness());
    }

    // stateMachine.executeLedBarTest();
  }

  appDispHandler();
  appLedHandler();
}

static void appLedHandler(void) {
  AgStateMachineState state = AgStateMachineNormal;
  if (configuration.isOfflineMode() == false) {
    if (wifiConnector.isConnected() == false) {
      state = AgStateMachineWiFiLost;
    } else if (apiClient.isFetchConfigureFailed()) {
      state = AgStateMachineSensorConfigFailed;
    } else if (apiClient.isPostToServerFailed()) {
      state = AgStateMachineServerLost;
    }
  }

  stateMachine.handleLeds(state);
}

static void appDispHandler(void) {
  if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
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
}

static void oledDisplayLedBarSchedule(void) {
  if (ag.isOne() || (ag.getBoardType() == DIY_PRO_INDOOR_V4_2)) {
    if (factoryBtnPressTime == 0) {
      appDispHandler();
    }
  }
  appLedHandler();
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
    pmFailCount = 0;
  } else {
    pmFailCount++;
    Serial.printf("PMS read failed: %d\r\n", pmFailCount);
    if (pmFailCount >= 3) {
      measurements.pm01_1 = -1;
      measurements.pm25_1 = -1;
      measurements.pm10_1 = -1;
      measurements.pm03PCount_1 = -1;
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
  }
}
