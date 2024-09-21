/*
This is the combined firmware code for AirGradient ONE and AirGradient Open Air
open-source hardware Air Quality Monitor with ESP32-C3 Microcontroller.

It is an air quality monitor for PM2.5, CO2, TVOCs, NOx, Temperature and
Humidity with a small display, an RGB led bar and can send data over Wifi.

Open source air quality monitors and kits are available:
Indoor Monitor: https://www.airgradient.com/indoor/
Outdoor Monitor: https://www.airgradient.com/outdoor/

Build Instructions: AirGradient ONE:
https://www.airgradient.com/documentation/one-v9/ Build Instructions:
AirGradient Open Air:
https://www.airgradient.com/documentation/open-air-pst-kit-1-3/

Please make sure you have esp32 board manager installed. Tested with
version 2.0.11.

Important flashing settings:
- Set board to "ESP32C3 Dev Module"
- Enable "USB CDC On Boot"
- Flash frequency "80Mhz"
- Flash mode "QIO"
- Flash size "4MB"
- Partition scheme "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"
- JTAG adapter "Disabled"

Configuration parameters, e.g. Celsius / Fahrenheit or PM unit (US AQI vs ug/m3)
can be set through the AirGradient dashboard.

If you have any questions please visit our forum at
https://forum.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include <HardwareSerial.h>
#include "AirGradient.h"
#include "OtaHandler.h"
#include "AgApiClient.h"
#include "AgConfigure.h"
#include "AgSchedule.h"
#include "AgStateMachine.h"
#include "AgWiFiConnector.h"
#include "EEPROM.h"
#include "ESPmDNS.h"
#include "LocalServer.h"
#include "MqttClient.h"
#include "OpenMetrics.h"
#include "WebServer.h"
#include <WebServer.h>
#include <WiFi.h>

#define LED_BAR_ANIMATION_PERIOD 100         /** ms */
#define DISP_UPDATE_INTERVAL 2500            /** ms */
#define SERVER_CONFIG_SYNC_INTERVAL 60000    /** ms */
#define SERVER_SYNC_INTERVAL 60000           /** ms */
#define MQTT_SYNC_INTERVAL 60000             /** ms */
#define SENSOR_CO2_CALIB_COUNTDOWN_MAX 5     /** sec */
#define SENSOR_TVOC_UPDATE_INTERVAL 1000     /** ms */
#define SENSOR_CO2_UPDATE_INTERVAL 4000      /** ms */
#define SENSOR_PM_UPDATE_INTERVAL 2000       /** ms */
#define SENSOR_TEMP_HUM_UPDATE_INTERVAL 2000 /** ms */
#define DISPLAY_DELAY_SHOW_CONTENT_MS 2000   /** ms */
#define FIRMWARE_CHECK_FOR_UPDATE_MS (60*60*1000)   /** ms */

/** I2C define */
#define I2C_SDA_PIN 7
#define I2C_SCL_PIN 6
#define OLED_I2C_ADDR 0x3C

static MqttClient mqttClient(Serial);
static TaskHandle_t mqttTask = NULL;
static Configuration configuration(Serial);
static AgApiClient apiClient(Serial, configuration);
static Measurements measurements;
static AirGradient *ag;
static OledDisplay oledDisplay(configuration, measurements, Serial);
static StateMachine stateMachine(oledDisplay, Serial, measurements,
                                 configuration);
static WifiConnector wifiConnector(oledDisplay, Serial, stateMachine,
                                   configuration);
static OpenMetrics openMetrics(measurements, configuration, wifiConnector,
                               apiClient);
static OtaHandler otaHandler;
static LocalServer localServer(Serial, openMetrics, measurements, configuration,
                               wifiConnector);

static uint32_t factoryBtnPressTime = 0;
static int getCO2FailCount = 0;
static AgFirmwareMode fwMode = FW_MODE_I_9PSL;

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
static void firmwareCheckForUpdate(void);
static void otaHandlerCallback(OtaState state, String mesasge);
static void displayExecuteOta(OtaState state, String msg,
                              int processing);

AgSchedule dispLedSchedule(DISP_UPDATE_INTERVAL, oledDisplayLedBarSchedule);
AgSchedule configSchedule(SERVER_CONFIG_SYNC_INTERVAL,
                          configurationUpdateSchedule);
AgSchedule agApiPostSchedule(SERVER_SYNC_INTERVAL, sendDataToServer);
AgSchedule co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Update);
AgSchedule pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, updatePm);
AgSchedule tempHumSchedule(SENSOR_TEMP_HUM_UPDATE_INTERVAL, tempHumUpdate);
AgSchedule tvocSchedule(SENSOR_TVOC_UPDATE_INTERVAL, updateTvoc);
AgSchedule watchdogFeedSchedule(60000, wdgFeedUpdate);
AgSchedule checkForUpdateSchedule(FIRMWARE_CHECK_FOR_UPDATE_MS, firmwareCheckForUpdate);

void setup() {
  /** Serial for print debug message */
  Serial.begin(115200);
  delay(100); /** For bester show log */

  /** Print device ID into log */
  Serial.println("Serial nr: " + ag->deviceId());

  /** Initialize local configure */
  configuration.begin();

  /** Init I2C */
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  delay(1000);

  /** Detect board type: ONE_INDOOR has OLED display, Scan the I2C address to
   * identify board type */
  Wire.beginTransmission(OLED_I2C_ADDR);
  if (Wire.endTransmission() == 0x00) {
    ag = new AirGradient(BoardType::ONE_INDOOR);
  } else {
    ag = new AirGradient(BoardType::OPEN_AIR_OUTDOOR);
  }
  Serial.println("Detected " + ag->getBoardName());

  configuration.setAirGradient(ag);
  oledDisplay.setAirGradient(ag);
  stateMachine.setAirGradient(ag);
  wifiConnector.setAirGradient(ag);
  apiClient.setAirGradient(ag);
  openMetrics.setAirGradient(ag);
  localServer.setAirGraident(ag);

  /** Example set custom API root URL */
  // apiClient.setApiRoot("https://example.custom.api");

  /** Init sensor */
  boardInit();

  /** Connecting wifi */
  bool connectToWifi = false;
  if (ag->isOne()) {
    /** Show message confirm offline mode, should me perform if LED bar button
     * test pressed */
    if (ledBarButtonTest == false) {
      oledDisplay.setText(
          "Press now for",
          configuration.isOfflineMode() ? "online mode" : "offline mode", "");
      uint32_t startTime = millis();
      while (true) {
        if (ag->button.getState() == ag->button.BUTTON_PRESSED) {
          configuration.setOfflineMode(!configuration.isOfflineMode());

          oledDisplay.setText(
              "Offline Mode",
              configuration.isOfflineMode() ? " = True" : "  = False", "");
          delay(1000);
          break;
        }
        uint32_t periodMs = (uint32_t)(millis() - startTime);
        if (periodMs >= 3000) {
          break;
        }
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

        #ifdef ESP8266
          // ota not supported
        #else
          firmwareCheckForUpdate();
          checkForUpdateSchedule.update();
        #endif

        apiClient.fetchServerConfiguration();
        configSchedule.update();
        if (apiClient.isFetchConfigureFailed()) {
          if (ag->isOne()) {
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
  if (ag->isOne()) {
    oledDisplay.setText("Warming Up", "Serial Number:", ag->deviceId().c_str());
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

    Serial.println("Display brightness: " + String(configuration.getDisplayBrightness()));
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
  if (configuration.hasSensorPMS1 || configuration.hasSensorPMS2) {
    pmsSchedule.run();
  }
  if (ag->isOne()) {
    if (configuration.hasSensorSHT) {
      tempHumSchedule.run();
    }
  }
  if (configuration.hasSensorSGP) {
    tvocSchedule.run();
  }
  if (ag->isOne()) {
    if (configuration.hasSensorPMS1) {
      ag->pms5003.handle();
    }
  } else {
    if (configuration.hasSensorPMS1) {
      ag->pms5003t_1.handle();
    }
    if (configuration.hasSensorPMS2) {
      ag->pms5003t_2.handle();
    }
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

  /** Firmware check for update handle */
  checkForUpdateSchedule.run();
}

static void co2Update(void) {
  int value = ag->s8.getCo2();
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
  if (!MDNS.begin(localServer.getHostname().c_str())) {
    Serial.println("Init mDNS failed");
    return;
  }

  MDNS.addService("_airgradient", "_tcp", 80);
  MDNS.addServiceTxt("_airgradient", "_tcp", "model",
                     AgFirmwareModeName(fwMode));
  MDNS.addServiceTxt("_airgradient", "_tcp", "serialno", ag->deviceId());
  MDNS.addServiceTxt("_airgradient", "_tcp", "fw_ver", ag->getVersion());
  MDNS.addServiceTxt("_airgradient", "_tcp", "vendor", "AirGradient");
}

static void createMqttTask(void) {
  if (mqttTask) {
    vTaskDelete(mqttTask);
    mqttTask = NULL;
    Serial.println("Delete old MQTT task");
  }

  Serial.println("Create new MQTT task");
  xTaskCreate(
      [](void *param) {
        for (;;) {
          delay(MQTT_SYNC_INTERVAL);

          /** Send data */
          if (mqttClient.isConnected()) {
            String payload = measurements.toString(
                true, fwMode, wifiConnector.RSSI(), ag, &configuration);
            String topic = "airgradient/readings/" + ag->deviceId();

            if (mqttClient.publish(topic.c_str(), payload.c_str(),
                                   payload.length())) {
              Serial.println("MQTT sync success");
            } else {
              Serial.println("MQTT sync failure");
            }
          }
        }
      },
      "mqtt-task", 1024 * 4, NULL, 6, &mqttTask);

  if (mqttTask == NULL) {
    Serial.println("Creat mqttTask failed");
  }
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
    createMqttTask();
  } else {
    Serial.println("Connection to MQTT broker failed");
  }
}

static void factoryConfigReset(void) {
  if (ag->button.getState() == ag->button.BUTTON_PRESSED) {
    if (factoryBtnPressTime == 0) {
      factoryBtnPressTime = millis();
    } else {
      uint32_t ms = (uint32_t)(millis() - factoryBtnPressTime);
      if (ms >= 2000) {
        // Show display message: For factory keep for x seconds
        if (ag->isOne()) {
          oledDisplay.setText("Factory reset", "keep pressed", "for 8 sec");
        } else {
          Serial.println("Factory reset, keep pressed for 8 sec");
        }

        int count = 7;
        while (ag->button.getState() == ag->button.BUTTON_PRESSED) {
          delay(1000);
          if (ag->isOne()) {

            String str = "for " + String(count) + " sec";
            oledDisplay.setText("Factory reset", "keep pressed", str.c_str());
          } else {
            Serial.printf("Factory reset, keep pressed for %d sec\r\n", count);
          }
          count--;
          if (count == 0) {
            /** Stop MQTT task first */
            if (mqttTask) {
              vTaskDelete(mqttTask);
              mqttTask = NULL;
            }

            /** Reset WIFI */
            WiFi.disconnect(true, true);

            /** Reset local config */
            configuration.reset();

            if (ag->isOne()) {
              oledDisplay.setText("Factory reset", "successful", "");
            } else {
              Serial.println("Factory reset successful");
            }
            delay(3000);
            oledDisplay.setText("","","");
            ESP.restart();
          }
        }

        /** Show current content cause reset ignore */
        factoryBtnPressTime = 0;
        if (ag->isOne()) {
          appDispHandler();
        }
      }
    }
  } else {
    if (factoryBtnPressTime != 0) {
      if (ag->isOne()) {
        /** Restore last display content */
        appDispHandler();
      }
    }
    factoryBtnPressTime = 0;
  }
}

static void wdgFeedUpdate(void) {
  ag->watchdog.reset();
  Serial.println();
  Serial.println("Offline mode or isPostToAirGradient = false: watchdog reset");
  Serial.println();
}

static void ledBarEnabledUpdate(void) {
  if (ag->isOne()) {
    int brightness = configuration.getLedBarBrightness();
    Serial.println("LED bar brightness: " + String(brightness));
    if ((brightness == 0) || (configuration.getLedBarMode() == LedBarModeOff)) {
      ag->ledBar.setEnable(false);
    } else {
      ag->ledBar.setBrightness(brightness);
      ag->ledBar.setEnable(configuration.getLedBarMode() != LedBarModeOff);
    }
     ag->ledBar.show();
  }
}

static bool sgp41Init(void) {
  ag->sgp41.setNoxLearningOffset(configuration.getNoxLearningOffset());
  ag->sgp41.setTvocLearningOffset(configuration.getTvocLearningOffset());
  if (ag->sgp41.begin(Wire)) {
    Serial.println("Init SGP41 success");
    configuration.hasSensorSGP = true;
    return true;
  } else {
    Serial.println("Init SGP41 failuire");
    configuration.hasSensorSGP = false;
  }
  return false;
}

static void firmwareCheckForUpdate(void) {
  Serial.println();
  Serial.println("firmwareCheckForUpdate:");

  if (wifiConnector.isConnected()) {
    Serial.println("firmwareCheckForUpdate: Perform");
    otaHandler.setHandlerCallback(otaHandlerCallback);
    otaHandler.updateFirmwareIfOutdated(ag->deviceId());
  } else {
    Serial.println("firmwareCheckForUpdate: Ignored");
  }
  Serial.println();
}

static void otaHandlerCallback(OtaState state, String mesasge) {
  Serial.println("OTA message: " + mesasge);
  switch (state) {
  case OtaState::OTA_STATE_BEGIN:
    displayExecuteOta(state, fwNewVersion, 0);
    break;
  case OtaState::OTA_STATE_FAIL:
    displayExecuteOta(state, "", 0);
    break;
  case OtaState::OTA_STATE_PROCESSING:
    displayExecuteOta(state, "", mesasge.toInt());
    break;
  case OtaState::OTA_STATE_SUCCESS:
    displayExecuteOta(state, "", mesasge.toInt());
    break;
  default:
    break;
  }
}

static void displayExecuteOta(OtaState state, String msg, int processing) {
  switch (state) {
  case OtaState::OTA_STATE_BEGIN: {
    if (ag->isOne()) {
      oledDisplay.showFirmwareUpdateVersion(msg);
    } else {
      Serial.println("New firmware: " + msg);
    }
    delay(2500);
    break;
  }
  case OtaState::OTA_STATE_FAIL: {
    if (ag->isOne()) {
      oledDisplay.showFirmwareUpdateFailed();
    } else {
      Serial.println("Error: Firmware update: failed");
    }

    delay(2500);
    break;
  }
  case OtaState::OTA_STATE_SKIP: {
    if (ag->isOne()) {
      oledDisplay.showFirmwareUpdateSkipped();
    } else {
      Serial.println("Firmware update: Skipped");
    }

    delay(2500);
    break;
  }
  case OtaState::OTA_STATE_UP_TO_DATE: {
    if (ag->isOne()) {
      oledDisplay.showFirmwareUpdateUpToDate();
    } else {
      Serial.println("Firmware update: up to date");
    }

    delay(2500);
    break;
  }
  case OtaState::OTA_STATE_PROCESSING: {
    if (ag->isOne()) {
      oledDisplay.showFirmwareUpdateProgress(processing);
    } else {
      Serial.println("Firmware update: " + String(processing) + String("%"));
    }

    break;
  }
  case OtaState::OTA_STATE_SUCCESS: {
    int i = 6;
    while(i != 0) {
      i = i - 1;
      Serial.println("OTA update performed, restarting ...");
      int i = 6;
      while (i != 0) {
        i = i - 1;
        if (ag->isOne()) {
          oledDisplay.showFirmwareUpdateSuccess(i);
        } else {
          Serial.println("Rebooting... " + String(i));
        }
        
        delay(1000);
      }
      oledDisplay.setBrightness(0);
      esp_restart();
    }
    break;
  }
  default:
    break;
  }
}

static void sendDataToAg() {
  /** Change oledDisplay and led state */
  if (ag->isOne()) {
    stateMachine.displayHandle(AgStateMachineWiFiOkServerConnecting);
  }
  stateMachine.handleLeds(AgStateMachineWiFiOkServerConnecting);

  /** Task handle led connecting animation */
  xTaskCreate(
      [](void *obj) {
        for (;;) {
          // ledSmHandler();
          stateMachine.handleLeds();
          if (stateMachine.getLedState() !=
              AgStateMachineWiFiOkServerConnecting) {
            break;
          }
          delay(LED_BAR_ANIMATION_PERIOD);
        }
        vTaskDelete(NULL);
      },
      "task_led", 2048, NULL, 5, NULL);

  delay(1500);
  if (apiClient.sendPing(wifiConnector.RSSI(), measurements.bootCount)) {
    if (ag->isOne()) {
      stateMachine.displayHandle(AgStateMachineWiFiOkServerConnected);
    }
    stateMachine.handleLeds(AgStateMachineWiFiOkServerConnected);
  } else {
    if (ag->isOne()) {
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

static void oneIndoorInit(void) {
  configuration.hasSensorPMS2 = false;

  /** Display init */
  oledDisplay.begin();

  /** Show boot display */
  Serial.println("Firmware Version: " + ag->getVersion());

  oledDisplay.setText("AirGradient ONE",
                      "FW Version: ", ag->getVersion().c_str());
  delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

  ag->ledBar.begin();
  ag->button.begin();
  ag->watchdog.begin();

  /** Run LED test on start up if button pressed */
  oledDisplay.setText("Press now for", "LED test", "");
  ledBarButtonTest = false;
  uint32_t stime = millis();
  while (true) {
    if (ag->button.getState() == ag->button.BUTTON_PRESSED) {
      ledBarButtonTest = true;
      stateMachine.executeLedBarPowerUpTest();
      break;
    }
    delay(1);
    uint32_t ms = (uint32_t)(millis() - stime);
    if (ms >= 3000) {
      break;
    }
  }

  /** Check for button to reset WiFi connecto to "airgraident" after test LED
   * bar */
  if (ledBarButtonTest) {
    if (ag->button.getState() == ag->button.BUTTON_PRESSED) {
      WiFi.begin("airgradient", "cleanair");
      oledDisplay.setText("Configure WiFi", "connect to", "\'airgradient\'");
      delay(2500);
      oledDisplay.setText("Rebooting...", "","");
      delay(2500);
      oledDisplay.setText("","","");
      ESP.restart();
    }
  }
  ledBarEnabledUpdate();

  /** Show message init sensor */
  oledDisplay.setText("Monitor", "initializing...", "");

  /** Init sensor SGP41 */
  if (sgp41Init() == false) {
    dispSensorNotFound("SGP41");
  }

  /** INit SHT */
  if (ag->sht.begin(Wire) == false) {
    Serial.println("SHTx sensor not found");
    configuration.hasSensorSHT = false;
    dispSensorNotFound("SHT");
  }

  /** Init S8 CO2 sensor */
  if (ag->s8.begin(Serial1) == false) {
    Serial.println("CO2 S8 sensor not found");
    configuration.hasSensorS8 = false;
    dispSensorNotFound("S8");
  }

  /** Init PMS5003 */
  if (ag->pms5003.begin(Serial0) == false) {
    Serial.println("PMS sensor not found");
    configuration.hasSensorPMS1 = false;

    dispSensorNotFound("PMS");
  }
}
static void openAirInit(void) {
  configuration.hasSensorSHT = false;

  fwMode = FW_MODE_O_1PST;
  Serial.println("Firmware Version: " + ag->getVersion());

  ag->watchdog.begin();
  ag->button.begin();
  ag->statusLed.begin();

  /** detect sensor: PMS5003, PMS5003T, SGP41 and S8 */
  /**
   * Serial1 and Serial0 is use for connect S8 and PM sensor or both PM
   */
  bool serial1Available = true;
  bool serial0Available = true;

  if (ag->s8.begin(Serial1) == false) {
    Serial1.end();
    delay(200);
    Serial.println("Can not detect S8 on Serial1, try on Serial0");
    /** Check on other port */
    if (ag->s8.begin(Serial0) == false) {
      configuration.hasSensorS8 = false;

      Serial.println("CO2 S8 sensor not found");
      Serial.println("Can not detect S8 run mode 'PPT'");
      fwMode = FW_MODE_O_1PPT;

      Serial0.end();
      delay(200);
    } else {
      Serial.println("Found S8 on Serial0");
      serial0Available = false;
    }
  } else {
    Serial.println("Found S8 on Serial1");
    serial1Available = false;
  }

  if (sgp41Init() == false) {
    Serial.println("SGP sensor not found");

    if (configuration.hasSensorS8 == false) {
      Serial.println("Can not detect SGP run mode 'O-1PP'");
      fwMode = FW_MODE_O_1PP;
    } else {
      Serial.println("Can not detect SGP run mode 'O-1PS'");
      fwMode = FW_MODE_O_1PS;
    }
  }

  /** Attempt to detect PM sensors */
  if (fwMode == FW_MODE_O_1PST) {
    bool pmInitSuccess = false;
    if (serial0Available) {
      if (ag->pms5003t_1.begin(Serial0) == false) {
        configuration.hasSensorPMS1 = false;
        Serial.println("No PM sensor detected on Serial0");
      } else {
        serial0Available = false;
        pmInitSuccess = true;
        Serial.println("Detected PM 1 on Serial0");
      }
    }
    if (pmInitSuccess == false) {
      if (serial1Available) {
        if (ag->pms5003t_1.begin(Serial1) == false) {
          configuration.hasSensorPMS1 = false;
          Serial.println("No PM sensor detected on Serial1");
        } else {
          serial1Available = false;
          Serial.println("Detected PM 1 on Serial1");
        }
      }
    }
    configuration.hasSensorPMS2 = false; // Disable PM2
  } else {
    if (ag->pms5003t_1.begin(Serial0) == false) {
      configuration.hasSensorPMS1 = false;
      Serial.println("No PM sensor detected on Serial0");
    } else {
      Serial.println("Detected PM 1 on Serial0");
    }
    if (ag->pms5003t_2.begin(Serial1) == false) {
      configuration.hasSensorPMS2 = false;
      Serial.println("No PM sensor detected on Serial1");
    } else {
      Serial.println("Detected PM 2 on Serial1");
    }

    if (fwMode == FW_MODE_O_1PP) {
      int count = (configuration.hasSensorPMS1 ? 1 : 0) +
                  (configuration.hasSensorPMS2 ? 1 : 0);
      if (count == 1) {
        fwMode = FW_MODE_O_1P;
      }
    }
  }

  /** update the PMS poll period base on fw mode and sensor available */
  if (fwMode != FW_MODE_O_1PST) {
    if (configuration.hasSensorPMS1 && configuration.hasSensorPMS2) {
      pmsSchedule.setPeriod(2000);
    }
  }
  Serial.printf("Firmware Mode: %s\r\n", AgFirmwareModeName(fwMode));
}

static void boardInit(void) {
  if (ag->isOne()) {
    oneIndoorInit();
  } else {
    openAirInit();
  }

  /** Set S8 CO2 abc days period */
  if (configuration.hasSensorS8) {
    if (ag->s8.setAbcPeriod(configuration.getCO2CalibrationAbcDays() * 24)) {
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
    vTaskDelay(1000);
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
      ag->sgp41.end();

      int oldTvocOffset = ag->sgp41.getTvocLearningOffset();
      int oldNoxOffset = ag->sgp41.getNoxLearningOffset();
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

  if (ag->isOne()) {
    if (configuration.isLedBarBrightnessChanged()) {
      if (configuration.getLedBarBrightness() == 0) {
        ag->ledBar.setEnable(false);
      } else {
        if (configuration.getLedBarMode() != LedBarMode::LedBarModeOff) {
          ag->ledBar.setEnable(true);
        }
        ag->ledBar.setBrightness(configuration.getLedBarBrightness());
      }
      ag->ledBar.show();
    }

    if (configuration.isLedBarModeChanged()) {
      if (configuration.getLedBarBrightness() == 0) {
        ag->ledBar.setEnable(false);
      } else {
        if(configuration.getLedBarMode() == LedBarMode::LedBarModeOff) {
          ag->ledBar.setEnable(false);
        } else {
          ag->ledBar.setEnable(true);
          ag->ledBar.setBrightness(configuration.getLedBarBrightness());
        }
      }
      ag->ledBar.show();
    }

    if (configuration.isDisplayBrightnessChanged()) {
      oledDisplay.setBrightness(configuration.getDisplayBrightness());
    }

    stateMachine.executeLedBarTest();
  }
  else if(ag->isOpenAir()) {
    stateMachine.executeLedBarTest();
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
  if (ag->isOne()) {
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
  if (ag->isOne()) {
    if (factoryBtnPressTime == 0) {
      appDispHandler();
    }
  }
  appLedHandler();
}

static void updateTvoc(void) {
  measurements.TVOC = ag->sgp41.getTvocIndex();
  measurements.TVOCRaw = ag->sgp41.getTvocRaw();
  measurements.NOx = ag->sgp41.getNoxIndex();
  measurements.NOxRaw = ag->sgp41.getNoxRaw();

  Serial.println();
  Serial.printf("TVOC index: %d\r\n", measurements.TVOC);
  Serial.printf("TVOC raw: %d\r\n", measurements.TVOCRaw);
  Serial.printf("NOx index: %d\r\n", measurements.NOx);
  Serial.printf("NOx raw: %d\r\n", measurements.NOxRaw);
}

static void updatePm(void) {
  bool restart = false;
  if (ag->isOne()) {
    if (ag->pms5003.isFailed() == false) {
      measurements.pm01_1 = ag->pms5003.getPm01Ae();
      measurements.pm25_1 = ag->pms5003.getPm25Ae();
      measurements.pm10_1 = ag->pms5003.getPm10Ae();
      measurements.pm03PCount_1 = ag->pms5003.getPm03ParticleCount();

      Serial.println();
      Serial.printf("PM1 ug/m3: %d\r\n", measurements.pm01_1);
      Serial.printf("PM2.5 ug/m3: %d\r\n", measurements.pm25_1);
      Serial.printf("PM10 ug/m3: %d\r\n", measurements.pm10_1);
      Serial.printf("PM0.3 Count: %d\r\n", measurements.pm03PCount_1);
      Serial.printf("PM firmware version: %d\r\n", ag->pms5003.getFirmwareVersion());
      ag->pms5003.resetFailCount();
    } else {
      ag->pms5003.updateFailCount();
      Serial.printf("PMS read failed %d times\r\n", ag->pms5003.getFailCount());
      if (ag->pms5003.getFailCount() >= PMS_FAIL_COUNT_SET_INVALID) {
        measurements.pm01_1 = utils::getInvalidPmValue();
        measurements.pm25_1 = utils::getInvalidPmValue();
        measurements.pm10_1 = utils::getInvalidPmValue();
        measurements.pm03PCount_1 = utils::getInvalidPmValue();
      }

      if (ag->pms5003.getFailCount() >= ag->pms5003.getFailCountMax()) {
        restart = true;
      }
    }
  } else {
    bool pmsResult_1 = false;
    bool pmsResult_2 = false;
    if (configuration.hasSensorPMS1 && (ag->pms5003t_1.isFailed() == false)) {
      measurements.pm01_1 = ag->pms5003t_1.getPm01Ae();
      measurements.pm25_1 = ag->pms5003t_1.getPm25Ae();
      measurements.pm10_1 = ag->pms5003t_1.getPm10Ae();
      measurements.pm03PCount_1 = ag->pms5003t_1.getPm03ParticleCount();
      measurements.temp_1 = ag->pms5003t_1.getTemperature();
      measurements.hum_1 = ag->pms5003t_1.getRelativeHumidity();

      pmsResult_1 = true;

      Serial.println();
      Serial.printf("[1] PM1 ug/m3: %d\r\n", measurements.pm01_1);
      Serial.printf("[1] PM2.5 ug/m3: %d\r\n", measurements.pm25_1);
      Serial.printf("[1] PM10 ug/m3: %d\r\n", measurements.pm10_1);
      Serial.printf("[1] PM3.0 Count: %d\r\n", measurements.pm03PCount_1);
      Serial.printf("[1] Temperature in C: %0.2f\r\n", measurements.temp_1);
      Serial.printf("[1] Relative Humidity: %d\r\n", measurements.hum_1);
      Serial.printf("[1] Temperature compensated in C: %0.2f\r\n",
                    ag->pms5003t_1.compensateTemp(measurements.temp_1));
      Serial.printf("[1] Relative Humidity compensated: %0.2f\r\n",
                    ag->pms5003t_1.compensateHum(measurements.hum_1));
      Serial.printf("[1] PM firmware version: %d\r\n", ag->pms5003t_1.getFirmwareVersion());

      ag->pms5003t_1.resetFailCount();
    } else {
      if (configuration.hasSensorPMS1) {
        ag->pms5003t_1.updateFailCount();
        Serial.printf("[1] PMS read failed %d times\r\n", ag->pms5003t_1.getFailCount());

        if (ag->pms5003t_1.getFailCount() >= PMS_FAIL_COUNT_SET_INVALID) {
          measurements.pm01_1 = utils::getInvalidPmValue();
          measurements.pm25_1 = utils::getInvalidPmValue();
          measurements.pm10_1 = utils::getInvalidPmValue();
          measurements.pm03PCount_1 = utils::getInvalidPmValue();
          measurements.temp_1 = utils::getInvalidTemperature();
          measurements.hum_1 = utils::getInvalidHumidity();
        }

        if (ag->pms5003t_1.getFailCount() >= ag->pms5003t_1.getFailCountMax()) {
          restart = true;
        }
      }
    }

    if (configuration.hasSensorPMS2 && (ag->pms5003t_2.isFailed() == false)) {
      measurements.pm01_2 = ag->pms5003t_2.getPm01Ae();
      measurements.pm25_2 = ag->pms5003t_2.getPm25Ae();
      measurements.pm10_2 = ag->pms5003t_2.getPm10Ae();
      measurements.pm03PCount_2 = ag->pms5003t_2.getPm03ParticleCount();
      measurements.temp_2 = ag->pms5003t_2.getTemperature();
      measurements.hum_2 = ag->pms5003t_2.getRelativeHumidity();

      pmsResult_2 = true;

      Serial.println();
      Serial.printf("[2] PM1 ug/m3: %d\r\n", measurements.pm01_2);
      Serial.printf("[2] PM2.5 ug/m3: %d\r\n", measurements.pm25_2);
      Serial.printf("[2] PM10 ug/m3: %d\r\n", measurements.pm10_2);
      Serial.printf("[2] PM3.0 Count: %d\r\n", measurements.pm03PCount_2);
      Serial.printf("[2] Temperature in C: %0.2f\r\n", measurements.temp_2);
      Serial.printf("[2] Relative Humidity: %d\r\n", measurements.hum_2);
      Serial.printf("[2] Temperature compensated in C: %0.2f\r\n",
                    ag->pms5003t_1.compensateTemp(measurements.temp_2));
      Serial.printf("[2] Relative Humidity compensated: %0.2f\r\n",
                    ag->pms5003t_1.compensateHum(measurements.hum_2));
      Serial.printf("[2] PM firmware version: %d\r\n", ag->pms5003t_2.getFirmwareVersion());

      ag->pms5003t_2.resetFailCount();
    } else {
      if (configuration.hasSensorPMS2) {
        ag->pms5003t_2.updateFailCount();
        Serial.printf("[2] PMS read failed %d times\r\n", ag->pms5003t_2.getFailCount());

        if (ag->pms5003t_2.getFailCount() >= PMS_FAIL_COUNT_SET_INVALID) {
          measurements.pm01_2 = utils::getInvalidPmValue();
          measurements.pm25_2 = utils::getInvalidPmValue();
          measurements.pm10_2 = utils::getInvalidPmValue();
          measurements.pm03PCount_2 = utils::getInvalidPmValue();
          measurements.temp_2 = utils::getInvalidTemperature();
          measurements.hum_2 = utils::getInvalidHumidity();
        }

        if (ag->pms5003t_2.getFailCount() >= ag->pms5003t_2.getFailCountMax()) {
          restart = true;
        }
      }
    }

    if (configuration.hasSensorPMS1 && configuration.hasSensorPMS2 &&
        pmsResult_1 && pmsResult_2) {
      /** Get total of PMS1*/
      measurements.pm1Value01 = measurements.pm1Value01 + measurements.pm01_1;
      measurements.pm1Value25 = measurements.pm1Value25 + measurements.pm25_1;
      measurements.pm1Value10 = measurements.pm1Value10 + measurements.pm10_1;
      measurements.pm1PCount =
          measurements.pm1PCount + measurements.pm03PCount_1;
      measurements.pm1temp = measurements.pm1temp + measurements.temp_1;
      measurements.pm1hum = measurements.pm1hum + measurements.hum_1;

      /** Get total of PMS2 */
      measurements.pm2Value01 = measurements.pm2Value01 + measurements.pm01_2;
      measurements.pm2Value25 = measurements.pm2Value25 + measurements.pm25_2;
      measurements.pm2Value10 = measurements.pm2Value10 + measurements.pm10_2;
      measurements.pm2PCount =
          measurements.pm2PCount + measurements.pm03PCount_2;
      measurements.pm2temp = measurements.pm2temp + measurements.temp_2;
      measurements.pm2hum = measurements.pm2hum + measurements.hum_2;

      measurements.countPosition++;

      /** Get average */
      if (measurements.countPosition == measurements.targetCount) {
        measurements.pm01_1 =
            measurements.pm1Value01 / measurements.targetCount;
        measurements.pm25_1 =
            measurements.pm1Value25 / measurements.targetCount;
        measurements.pm10_1 =
            measurements.pm1Value10 / measurements.targetCount;
        measurements.pm03PCount_1 =
            measurements.pm1PCount / measurements.targetCount;
        measurements.temp_1 = measurements.pm1temp / measurements.targetCount;
        measurements.hum_1 = measurements.pm1hum / measurements.targetCount;

        measurements.pm01_2 =
            measurements.pm2Value01 / measurements.targetCount;
        measurements.pm25_2 =
            measurements.pm2Value25 / measurements.targetCount;
        measurements.pm10_2 =
            measurements.pm2Value10 / measurements.targetCount;
        measurements.pm03PCount_2 =
            measurements.pm2PCount / measurements.targetCount;
        measurements.temp_2 = measurements.pm2temp / measurements.targetCount;
        measurements.hum_2 = measurements.pm2hum / measurements.targetCount;

        measurements.countPosition = 0;

        measurements.pm1Value01 = 0;
        measurements.pm1Value25 = 0;
        measurements.pm1Value10 = 0;
        measurements.pm1PCount = 0;
        measurements.pm1temp = 0;
        measurements.pm1hum = 0;
        measurements.pm2Value01 = 0;
        measurements.pm2Value25 = 0;
        measurements.pm2Value10 = 0;
        measurements.pm2PCount = 0;
        measurements.pm2temp = 0;
        measurements.pm2hum = 0;
      }
    }

    if (pmsResult_1 && pmsResult_2) {
      measurements.Temperature =
          (measurements.temp_1 + measurements.temp_2) / 2;
      measurements.Humidity = (measurements.hum_1 + measurements.hum_2) / 2;
    } else {
      if (pmsResult_1) {
        measurements.Temperature = measurements.temp_1;
        measurements.Humidity = measurements.hum_1;
      }
      if (pmsResult_2) {
        measurements.Temperature = measurements.temp_2;
        measurements.Humidity = measurements.hum_2;
      }
    }

    if (configuration.hasSensorSGP) {
      float temp;
      float hum;
      if (pmsResult_1 && pmsResult_2) {
        temp = (measurements.temp_1 + measurements.temp_2) / 2.0f;
        hum = (measurements.hum_1 + measurements.hum_2) / 2.0f;
      } else {
        if (pmsResult_1) {
          temp = measurements.temp_1;
          hum = measurements.hum_1;
        }
        if (pmsResult_2) {
          temp = measurements.temp_2;
          hum = measurements.hum_2;
        }
      }
      ag->sgp41.setCompensationTemperatureHumidity(temp, hum);
    }
  }

  if (restart) {
    Serial.printf("PMS failure count reach to max set %d, restarting...", ag->pms5003.getFailCountMax());
    ESP.restart();
  }
}

static void sendDataToServer(void) {
  /** Ignore send data to server if postToAirGradient disabled */
  if (configuration.isPostDataToAirGradient() == false || configuration.isOfflineMode()) {
    return;
  }

  String syncData = measurements.toString(false, fwMode, wifiConnector.RSSI(),
                                          ag, &configuration);
  if (apiClient.postToServer(syncData)) {
    ag->watchdog.reset();
    Serial.println();
    Serial.println(
        "Online mode and isPostToAirGradient = true: watchdog reset");
    Serial.println();
  }

  measurements.bootCount++;
}

static void tempHumUpdate(void) {
  delay(100);
  if (ag->sht.measure()) {
    measurements.Temperature = ag->sht.getTemperature();
    measurements.Humidity = ag->sht.getRelativeHumidity();

    Serial.printf("Temperature in C: %0.2f\r\n", measurements.Temperature);
    Serial.printf("Relative Humidity: %d\r\n", measurements.Humidity);
    Serial.printf("Temperature compensated in C: %0.2f\r\n",
                  measurements.Temperature);
    Serial.printf("Relative Humidity compensated: %d\r\n",
                  measurements.Humidity);

    // Update compensation temperature and humidity for SGP41
    if (configuration.hasSensorSGP) {
      ag->sgp41.setCompensationTemperatureHumidity(measurements.Temperature,
                                                   measurements.Humidity);
    }
  } else {
    measurements.Temperature = utils::getInvalidTemperature();
    measurements.Humidity = utils::getInvalidHumidity();
    Serial.println("SHT read failed");
  }
}
