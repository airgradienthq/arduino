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

Compile Instructions:
https://github.com/airgradienthq/arduino/blob/master/docs/howto-compile.md

Configuration parameters, e.g. Celsius / Fahrenheit or PM unit (US AQI vs ug/m3)
can be set through the AirGradient dashboard.

If you have any questions please visit our forum at
https://forum.airgradient.com/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/
#ifndef ARDUINOJSON_ENABLE_PROGMEM
#define ARDUINOJSON_ENABLE_PROGMEM 0
#endif

#include "AgConfigure.h"
#include "AgHardwareIdentity.h"
#include "AgSatellites.h"
#include "AgSchedule.h"
#include "AgStateMachine.h"
#include "AgValue.h"
#include "AgWiFiConnector.h"
#include "AirGradient.h"
#include "App/AppDef.h"
#include "Arduino.h"
#include "EEPROM.h"
#include "ESPmDNS.h"
#include "Libraries/airgradient-client/src/common.h"
#include "LocalServer.h"
#include "MqttClient.h"
#include "OpenMetrics.h"
#include "WebServer.h"
#include "esp32c3/rom/rtc.h"
#include <HardwareSerial.h>
#include <WebServer.h>
#include <WiFi.h>
#include <cstdint>
#include <string>

#include "Libraries/airgradient-client/src/agSerial.h"
#include "Libraries/airgradient-client/src/cellularModule.h"
#include "Libraries/airgradient-client/src/cellularModuleA7672xx.h"
#include "Libraries/airgradient-client/src/airgradientCellularClient.h"
#include "Libraries/airgradient-client/src/airgradientWifiClient.h"
#include "Libraries/airgradient-ota/src/airgradientOta.h"
#include "Libraries/airgradient-ota/src/airgradientOtaWifi.h"
#include "Libraries/airgradient-ota/src/airgradientOtaCellular.h"
#include "esp_system.h"
#include "freertos/projdefs.h"

#define LED_BAR_ANIMATION_PERIOD 100                       /** ms */
#define DISP_UPDATE_INTERVAL 2500                          /** ms */
#define WIFI_SERVER_CONFIG_SYNC_INTERVAL 1 * 60000         /** ms */
#define WIFI_MEASUREMENT_INTERVAL 1 * 60000                /** ms */
#define WIFI_TRANSMISSION_INTERVAL 1 * 60000               /** ms */
#define CELLULAR_SERVER_CONFIG_SYNC_INTERVAL 30 * 60000    /** ms */
#define CELLULAR_MEASUREMENT_INTERVAL 3 * 60000            /** ms */
#define CELLULAR_TRANSMISSION_INTERVAL 3 * 60000           /** ms */
#define MQTT_SYNC_INTERVAL 60000                           /** ms */
#define SENSOR_CO2_CALIB_COUNTDOWN_MAX 5                   /** sec */
#define SENSOR_TVOC_UPDATE_INTERVAL 1000                   /** ms */
#define SENSOR_CO2_UPDATE_INTERVAL 4000                    /** ms */
#define SENSOR_PM_UPDATE_INTERVAL 2000                     /** ms */
#define SENSOR_TEMP_HUM_UPDATE_INTERVAL 6000               /** ms */
#define DISPLAY_DELAY_SHOW_CONTENT_MS 2000                 /** ms */
#define BOARD_SELECTION_REBOOT_DELAY_MS 3000               /** ms */
#define FIRMWARE_CHECK_FOR_UPDATE_MS (60 * 60 * 1000)      /** ms */
#define TIME_TO_START_POWER_CYCLE_CELLULAR_MODULE (1 * 60) /** minutes */
#define TIMEOUT_WAIT_FOR_CELLULAR_MODULE_READY (2 * 60)    /** minutes */

#define MEASUREMENT_TRANSMIT_CYCLE 3
#define MAXIMUM_MEASUREMENT_CYCLE_QUEUE 80
#define MEASUREMENT_LOCK_TIMEOUT (3 * 60000) /** ms — cap loop() stall when a post holds the queue lock */

/** I2C define */
#define I2C_SDA_PIN 7
#define I2C_SCL_PIN 6
#define OLED_I2C_ADDR 0x3C

/** Power pin */
#define GPIO_POWER_MODULE_PIN 5
#define GPIO_EXPANSION_CARD_POWER 4
#define GPIO_IIC_RESET 3

#define MINUTES() ((uint32_t)(esp_timer_get_time() / 1000 / 1000 / 60))

static MqttClient mqttClient(Serial);
static TaskHandle_t mqttTask = NULL;
static Configuration configuration(Serial);
static Measurements measurements(configuration);
static AirGradient *ag;
static AgHardwareIdentity hardwareIdentity;
static bool oledDetected = false;
static AgSatellites *satellites = nullptr;
static OledDisplay oledDisplay(configuration, measurements, Serial);
static StateMachine stateMachine(oledDisplay, Serial, measurements, configuration);
static WifiConnector wifiConnector(oledDisplay, Serial, stateMachine, configuration);
static OpenMetrics openMetrics(measurements, configuration, wifiConnector);
static LocalServer localServer(Serial, openMetrics, measurements, configuration, wifiConnector);
static AgSerial *agSerial;
static CellularModule *cellularCard;
static AirgradientClient *agClient;

enum NetworkOption { UseWifi, UseCellular };
NetworkOption networkOption;
static TaskHandle_t mainTaskHandle = NULL;
TaskHandle_t handleNetworkTask = NULL;
static bool firmwareUpdateInProgress = false;

static uint32_t factoryBtnPressTime = 0;
static AgFirmwareMode fwMode = FW_MODE_I_9PSL;
static bool ledBarButtonTest = false;
static String fwNewVersion;
static int lastCellSignalQuality = 99; // CSQ

// Default value is 0, indicate its not started yet
// In minutes
uint32_t agCeClientProblemDetectedTime = 0;

SemaphoreHandle_t mutexMeasurementCycleQueue;
static AirgradientClient::AirgradientPayload measurementPayload;

static void boardInit(void);
static void initializeNetwork();
static void failedHandler(String msg);
static void configurationUpdateSchedule(void);
static void configUpdateHandle(void);
static void updateDisplayAndLedBar(void);
static void updateTvoc(void);
static void updatePm(void);
static void updateSPS30(SPS30 &sensor, int channel);
static void sendDataToServer(void);
static void tempHumUpdate(void);
static void co2Update(void);
static void printMeasurements();
static void mdnsInit(void);
static void createMqttTask(void);
static void initMqtt(void);
static void factoryConfigReset(void);
static void wdgFeedUpdate(void);
static void ledBarEnabledUpdate(void);
static bool sgp41Init(void);
static void checkForFirmwareUpdate(void);
static void otaHandlerCallback(AirgradientOTA::OtaResult result, const char *msg);
static void displayExecuteOta(AirgradientOTA::OtaResult result, String msg, int processing);
static int calculateMaxPeriod(int updateInterval);
static void setMeasurementMaxPeriod();
static void newMeasurementCycle();
static void restartIfCeClientIssueOverTwoHours();
static void networkSignalCheck();
static void networkingTask(void *args);
static AirgradientClient::PayloadType getClientPayloadType();
static AirgradientClient::CommonPayload buildCommonPayload(Measurements::Measures &mc);
static void saveOperatorState();
static void restoreOperatorState();
static BoardType getBoardType();
static void requestBoardSelectionReboot();

AgSchedule dispLedSchedule(DISP_UPDATE_INTERVAL, updateDisplayAndLedBar);
AgSchedule configSchedule(WIFI_SERVER_CONFIG_SYNC_INTERVAL, configurationUpdateSchedule);
AgSchedule transmissionSchedule(WIFI_TRANSMISSION_INTERVAL, sendDataToServer);
AgSchedule measurementSchedule(WIFI_MEASUREMENT_INTERVAL, newMeasurementCycle);
AgSchedule co2Schedule(SENSOR_CO2_UPDATE_INTERVAL, co2Update);
AgSchedule pmsSchedule(SENSOR_PM_UPDATE_INTERVAL, updatePm);
AgSchedule tempHumSchedule(SENSOR_TEMP_HUM_UPDATE_INTERVAL, tempHumUpdate);
AgSchedule tvocSchedule(SENSOR_TVOC_UPDATE_INTERVAL, updateTvoc);
AgSchedule watchdogFeedSchedule(60000, wdgFeedUpdate);
AgSchedule checkForUpdateSchedule(FIRMWARE_CHECK_FOR_UPDATE_MS, checkForFirmwareUpdate);
AgSchedule networkSignalCheckSchedule(10000, networkSignalCheck);
AgSchedule printMeasurementsSchedule(6000, printMeasurements);

void setup() {
  /** Serial for print debug message */
  Serial.begin(115200);
  delay(100); /** For bester show log */
  mainTaskHandle = xTaskGetCurrentTaskHandle();
  hardwareIdentity.begin();

  // Enable cullular module power board
  pinMode(GPIO_EXPANSION_CARD_POWER, OUTPUT);
  digitalWrite(GPIO_EXPANSION_CARD_POWER, HIGH);

  // Set reason why esp is reset
  esp_reset_reason_t reason = esp_reset_reason();
  measurements.setResetReason(reason);

  /** Initialize local configure */
  configuration.begin();
  configuration.setConfigurationUpdatedCallback(configUpdateHandle);

  /** Init I2C */
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  delay(1000);

  /** Detect board type from eFuse, OLED, or persisted model. */
  Wire.beginTransmission(OLED_I2C_ADDR);
  oledDetected = Wire.endTransmission() == 0x00;
  ag = new AirGradient(getBoardType());
  Serial.println("Detected " + ag->getBoardName());
  Serial.printf("Board selection: eFuse=%s (0x%02X), OLED=%s, model=%s, "
                "selected=%s\n",
                hardwareIdentity.getValueName(), hardwareIdentity.getRawValue(),
                oledDetected ? "detected" : "not detected",
                configuration.getModel().c_str(), ag->getBoardName().c_str());

  /** Print device ID into log */
  Serial.println("Serial nr: " + ag->deviceId());

  configuration.setAirGradient(ag);
  oledDisplay.setAirGradient(ag);
  stateMachine.setAirGradient(ag);
  wifiConnector.setAirGradient(ag);
  openMetrics.setAirGradient(ag);
  localServer.setAirGraident(ag);
  measurements.setAirGradient(ag);

  if (configuration.isSatellitesEnabled()) {
    satellites = new AgSatellites(measurements, configuration);
    measurements.setSatellites(satellites);
    Serial.println("Satellites enabled on boot");
  }

  /** Init sensor */
  boardInit();
  setMeasurementMaxPeriod();

  bool connectToNetwork = true;
  if (ag->isOne()) { // Offline mode only available for indoor monitor
    /** Show message confirm offline mode, should me perform if LED bar button
     * test pressed */
    if (ledBarButtonTest == false) {
      oledDisplay.setText("Press now for",
                          configuration.isOfflineMode() ? "online mode" : "offline mode", "");
      uint32_t startTime = millis();
      while (true) {
        if (ag->button.getState() == ag->button.BUTTON_PRESSED) {
          configuration.setOfflineMode(!configuration.isOfflineMode());

          oledDisplay.setText("Offline Mode",
                              configuration.isOfflineMode() ? " = True" : "  = False", "");
          delay(1000);
          break;
        }
        uint32_t periodMs = (uint32_t)(millis() - startTime);
        if (periodMs >= 3000) {
          break;
        }
      }
      connectToNetwork = !configuration.isOfflineMode();
    } else {
      configuration.setOfflineModeWithoutSave(true);
      connectToNetwork = false;
    }
  }

  // Initialize networking configuration
  if (connectToNetwork) {
    oledDisplay.setText("Initialize", "network...", "");
    initializeNetwork();
    wifiConnector.stopBLE();
  }

  /** Show display Warning up */
  if (ag->isOne()) {
    oledDisplay.setText("Warming Up", "Serial Number:", ag->deviceId().c_str());
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);

    Serial.println("Display brightness: " + String(configuration.getDisplayBrightness()));
    oledDisplay.setBrightness(configuration.getDisplayBrightness());
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
  }

  if (networkOption == UseCellular) {
    // If using cellular re-set scheduler interval
    configSchedule.setPeriod(CELLULAR_SERVER_CONFIG_SYNC_INTERVAL);
    transmissionSchedule.setPeriod(CELLULAR_TRANSMISSION_INTERVAL);
    measurementSchedule.setPeriod(CELLULAR_MEASUREMENT_INTERVAL);
    measurementSchedule.update();
    // Queue now only applied for cellular
    // Initialize mutex to access measurementPayload
    mutexMeasurementCycleQueue = xSemaphoreCreateMutex();
  }

  // Only run network task if monitor is not in offline mode
  if (configuration.isOfflineMode() == false) {
    BaseType_t xReturned =
        xTaskCreate(networkingTask, "NetworkingTask", 8192, null, 5, &handleNetworkTask);
    if (xReturned == pdPASS) {
      Serial.println("Success create networking task");
    } else {
      assert("Failed to create networking task");
    }
  }

  // Log monitor mode for debugging purpose
  if (configuration.isOfflineMode()) {
    Serial.println("Running monitor in offline mode");
  } else if (configuration.isCloudConnectionDisabled()) {
    Serial.println("Running monitor without connection to AirGradient server");
  }
}

void loop() {
  if (networkOption == UseCellular) {
    // Check if cellular client not ready until certain time
    // Redundant check in both task to make sure its executed
    restartIfCeClientIssueOverTwoHours();
  }

  // Schedule to feed external watchdog
  watchdogFeedSchedule.run();

  if (firmwareUpdateInProgress) {
    // Firmare update currently in progress, temporarily disable running sensor schedules
    delay(10000);
    return;
  }

  if (ulTaskNotifyTake(pdTRUE, 0) > 0) {
    Serial.printf("Rebooting in %u ms to apply hardware board selection\n",
                  BOARD_SELECTION_REBOOT_DELAY_MS);
    if (ag->isOne()) {
      oledDisplay.setText("Hardware changed", "Rebooting...", "");
    }
    delay(BOARD_SELECTION_REBOOT_DELAY_MS);
    esp_restart();
  }

  // Schedule to update display and led
  dispLedSchedule.run();

  if (networkOption == UseCellular) {
    // Queue now only applied for cellular
    measurementSchedule.run();
  }

  if (configuration.hasSensorS8) {
    co2Schedule.run();
  }
  if (configuration.hasSensorPMS1 || configuration.hasSensorPMS2 ||
      configuration.hasSensorSPS30_1 || configuration.hasSensorSPS30_2) {
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
      static bool pmsConnected = false;
      if (pmsConnected != ag->pms5003.connected()) {
        pmsConnected = ag->pms5003.connected();
        Serial.printf("PMS sensor %s \n", pmsConnected ? "connected" : "removed");
      }
    }
  } else {
    if (configuration.hasSensorPMS1) {
      ag->pms5003t_1.handle();
    }
    if (configuration.hasSensorPMS2) {
      ag->pms5003t_2.handle();
    }
  }

  /* Run satellite BLE scanning */
  if (satellites != nullptr) {
    satellites->run();
  }

  /* Run measurement schedule */
  printMeasurementsSchedule.run();

  /** factory reset handle */
  factoryConfigReset();

  if (configuration.isCommandRequested()) {
    // Each state machine already has an independent request command check
    stateMachine.executeCo2Calibration();
    stateMachine.executeLedBarTest();
  }
}

static void co2Update(void) {
  if (!configuration.hasSensorS8) {
    // Device don't have S8 sensor
    return;
  }

  int value = ag->s8.getCo2();
  if (utils::isValidCO2(value)) {
    measurements.update(Measurements::CO2, value);
  } else {
    measurements.update(Measurements::CO2, utils::getInvalidCO2());
  }
}

void printMeasurements() { measurements.printCurrentAverage(); }

static void mdnsInit(void) {
  if (!MDNS.begin(localServer.getHostname().c_str())) {
    Serial.println("Init mDNS failed");
    return;
  }

  MDNS.addService("_airgradient", "_tcp", 80);
  MDNS.addServiceTxt("_airgradient", "_tcp", "model", AgFirmwareModeName(fwMode));
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
            String payload = measurements.toString(true, fwMode, wifiConnector.RSSI());
            String topic = "airgradient/readings/" + ag->deviceId();

            if (mqttClient.publish(topic.c_str(), payload.c_str(), payload.length())) {
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
    Serial.println("MQTT is not configured, skipping initialization of MQTT client");
    return;
  }

  if (networkOption == UseCellular) {
    Serial.println("MQTT not available for cellular options");
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
            oledDisplay.setText("", "", "");
            ESP.restart();
          }
        }

        /** Show current content cause reset ignore */
        factoryBtnPressTime = 0;
        if (ag->isOne()) {
          updateDisplayAndLedBar();
        }
      }
    }
  } else {
    if (factoryBtnPressTime != 0) {
      if (ag->isOne()) {
        /** Restore last display content */
        updateDisplayAndLedBar();
      }
    }
    factoryBtnPressTime = 0;
  }
}

static void wdgFeedUpdate(void) {
  ag->watchdog.reset();
  Serial.println("External watchdog feed!");
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
    Serial.println("Init SGP41 failure");
    configuration.hasSensorSGP = false;
  }
  return false;
}

void checkForFirmwareUpdate(void) {
  if (configuration.isCloudConnectionDisabled()) {
    Serial.println("Cloud connection is disabled, skip firmware update");
    return;
  }

  AirgradientOTA *agOta;
  if (networkOption == UseWifi) {
    agOta = new AirgradientOTAWifi;
  } else {
    agOta = new AirgradientOTACellular(cellularCard, agClient->getICCID());
  }

  // Indicate main task that firmware update is in progress
  firmwareUpdateInProgress = true;

  agOta->setHandlerCallback(otaHandlerCallback);

  String httpDomain = configuration.getHttpDomain();
  if (httpDomain != "") {
    Serial.printf("httpDomain configuration available, start OTA with custom domain\n",
                  httpDomain.c_str());
    agOta->updateIfAvailable(ag->deviceId().c_str(), GIT_VERSION, httpDomain.c_str());
  } else {
    agOta->updateIfAvailable(ag->deviceId().c_str(), GIT_VERSION);
  }

  // Only goes to this line if firmware update is not success
  // Handled by otaHandlerCallback

  // Indicate main task that firmware update finish
  firmwareUpdateInProgress = false;

  delete agOta;
  Serial.println();
}

void otaHandlerCallback(AirgradientOTA::OtaResult result, const char *msg) {
  switch (result) {
  case AirgradientOTA::Starting: {
    Serial.println("Firmware update starting...");
    if (configuration.hasSensorSGP && networkOption == UseCellular) {
      // Temporary pause SGP41 task while cellular firmware update is in progress
      ag->sgp41.pause();
    }
    displayExecuteOta(result, fwNewVersion, 0);
    break;
  }
  case AirgradientOTA::InProgress:
    Serial.printf("OTA progress: %s\n", msg);
    displayExecuteOta(result, "", std::stoi(msg));
    break;
  case AirgradientOTA::Failed:
    displayExecuteOta(result, "", 0);
    if (configuration.hasSensorSGP && networkOption == UseCellular) {
      ag->sgp41.resume();
    }
    break;
  case AirgradientOTA::Skipped:
  case AirgradientOTA::AlreadyUpToDate:
    displayExecuteOta(result, "", 0);
    break;
  case AirgradientOTA::Success:
    displayExecuteOta(result, "", 0);
    esp_restart();
    break;
  default:
    break;
  }
}

static void displayExecuteOta(AirgradientOTA::OtaResult result, String msg, int processing) {
  switch (result) {
  case AirgradientOTA::Starting:
    if (ag->isOne()) {
      oledDisplay.showFirmwareUpdateVersion(msg);
    } else {
      Serial.println("New firmware: " + msg);
    }
    delay(2500);
    break;
  case AirgradientOTA::Failed:
    if (ag->isOne()) {
      oledDisplay.showFirmwareUpdateFailed();
    } else {
      Serial.println("Error: Firmware update: failed");
    }
    delay(2500);
    break;
  case AirgradientOTA::Skipped:
    if (ag->isOne()) {
      oledDisplay.showFirmwareUpdateSkipped();
    } else {
      Serial.println("Firmware update: Skipped");
    }
    delay(2500);
    break;
  case AirgradientOTA::AlreadyUpToDate:
    if (ag->isOne()) {
      oledDisplay.showFirmwareUpdateUpToDate();
    } else {
      Serial.println("Firmware update: up to date");
    }
    delay(2500);
    break;
  case AirgradientOTA::InProgress:
    if (ag->isOne()) {
      oledDisplay.showFirmwareUpdateProgress(processing);
    } else {
      Serial.println("Firmware update: " + String(processing) + String("%"));
    }
    break;
  case AirgradientOTA::Success: {
    Serial.println("OTA update performed, restarting ...");
    int i = 3;
    while (i != 0) {
      i = i - 1;
      if (ag->isOne()) {
        oledDisplay.showFirmwareUpdateSuccess(i);
      } else {
        Serial.println("Rebooting... " + String(i));
      }
      delay(1000);
    }

    if (ag->isOne()) {
      oledDisplay.setBrightness(0);
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
  wifiConnector.bleNotifyStatus(PROV_CONNECTING_TO_SERVER);

  /** Task handle led connecting animation */
  xTaskCreate(
      [](void *obj) {
        for (;;) {
          // ledSmHandler();
          stateMachine.handleLeds();
          if (stateMachine.getLedState() != AgStateMachineWiFiOkServerConnecting) {
            break;
          }
          delay(LED_BAR_ANIMATION_PERIOD);
        }
        vTaskDelete(NULL);
      },
      "task_led", 2048, NULL, 5, NULL);

  delay(1500);

  // Build payload to check connection to airgradient server
  JSONVar root;
  root["wifi"] = wifiConnector.RSSI();
  root["boot"] = measurements.bootCount();
  std::string payload = JSON.stringify(root).c_str();
  if (agClient->httpPostMeasures(payload)) {
    if (ag->isOne()) {
      stateMachine.displayHandle(AgStateMachineWiFiOkServerConnected);
    }
    stateMachine.handleLeds(AgStateMachineWiFiOkServerConnected);
    wifiConnector.bleNotifyStatus(PROV_SERVER_REACHABLE);
  } else {
    if (ag->isOne()) {
      stateMachine.displayHandle(AgStateMachineWiFiOkServerConnectFailed);
    }
    stateMachine.handleLeds(AgStateMachineWiFiOkServerConnectFailed);
    wifiConnector.bleNotifyStatus(PROV_ERR_SERVER_UNREACHABLE);
  }

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

  oledDisplay.setText("AirGradient ONE", "FW Version: ", ag->getVersion().c_str());
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
      oledDisplay.setText("Rebooting...", "", "");
      delay(2500);
      oledDisplay.setText("", "", "");
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

  /** Init PMS5003, fallback to SPS30 if not found */
  if (ag->pms5003.begin(Serial0) == false) {
    Serial.println("PMS5003 not found, trying SPS30...");
    configuration.hasSensorPMS1 = false;

    if (ag->sps30_1.begin(Serial0)) {
      Serial.println("SPS30 detected on Serial0");
      configuration.hasSensorSPS30_1 = true;
    } else {
      Serial.println("SPS30 not found either");
      dispSensorNotFound("PM sensor");
    }
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

  /**
   * Attempt to detect PM sensors on available serial ports.
   * Per-port order: PMS5003T first (@9600), fallback to SPS30 (@115200).
   * For single-PM modes the detected sensor is always assigned to channel 1.
   * For dual-PM modes Serial0 → channel 1, Serial1 → channel 2.
   */
  auto detectPmOnSerial = [](HardwareSerial &serial, PMS5003T &pms, SPS30 &sps,
                             const char *portName) -> int {
    // Returns: 0 = none, 1 = PMS5003T, 2 = SPS30
    if (pms.begin(serial)) {
      Serial.printf("Detected PMS5003T on %s\n", portName);
      return 1;
    }
    Serial.printf("PMS5003T not found on %s, trying SPS30...\n", portName);
    if (sps.begin(serial)) {
      Serial.printf("Detected SPS30 on %s\n", portName);
      return 2;
    }
    Serial.printf("No PM sensor detected on %s\n", portName);
    return 0;
  };

  if (fwMode == FW_MODE_O_1PST || fwMode == FW_MODE_O_1PS) {
    // Single PM channel expected — try Serial0 first, fallback Serial1
    configuration.hasSensorPMS1 = false;
    configuration.hasSensorPMS2 = false;
    bool pmFound = false;

    if (serial0Available) {
      int result =
          detectPmOnSerial(Serial0, ag->pms5003t_1, ag->sps30_1, "Serial0");
      if (result == 1) {
        configuration.hasSensorPMS1 = true;
        serial0Available = false;
        pmFound = true;
      } else if (result == 2) {
        configuration.hasSensorSPS30_1 = true;
        serial0Available = false;
        pmFound = true;
      }
    }
    if (!pmFound && serial1Available) {
      int result =
          detectPmOnSerial(Serial1, ag->pms5003t_1, ag->sps30_1, "Serial1");
      if (result == 1) {
        configuration.hasSensorPMS1 = true;
        serial1Available = false;
      } else if (result == 2) {
        configuration.hasSensorSPS30_1 = true;
        serial1Available = false;
      }
    }
  } else {
    // Dual PM channel modes (O_1PPT / O_1PP) — Serial0 → ch1, Serial1 → ch2
    configuration.hasSensorPMS1 = false;
    configuration.hasSensorPMS2 = false;

    // Channel 1 on Serial0
    int result1 =
        detectPmOnSerial(Serial0, ag->pms5003t_1, ag->sps30_1, "Serial0");
    if (result1 == 1) {
      configuration.hasSensorPMS1 = true;
    } else if (result1 == 2) {
      configuration.hasSensorSPS30_1 = true;
    }

    // Channel 2 on Serial1
    int result2 =
        detectPmOnSerial(Serial1, ag->pms5003t_2, ag->sps30_2, "Serial1");
    if (result2 == 1) {
      configuration.hasSensorPMS2 = true;
    } else if (result2 == 2) {
      configuration.hasSensorSPS30_2 = true;
    }

    // Check if we should downgrade from two-PM to single-PM mode
    if (fwMode == FW_MODE_O_1PP) {
      bool ch1HasPm =
          configuration.hasSensorPMS1 || configuration.hasSensorSPS30_1;
      bool ch2HasPm =
          configuration.hasSensorPMS2 || configuration.hasSensorSPS30_2;
      if (ch1HasPm != ch2HasPm) {
        fwMode = FW_MODE_O_1P;
      }
    }
  }

  /** Update the PMS poll period based on fw mode and sensor availability */
  if (fwMode != FW_MODE_O_1PST && fwMode != FW_MODE_O_1PS) {
    bool ch1HasPm =
        configuration.hasSensorPMS1 || configuration.hasSensorSPS30_1;
    bool ch2HasPm =
        configuration.hasSensorPMS2 || configuration.hasSensorSPS30_2;
    if (ch1HasPm && ch2HasPm) {
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

    ag->s8.printInformation();
  }

  localServer.setFwMode(fwMode);
}

static void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    vTaskDelay(1000);
  }
}

static AirgradientClient::PayloadType getClientPayloadType() {
  if (!ag->isOne() && (fwMode == FW_MODE_O_1PPT || fwMode == FW_MODE_O_1PP)) {
    return AirgradientClient::ONE_OPENAIR_TWO_PMS;
  }

  return AirgradientClient::ONE_OPENAIR;
}

// Map averaged sensor measures into the client CommonPayload struct. Values are
// stored raw (the client encoder applies scaling); invalid readings use the
// utils invalid sentinels, which the client IS_*_VALID macros treat as invalid.
static AirgradientClient::CommonPayload buildCommonPayload(Measurements::Measures &mc) {
  AirgradientClient::CommonPayload c;

  // CO2, Temperature, Humidity
  c.rco2 = utils::isValidCO2(mc.co2) ? static_cast<int>(mc.co2 + 0.5f) : utils::getInvalidCO2();
  c.atmp = Measurements::avgTempHum(mc.temperature[0], mc.temperature[1],
                                    &utils::isValidTemperature, utils::getInvalidTemperature());
  c.rhum = Measurements::avgTempHum(mc.humidity[0], mc.humidity[1], &utils::isValidHumidity,
                                    utils::getInvalidHumidity());

  // PM mass (atmospheric environment); pm25 kept per-channel
  c.pm01 = Measurements::avgPm(mc.pm_01[0], mc.pm_01[1]);
  c.pm25[0] = utils::isValidPm(mc.pm_25[0]) ? mc.pm_25[0]
                                            : static_cast<float>(utils::getInvalidPmValue());
  c.pm25[1] = utils::isValidPm(mc.pm_25[1]) ? mc.pm_25[1]
                                            : static_cast<float>(utils::getInvalidPmValue());
  c.pm10 = Measurements::avgPm(mc.pm_10[0], mc.pm_10[1]);

  // PM2.5 standard particle (per-channel)
  c.pm25Sp[0] = utils::isValidPm(mc.pm_25_sp[0]) ? mc.pm_25_sp[0]
                                                 : static_cast<float>(utils::getInvalidPmValue());
  c.pm25Sp[1] = utils::isValidPm(mc.pm_25_sp[1]) ? mc.pm_25_sp[1]
                                                 : static_cast<float>(utils::getInvalidPmValue());

  // Particle counts; 0.3 count kept per-channel, the rest averaged
  c.particleCount003[0] = utils::isValidPm03Count(mc.pm_03_pc[0])
                              ? static_cast<int>(mc.pm_03_pc[0] + 0.5f)
                              : utils::getInvalidPmValue();
  c.particleCount003[1] = utils::isValidPm03Count(mc.pm_03_pc[1])
                              ? static_cast<int>(mc.pm_03_pc[1] + 0.5f)
                              : utils::getInvalidPmValue();
  c.particleCount005 = Measurements::avgCount(mc.pm_05_pc[0], mc.pm_05_pc[1]);
  c.particleCount01 = Measurements::avgCount(mc.pm_01_pc[0], mc.pm_01_pc[1]);
  c.particleCount02 = Measurements::avgCount(mc.pm_25_pc[0], mc.pm_25_pc[1]);
  c.particleCount50 = Measurements::avgCount(mc.pm_5_pc[0], mc.pm_5_pc[1]);
  c.particleCount10 = Measurements::avgCount(mc.pm_10_pc[0], mc.pm_10_pc[1]);

  // TVOC / NOx (index and raw)
  c.tvoc = utils::isValidVOC(mc.tvoc) ? static_cast<int>(mc.tvoc + 0.5f) : utils::getInvalidVOC();
  c.tvocRaw =
      utils::isValidVOC(mc.tvoc_raw) ? static_cast<int>(mc.tvoc_raw + 0.5f) : utils::getInvalidVOC();
  c.nox = utils::isValidNOx(mc.nox) ? static_cast<int>(mc.nox + 0.5f) : utils::getInvalidNOx();
  c.noxRaw =
      utils::isValidNOx(mc.nox_raw) ? static_cast<int>(mc.nox_raw + 0.5f) : utils::getInvalidNOx();

  return c;
}

static void restoreOperatorState() {
  String ops = configuration.getCellOperators();
  if (ops.length() == 0) {
    Serial.println("No saved operator state to restore");
    return;
  }
  uint32_t opId = configuration.getCellOperatorId();
  uint32_t failCount = configuration.getCellOperatorFailCount();
  if (cellularCard->setOperators(ops.c_str(), opId, failCount)) {
    Serial.printf("Restored operator state: id=%u, failCount=%u, list=%s\n", opId,
                  failCount, ops.c_str());
  } else {
    Serial.println("Failed to restore operator state");
  }
}

static void saveOperatorState() {
  String ops = cellularCard->getSerializedOperators().c_str();
  uint32_t opId = cellularCard->getCurrentOperatorId();
  uint32_t failCount = cellularCard->getRegistrationFailCount();
  configuration.setCellOperatorState(ops, opId, failCount);
  Serial.printf("Saved operator state: id=%u, failCount=%u, list=%s\n", opId,
                failCount, ops.c_str());
}

void initializeNetwork() {
  // Check if cellular module available
  agSerial = new AgSerial(Wire);
  agSerial->init(GPIO_IIC_RESET);
  if (agSerial->open()) {
    Serial.println("Cellular module found");
    // Initialize cellular module and use cellular as agClient
    cellularCard = new CellularModuleA7672XX(agSerial, GPIO_POWER_MODULE_PIN);
    // Restore previously saved operator state before registration
    restoreOperatorState();
    agClient = new AirgradientCellularClient(cellularCard);
    networkOption = UseCellular;
  } else {
    Serial.println("Cellular module not available, using wifi");
    delete agSerial;
    agSerial = nullptr;
    // Use wifi as agClient
    agClient = new AirgradientWifiClient;
    networkOption = UseWifi;
  }

  if (networkOption == UseCellular) {
    // Enable serial stream debugging to check the AT command when doing registration
    agSerial->setDebug(true);
  }

  String httpDomain = configuration.getHttpDomain();
  if (httpDomain != "") {
    agClient->setHttpDomain(httpDomain.c_str());
    Serial.printf("HTTP domain name is set to: %s\n", httpDomain.c_str());
    oledDisplay.setText("HTTP domain name", "using local", "configuration");
    delay(2500);
  }

  agClient->setExtendedPmMeasures(configuration.isExtendedPmMeasuresEnabled());

  if (!agClient->begin(ag->deviceId().c_str(), getClientPayloadType())) {
    if (networkOption == UseCellular) {
      saveOperatorState();
    }
    oledDisplay.setText("Client", "initialization", "failed");
    delay(5000);
    oledDisplay.showRebooting();
    delay(2500);
    oledDisplay.setText("", "", "");
    ESP.restart();
  }

  // Save operator state after successful registration
  if (networkOption == UseCellular) {
    saveOperatorState();
  }

  // Provide openmetrics to have access to last transmission result
  openMetrics.setAirgradientClient(agClient);

  if (networkOption == UseCellular) {
    // Disabling it again
    agSerial->setDebug(false);
  }

  if (networkOption == UseWifi) {
    String modelName = AgFirmwareModeName(fwMode);
    if (!wifiConnector.connect(modelName)) {
      Serial.println("Cannot initiate wifi connection");
      return;
    }

    if (!wifiConnector.isConnected()) {
      Serial.println("Failed connect to WiFi");
      oledDisplay.showRebooting();
      delay(2500);
      oledDisplay.setText("", "", "");
      ESP.restart();
    }

    // Initiate local network configuration
    mdnsInit();
    localServer.begin();
    // Apply mqtt connection if configured
    initMqtt();

    // Ignore the rest if cloud connection to AirGradient is disabled
    if (configuration.isCloudConnectionDisabled()) {
      return;
    }

    // Send data for the first time to AG server at boot only if postDataToAirgradient is enabled
    if (configuration.isPostDataToAirGradient()) {
      sendDataToAg();
    }
  }

  // Skip fetch configuration if configuration control is set to "local" only
  if (configuration.getConfigurationControl() == ConfigurationControl::ConfigurationControlLocal) {
    ledBarEnabledUpdate();
    return;
  }

  std::string config;
  if (networkOption == UseCellular) {
    config = agClient->coapFetchConfig();
  } else {
    config = agClient->httpFetchConfig();
  }
  configSchedule.update();
  // Check if fetch configuration failed or fetch succes but parsing failed
  if (agClient->isLastFetchConfigSucceed() == false ||
      configuration.parse(config.c_str(), false) == false) {
    if (ag->isOne()) {
      if (agClient->isRegisteredOnAgServer() == false) {
        stateMachine.displaySetAddToDashBoard();
        stateMachine.displayHandle(AgStateMachineWiFiOkServerOkSensorConfigFailed);
        wifiConnector.bleNotifyStatus(PROV_ERR_MONITOR_NOT_REGISTERED);
      } else {
        stateMachine.displayClearAddToDashBoard();
        wifiConnector.bleNotifyStatus(PROV_ERR_GET_MONITOR_CONFIG_FAILED);
      }
    }
    stateMachine.handleLeds(AgStateMachineWiFiOkServerOkSensorConfigFailed);
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
  } else {
    ledBarEnabledUpdate();
    wifiConnector.bleNotifyStatus(PROV_MONITOR_CONFIGURED);
  }
}

static void configurationUpdateSchedule(void) {
  if (configuration.getConfigurationControl() == ConfigurationControl::ConfigurationControlLocal) {
    Serial.println("Ignore fetch server configuration, configurationControl set to local");
    agClient->resetFetchConfigurationStatus();
    return;
  }

  std::string config;
  if (networkOption == UseCellular) {
    config = agClient->coapFetchConfig();
  } else {
    config = agClient->httpFetchConfig();
  }
  if (agClient->isLastFetchConfigSucceed()) {
    configuration.parse(config.c_str(), false);
  }
}

static void configUpdateHandle() {
  if (configuration.isUpdated() == false) {
    return;
  }

  String mqttUri = configuration.getMqttBrokerUri();
  if (mqttClient.isCurrentUri(mqttUri) == false) {
    mqttClient.end();
    initMqtt();
  }

  String httpDomain = configuration.getHttpDomain();
  if (httpDomain != "") {
    Serial.printf("HTTP domain name set to: %s\n", httpDomain.c_str());
    agClient->setHttpDomain(httpDomain.c_str());
  } else {
    // Its empty, set to default
    Serial.println("HTTP domain name from configuration empty, set to default");
    agClient->setHttpDomainDefault();
  }

  agClient->setExtendedPmMeasures(configuration.isExtendedPmMeasuresEnabled());

  if (configuration.hasSensorSGP) {
    if (configuration.noxLearnOffsetChanged() || configuration.tvocLearnOffsetChanged()) {
      ag->sgp41.end();

      int oldTvocOffset = ag->sgp41.getTvocLearningOffset();
      int oldNoxOffset = ag->sgp41.getNoxLearningOffset();
      bool result = sgp41Init();
      const char *resultStr = "successful";
      if (!result) {
        resultStr = "failure";
      }
      if (oldTvocOffset != configuration.getTvocLearningOffset()) {
        Serial.printf("Setting tvocLearningOffset from %d to %d hours %s\r\n", oldTvocOffset,
                      configuration.getTvocLearningOffset(), resultStr);
      }
      if (oldNoxOffset != configuration.getNoxLearningOffset()) {
        Serial.printf("Setting noxLearningOffset from %d to %d hours %s\r\n", oldNoxOffset,
                      configuration.getNoxLearningOffset(), resultStr);
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
        if (configuration.getLedBarMode() == LedBarMode::LedBarModeOff) {
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
  }

  if (configuration.isSatellitesChanged() && configuration.isSatellitesEnabled()) {
    if (satellites == nullptr) {
      // Initialized if satellites enabled on run time
      satellites = new AgSatellites(measurements, configuration);
      measurements.setSatellites(satellites);
      Serial.println("Satellites enabled on runtime");
    }
  }

  requestBoardSelectionReboot();

  // Update display and led bar notification based on updated configuration
  updateDisplayAndLedBar();
}

static BoardType getBoardType() {
  return hardwareIdentity.resolve(oledDetected, configuration.getModel());
}

static void requestBoardSelectionReboot() {
  if (hardwareIdentity.isProvisioned()) {
    return;
  }

  const BoardType boardType = getBoardType();
  if (boardType == ag->getBoardType()) {
    return;
  }

  Serial.printf("Board selection changed: OLED=%s, model=%s, selected=%s\n",
                oledDetected ? "detected" : "not detected",
                configuration.getModel().c_str(),
                boardType == BoardType::ONE_INDOOR ? "ONE_INDOOR"
                                                   : "OPEN_AIR_OUTDOOR");
  if (mainTaskHandle == NULL) {
    Serial.println(
        "Cannot request board selection reboot: main task unavailable");
    return;
  }
  xTaskNotifyGive(mainTaskHandle);
}

static void updateDisplayAndLedBar(void) {
  if (factoryBtnPressTime != 0) {
    // Do not distrub factory reset sequence countdown
    return;
  }

  if (configuration.isOfflineMode()) {
    // Ignore network related status when in offline mode
    stateMachine.displayHandle(AgStateMachineNormal);
    stateMachine.handleLeds(AgStateMachineNormal);
    return;
  }

  if (networkOption == UseWifi) {
    if (wifiConnector.isConnected() == false) {
      stateMachine.displayHandle(AgStateMachineWiFiLost);
      stateMachine.handleLeds(AgStateMachineWiFiLost);
      return;
    }
  } else if (networkOption == UseCellular) {
    if (agClient->isClientReady() == false) {
      // Same action as wifi
      stateMachine.displayHandle(AgStateMachineWiFiLost);
      stateMachine.handleLeds(AgStateMachineWiFiLost);
      return;
    }
  }

  if (configuration.isCloudConnectionDisabled()) {
    // Ignore API related check since cloud is disabled
    stateMachine.displayHandle(AgStateMachineNormal);
    stateMachine.handleLeds(AgStateMachineNormal);
    return;
  }

  AgStateMachineState state = AgStateMachineNormal;
  if (agClient->isLastFetchConfigSucceed() == false) {
    state = AgStateMachineSensorConfigFailed;
    if (agClient->isRegisteredOnAgServer() == false) {
      stateMachine.displaySetAddToDashBoard();
    } else {
      stateMachine.displayClearAddToDashBoard();
    }
  } else if (agClient->isLastPostMeasureSucceed() == false &&
             configuration.isPostDataToAirGradient()) {
    state = AgStateMachineServerLost;
  }

  stateMachine.displayHandle(state);
  stateMachine.handleLeds(state);
}

static void updateTvoc(void) {
  if (!configuration.hasSensorSGP) {
    return;
  }

  measurements.update(Measurements::TVOC, ag->sgp41.getTvocIndex());
  measurements.update(Measurements::TVOCRaw, ag->sgp41.getTvocRaw());
  measurements.update(Measurements::NOx, ag->sgp41.getNoxIndex());
  measurements.update(Measurements::NOxRaw, ag->sgp41.getNoxRaw());
}

static void updatePMS5003() {
  if (ag->pms5003.connected()) {
    measurements.update(Measurements::PM01, ag->pms5003.getPm01Ae());
    measurements.update(Measurements::PM25, ag->pms5003.getPm25Ae());
    measurements.update(Measurements::PM10, ag->pms5003.getPm10Ae());
    measurements.update(Measurements::PM01_SP, ag->pms5003.getPm01Sp());
    measurements.update(Measurements::PM25_SP, ag->pms5003.getPm25Sp());
    measurements.update(Measurements::PM10_SP, ag->pms5003.getPm10Sp());
    measurements.update(Measurements::PM03_PC, ag->pms5003.getPm03ParticleCount());
    measurements.update(Measurements::PM05_PC, ag->pms5003.getPm05ParticleCount());
    measurements.update(Measurements::PM01_PC, ag->pms5003.getPm01ParticleCount());
    measurements.update(Measurements::PM25_PC, ag->pms5003.getPm25ParticleCount());
    measurements.update(Measurements::PM5_PC, ag->pms5003.getPm5ParticleCount());
    measurements.update(Measurements::PM10_PC, ag->pms5003.getPm10ParticleCount());
  } else {
    measurements.update(Measurements::PM01, utils::getInvalidPmValue());
    measurements.update(Measurements::PM25, utils::getInvalidPmValue());
    measurements.update(Measurements::PM10, utils::getInvalidPmValue());
    measurements.update(Measurements::PM01_SP, utils::getInvalidPmValue());
    measurements.update(Measurements::PM25_SP, utils::getInvalidPmValue());
    measurements.update(Measurements::PM10_SP, utils::getInvalidPmValue());
    measurements.update(Measurements::PM03_PC, utils::getInvalidPmValue());
    measurements.update(Measurements::PM05_PC, utils::getInvalidPmValue());
    measurements.update(Measurements::PM01_PC, utils::getInvalidPmValue());
    measurements.update(Measurements::PM25_PC, utils::getInvalidPmValue());
    measurements.update(Measurements::PM5_PC, utils::getInvalidPmValue());
    measurements.update(Measurements::PM10_PC, utils::getInvalidPmValue());
  }
}

static void updateSPS30(SPS30 &sensor, int channel) {
  if (sensor.readValues()) {
    // Mass concentrations — mapped to both Ae and SP (SPS30 has no distinction)
    measurements.update(Measurements::PM01, sensor.getPm01Ae(), channel);
    measurements.update(Measurements::PM25, sensor.getPm25Ae(), channel);
    measurements.update(Measurements::PM10, sensor.getPm10Ae(), channel);
    measurements.update(Measurements::PM01_SP, sensor.getPm01Sp(), channel);
    measurements.update(Measurements::PM25_SP, sensor.getPm25Sp(), channel);
    measurements.update(Measurements::PM10_SP, sensor.getPm10Sp(), channel);

    // Number concentrations (already converted to #/0.1L by wrapper)
    measurements.update(Measurements::PM05_PC, sensor.getPm05ParticleCount(), channel);
    measurements.update(Measurements::PM01_PC, sensor.getPm01ParticleCount(), channel);
    measurements.update(Measurements::PM25_PC, sensor.getPm25ParticleCount(), channel);
    measurements.update(Measurements::PM10_PC, sensor.getPm10ParticleCount(), channel);
  } else {
    measurements.update(Measurements::PM01, utils::getInvalidPmValue(), channel);
    measurements.update(Measurements::PM25, utils::getInvalidPmValue(), channel);
    measurements.update(Measurements::PM10, utils::getInvalidPmValue(), channel);
    measurements.update(Measurements::PM01_SP, utils::getInvalidPmValue(), channel);
    measurements.update(Measurements::PM25_SP, utils::getInvalidPmValue(), channel);
    measurements.update(Measurements::PM10_SP, utils::getInvalidPmValue(), channel);
    measurements.update(Measurements::PM01_PC, utils::getInvalidPmValue(), channel);
    measurements.update(Measurements::PM25_PC, utils::getInvalidPmValue(), channel);
    measurements.update(Measurements::PM5_PC, utils::getInvalidPmValue(), channel);
    measurements.update(Measurements::PM10_PC, utils::getInvalidPmValue(), channel);
  }
}

static void updatePm(void) {
  if (ag->isOne()) {
    if (configuration.hasSensorSPS30_1) {
      updateSPS30(ag->sps30_1, 1);
    } else {
      updatePMS5003();
    }
    return;
  }

  // Open Air Monitor series — each channel can be PMS5003T or SPS30.
  // Track which channels produced valid PMS5003T T/RH for SGP41 compensation.
  bool newPmsTempHumCh1 = false;
  bool newPmsTempHumCh2 = false;

  // ---- Channel 1 ----
  if (configuration.hasSensorPMS1) {
    int channel = 1;
    if (ag->pms5003t_1.connected()) {
      measurements.update(Measurements::PM01, ag->pms5003t_1.getPm01Ae(), channel);
      measurements.update(Measurements::PM25, ag->pms5003t_1.getPm25Ae(), channel);
      measurements.update(Measurements::PM10, ag->pms5003t_1.getPm10Ae(), channel);
      measurements.update(Measurements::PM01_SP, ag->pms5003t_1.getPm01Sp(), channel);
      measurements.update(Measurements::PM25_SP, ag->pms5003t_1.getPm25Sp(), channel);
      measurements.update(Measurements::PM10_SP, ag->pms5003t_1.getPm10Sp(), channel);
      measurements.update(Measurements::PM03_PC, ag->pms5003t_1.getPm03ParticleCount(), channel);
      measurements.update(Measurements::PM05_PC, ag->pms5003t_1.getPm05ParticleCount(), channel);
      measurements.update(Measurements::PM01_PC, ag->pms5003t_1.getPm01ParticleCount(), channel);
      measurements.update(Measurements::PM25_PC, ag->pms5003t_1.getPm25ParticleCount(), channel);
      measurements.update(Measurements::Temperature, ag->pms5003t_1.getTemperature(), channel);
      measurements.update(Measurements::Humidity, ag->pms5003t_1.getRelativeHumidity(), channel);
      newPmsTempHumCh1 = true;
    } else {
      measurements.update(Measurements::PM01, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM25, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM10, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM01_SP, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM25_SP, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM10_SP, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM03_PC, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM05_PC, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM01_PC, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM25_PC, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::Temperature, utils::getInvalidTemperature(), channel);
      measurements.update(Measurements::Humidity, utils::getInvalidHumidity(), channel);
    }
  } else if (configuration.hasSensorSPS30_1) {
    updateSPS30(ag->sps30_1, 1);
  }

  // ---- Channel 2 ----
  if (configuration.hasSensorPMS2) {
    int channel = 2;
    if (ag->pms5003t_2.connected()) {
      measurements.update(Measurements::PM01, ag->pms5003t_2.getPm01Ae(), channel);
      measurements.update(Measurements::PM25, ag->pms5003t_2.getPm25Ae(), channel);
      measurements.update(Measurements::PM10, ag->pms5003t_2.getPm10Ae(), channel);
      measurements.update(Measurements::PM01_SP, ag->pms5003t_2.getPm01Sp(), channel);
      measurements.update(Measurements::PM25_SP, ag->pms5003t_2.getPm25Sp(), channel);
      measurements.update(Measurements::PM10_SP, ag->pms5003t_2.getPm10Sp(), channel);
      measurements.update(Measurements::PM03_PC, ag->pms5003t_2.getPm03ParticleCount(), channel);
      measurements.update(Measurements::PM05_PC, ag->pms5003t_2.getPm05ParticleCount(), channel);
      measurements.update(Measurements::PM01_PC, ag->pms5003t_2.getPm01ParticleCount(), channel);
      measurements.update(Measurements::PM25_PC, ag->pms5003t_2.getPm25ParticleCount(), channel);
      measurements.update(Measurements::Temperature, ag->pms5003t_2.getTemperature(), channel);
      measurements.update(Measurements::Humidity, ag->pms5003t_2.getRelativeHumidity(), channel);
      newPmsTempHumCh2 = true;
    } else {
      measurements.update(Measurements::PM01, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM25, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM10, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM01_SP, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM25_SP, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM10_SP, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM03_PC, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM05_PC, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM01_PC, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::PM25_PC, utils::getInvalidPmValue(), channel);
      measurements.update(Measurements::Temperature, utils::getInvalidTemperature(), channel);
      measurements.update(Measurements::Humidity, utils::getInvalidHumidity(), channel);
    }
  } else if (configuration.hasSensorSPS30_2) {
    updateSPS30(ag->sps30_2, 2);
  }

  // SGP41 compensation — only uses T/RH from PMS5003T channels (SPS30 has no T/RH)
  if (configuration.hasSensorSGP) {
    if (newPmsTempHumCh1 || newPmsTempHumCh2) {
      float temp, hum;
      if (newPmsTempHumCh1 && newPmsTempHumCh2) {
        temp = (measurements.getFloat(Measurements::Temperature, 1) +
                measurements.getFloat(Measurements::Temperature, 2)) /
               2.0f;
        hum = (measurements.getFloat(Measurements::Humidity, 1) +
               measurements.getFloat(Measurements::Humidity, 2)) /
              2.0f;
      } else if (newPmsTempHumCh1) {
        temp = measurements.getFloat(Measurements::Temperature, 1);
        hum = measurements.getFloat(Measurements::Humidity, 1);
      } else {
        temp = measurements.getFloat(Measurements::Temperature, 2);
        hum = measurements.getFloat(Measurements::Humidity, 2);
      }
      ag->sgp41.setCompensationTemperatureHumidity(temp, hum);
    }
    // When no PMS5003T channel provides T/RH (e.g. 2× SPS30), SGP41 keeps
    // its previous compensation values (default 25 °C / 50 %RH on first run).
  }
}

void postUsingWifi() {
  // Increment bootcount when send measurements data is scheduled
  int bootCount = measurements.bootCount() + 1;
  measurements.setBootCount(bootCount);

  String payload = measurements.toString(false, fwMode, wifiConnector.RSSI());
  if (agClient->httpPostMeasures(payload.c_str()) == false) {
    Serial.println();
    Serial.println("Online mode and isPostToAirGradient = true");
    Serial.println();
  }

  // Log current free heap size
  Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
}

/**
 * forcePost to force post without checking transmit cycle
 */
void postUsingCellular(bool forcePost) {
  // Aquire queue mutex; held across the post so newMeasurementCycle cannot
  // mutate the payload mid-encode. newMeasurementCycle uses a bounded take so
  // loop() never stalls past the external watchdog.
  xSemaphoreTake(mutexMeasurementCycleQueue, portMAX_DELAY);

  // Make sure measurement cycle available
  int queueSize = measurementPayload.bufferCount;
  if (queueSize == 0) {
    Serial.println("Skipping transmission, measurementCycle empty");
    xSemaphoreGive(mutexMeasurementCycleQueue);
    return;
  }

  // Ready when size is divisible by 3, or the buffer is full. The full-buffer
  // override breaks a deadlock: the cap pins size at a non-multiple of 3, which
  // the divisibility gate alone would never release.
  bool queueFull = queueSize >= MAXIMUM_MEASUREMENT_CYCLE_QUEUE;
  if (!forcePost && !queueFull && (queueSize % MEASUREMENT_TRANSMIT_CYCLE) > 0) {
    Serial.printf("Not ready to transmit, queue size are %d\n", queueSize);
    xSemaphoreGive(mutexMeasurementCycleQueue);
    return;
  }

  // Buffers and bufferCount already populated by newMeasurementCycle; set header
  measurementPayload.measureInterval = CELLULAR_MEASUREMENT_INTERVAL / 1000; // Convert to seconds
  measurementPayload.payloadType = getClientPayloadType();
  measurementPayload.signal = cellularCard->csqToDbm(lastCellSignalQuality); // latest signal as RSSI

  // Attempt to send over CoAP
  if (agClient->coapPostMeasures(measurementPayload) == false) {
    // Consider network has a problem, retry in next schedule (data retained)
    Serial.println("Post measures failed, retry in next schedule");
    xSemaphoreGive(mutexMeasurementCycleQueue);
    return;
  }

  // Post success, clear the cache
  measurementPayload.bufferCount = 0;
  xSemaphoreGive(mutexMeasurementCycleQueue);
}

void sendDataToServer(void) {
  if (configuration.isPostDataToAirGradient() == false) {
    Serial.println("Skipping transmission of data to AG server, post data to server disabled");
    agClient->resetPostMeasuresStatus();
    return;
  }

  if (networkOption == UseWifi) {
    postUsingWifi();
  } else if (networkOption == UseCellular) {
    postUsingCellular(false);
  }
}

static void tempHumUpdate(void) {
  delay(100);
  if (ag->sht.measure()) {
    float temp = ag->sht.getTemperature();
    float rhum = ag->sht.getRelativeHumidity();

    measurements.update(Measurements::Temperature, temp);
    measurements.update(Measurements::Humidity, rhum);

    // Update compensation temperature and humidity for SGP41
    if (configuration.hasSensorSGP) {
      ag->sgp41.setCompensationTemperatureHumidity(temp, rhum);
    }
  } else {
    measurements.update(Measurements::Temperature, utils::getInvalidTemperature());
    measurements.update(Measurements::Humidity, utils::getInvalidHumidity());
    Serial.println("SHT read failed");
  }
}

/* Set max period for each measurement type based on sensor update interval*/
void setMeasurementMaxPeriod() {
  int max;

  /// Max period for S8 sensors measurements
  measurements.maxPeriod(Measurements::CO2, calculateMaxPeriod(SENSOR_CO2_UPDATE_INTERVAL));

  /// Max period for SGP sensors measurements
  max = calculateMaxPeriod(SENSOR_TVOC_UPDATE_INTERVAL);
  measurements.maxPeriod(Measurements::TVOC, max);
  measurements.maxPeriod(Measurements::TVOCRaw, max);
  measurements.maxPeriod(Measurements::NOx, max);
  measurements.maxPeriod(Measurements::NOxRaw, max);

  /// Max period for PMS sensors measurements
  max = calculateMaxPeriod(SENSOR_PM_UPDATE_INTERVAL);
  measurements.maxPeriod(Measurements::PM25, max);
  measurements.maxPeriod(Measurements::PM01, max);
  measurements.maxPeriod(Measurements::PM10, max);
  measurements.maxPeriod(Measurements::PM25_SP, max);
  measurements.maxPeriod(Measurements::PM01_SP, max);
  measurements.maxPeriod(Measurements::PM10_SP, max);
  measurements.maxPeriod(Measurements::PM03_PC, max);
  measurements.maxPeriod(Measurements::PM05_PC, max);
  measurements.maxPeriod(Measurements::PM01_PC, max);
  measurements.maxPeriod(Measurements::PM25_PC, max);
  measurements.maxPeriod(Measurements::PM5_PC, max);
  measurements.maxPeriod(Measurements::PM10_PC, max);

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
  // 0.8 is 80% reduced interval for max period
  // NOTE: Both network option use the same measurement interval
  return (WIFI_MEASUREMENT_INTERVAL - (WIFI_MEASUREMENT_INTERVAL * 0.8)) / updateInterval;
}

void networkSignalCheck() {
  if (networkOption == UseWifi) {
    Serial.printf("WiFi RSSI %d\n", wifiConnector.RSSI());
  } else if (networkOption == UseCellular) {
    auto result = cellularCard->retrieveSignal();
    if (result.status != CellReturnStatus::Ok) {
      agClient->setClientReady(false);
      lastCellSignalQuality = 99;
      return;
    }

    // Save last signal quality
    lastCellSignalQuality = result.data;

    if (result.data == 99) {
      // 99 indicate cellular not attached to network
      agClient->setClientReady(false);
      return;
    }

    Serial.printf("Cellular signal quality %d\n", result.data);
  }
}

/**
 * If in 2 hours cellular client still not ready, then restart system
 */
void restartIfCeClientIssueOverTwoHours() {
  if (agCeClientProblemDetectedTime > 0 &&
      (MINUTES() - agCeClientProblemDetectedTime) > TIMEOUT_WAIT_FOR_CELLULAR_MODULE_READY) {
    // Give up wait
    Serial.println("Rebooting because CE client issues for 2 hours detected");
    int i = 3;
    while (i != 0) {
      if (ag->isOne()) {
        String tmp = "Rebooting in " + String(i);
        oledDisplay.setText("CE error", "since 2h", tmp.c_str());
      } else {
        Serial.println("Rebooting... " + String(i));
      }
      i = i - 1;
      delay(1000);
    }
    oledDisplay.setBrightness(0);
    esp_restart();
  }
}

void networkingTask(void *args) {
  // If cloud connection enabled, run first transmission to server at boot
  if (configuration.isCloudConnectionDisabled() == false) {
    // OTA check on boot
#ifndef ESP8266
    checkForFirmwareUpdate();
    checkForUpdateSchedule.update();
#endif

    // Because cellular interval is longer, needs to send first measures cycle on
    // boot to indicate that its online
    if (networkOption == UseCellular) {
      Serial.println("Prepare first measures cycle to send on boot for 20s");
      delay(20000);
      networkSignalCheck();
      newMeasurementCycle();
      postUsingCellular(true);
      measurementSchedule.update();
    }
    // Reset scheduler
    configSchedule.update();
    transmissionSchedule.update();
  }

  while (1) {
    // Handle reconnection based on mode
    if (networkOption == UseWifi) {
      wifiConnector.handle();
      if (wifiConnector.isConnected() == false) {
        delay(1000);
        continue;
      }
    } else if (networkOption == UseCellular) {
      if (agClient->isClientReady() == false) {
        // Start time if value still default
        if (agCeClientProblemDetectedTime == 0) {
          agCeClientProblemDetectedTime = MINUTES();
        }

        // Enable at command debug
        agSerial->setDebug(true);

        // Check if cellular client not ready until certain time
        // Redundant check in both task to make sure its executed
        restartIfCeClientIssueOverTwoHours();

        // Power cycling cellular module due to network issues for more than 1 hour
        bool resetModule = true;
        if ((MINUTES() - agCeClientProblemDetectedTime) >
            TIME_TO_START_POWER_CYCLE_CELLULAR_MODULE) {
          Serial.println("The CE client hasn't recovered in more than 1 hour, "
                         "performing a power cycle");
          cellularCard->powerOff();
          delay(2000);
          cellularCard->powerOn();
          delay(10000);
          // no need to reset module when calling ensureClientConnection()
          resetModule = false;
        }

        // Attempt to reconnect
        Serial.println("Cellular client not ready, ensuring connection...");
        if (agClient->ensureClientConnection(resetModule) == false) {
          Serial.println("Cellular client connection not ready, retry in 30s...");
          delay(30000); // before retry, wait for 30s
          continue;
        }

        // Client is ready
        saveOperatorState();
        agCeClientProblemDetectedTime = 0; // reset to default
        agSerial->setDebug(false);         // disable at command debug
      }
    }

    // If connection to AirGradient server disable don't run config and transmission schedule
    if (configuration.isCloudConnectionDisabled()) {
      delay(1000);
      continue;
    }

    // Run scheduler
    networkSignalCheckSchedule.run();
    transmissionSchedule.run();
    configSchedule.run();
    checkForUpdateSchedule.run();

    delay(50);
  }

  vTaskDelete(handleNetworkTask);
}

void newMeasurementCycle() {
  // Bounded wait: if a post holds the lock, wait up to MEASUREMENT_LOCK_TIMEOUT.
  // Caps loop() stall under the external watchdog; on timeout, skip this cycle.
  if (xSemaphoreTake(mutexMeasurementCycleQueue, pdMS_TO_TICKS(MEASUREMENT_LOCK_TIMEOUT)) != pdTRUE) {
    Serial.println("Skip measurement cycle, transmission holding the lock");
    return;
  }

  // Make sure buffer not overflow; drop the oldest entry if at capacity
  if (measurementPayload.bufferCount >= MAXIMUM_MEASUREMENT_CYCLE_QUEUE) {
    memmove(&measurementPayload.payloadBuffer[0], &measurementPayload.payloadBuffer[1],
            (MAXIMUM_MEASUREMENT_CYCLE_QUEUE - 1) * sizeof(AirgradientClient::PayloadBuffer));
    measurementPayload.bufferCount = MAXIMUM_MEASUREMENT_CYCLE_QUEUE - 1;
  }

  // Get current measures and cache as client payload buffer
  auto mc = measurements.getMeasures();
  measurementPayload.payloadBuffer[measurementPayload.bufferCount].common = buildCommonPayload(mc);
  measurementPayload.bufferCount++;
  Serial.println("New measurement cycle added to queue");

  xSemaphoreGive(mutexMeasurementCycleQueue);
  // Log current free heap size
  Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
}
