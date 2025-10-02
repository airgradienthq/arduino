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
#include "AgConfigure.h"
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

#define LED_BAR_ANIMATION_PERIOD 100                   /** ms */
#define DISP_UPDATE_INTERVAL 2500                      /** ms */
#define WIFI_SERVER_CONFIG_SYNC_INTERVAL 1 * 60000     /** ms */
#define WIFI_MEASUREMENT_INTERVAL 1 * 60000            /** ms */
#define WIFI_TRANSMISSION_INTERVAL 1 * 60000           /** ms */
#define CELLULAR_SERVER_CONFIG_SYNC_INTERVAL 30 * 60000 /** ms */
#define CELLULAR_MEASUREMENT_INTERVAL 3 * 60000        /** ms */
#define CELLULAR_TRANSMISSION_INTERVAL 3 * 60000       /** ms */
#define MQTT_SYNC_INTERVAL 60000                       /** ms */
#define SENSOR_CO2_CALIB_COUNTDOWN_MAX 5               /** sec */
#define SENSOR_TVOC_UPDATE_INTERVAL 1000               /** ms */
#define SENSOR_CO2_UPDATE_INTERVAL 4000                /** ms */
#define SENSOR_PM_UPDATE_INTERVAL 2000                 /** ms */
#define SENSOR_TEMP_HUM_UPDATE_INTERVAL 6000           /** ms */
#define DISPLAY_DELAY_SHOW_CONTENT_MS 2000             /** ms */
#define FIRMWARE_CHECK_FOR_UPDATE_MS (60 * 60 * 1000)  /** ms */
#define TIME_TO_START_POWER_CYCLE_CELLULAR_MODULE (1 * 60) /** minutes */
#define TIMEOUT_WAIT_FOR_CELLULAR_MODULE_READY    (2 * 60) /** minutes */

#define MEASUREMENT_TRANSMIT_CYCLE 3
#define MAXIMUM_MEASUREMENT_CYCLE_QUEUE 80
#define RESERVED_MEASUREMENT_CYCLE_CAPACITY 10

/** I2C define */
#define I2C_SDA_PIN 7
#define I2C_SCL_PIN 6
#define OLED_I2C_ADDR 0x3C

/** Power pin */
#define GPIO_POWER_MODULE_PIN  5
#define GPIO_EXPANSION_CARD_POWER 4
#define GPIO_IIC_RESET 3

#define MINUTES() ((uint32_t)(esp_timer_get_time() / 1000 / 1000 / 60))

static MqttClient mqttClient(Serial);
static TaskHandle_t mqttTask = NULL;
static Configuration configuration(Serial);
static Measurements measurements(configuration);
static AirGradient *ag;
static OledDisplay oledDisplay(configuration, measurements, Serial);
static StateMachine stateMachine(oledDisplay, Serial, measurements,
                                 configuration);
static WifiConnector wifiConnector(oledDisplay, Serial, stateMachine,
                                   configuration);
static OpenMetrics openMetrics(measurements, configuration, wifiConnector);
static LocalServer localServer(Serial, openMetrics, measurements, configuration,
                               wifiConnector);
static AgSerial *agSerial;
static CellularModule *cellularCard;
static AirgradientClient *agClient;

enum NetworkOption {
  UseWifi,
  UseCellular
};
NetworkOption networkOption;
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
static std::vector<Measurements::Measures> measurementCycleQueue;

static void boardInit(void);
static void initializeNetwork();
static void failedHandler(String msg);
static void configurationUpdateSchedule(void);
static void configUpdateHandle(void);
static void updateDisplayAndLedBar(void);
static void updateTvoc(void);
static void updatePm(void);
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

AgSchedule dispLedSchedule(DISP_UPDATE_INTERVAL, updateDisplayAndLedBar);
AgSchedule configSchedule(WIFI_SERVER_CONFIG_SYNC_INTERVAL,
                          configurationUpdateSchedule);
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

  // Enable cullular module power board
  pinMode(GPIO_EXPANSION_CARD_POWER, OUTPUT);
  digitalWrite(GPIO_EXPANSION_CARD_POWER, HIGH);

  /** Print device ID into log */
  Serial.println("Serial nr: " + ag->deviceId());

  // Set reason why esp is reset
  esp_reset_reason_t reason = esp_reset_reason();
  measurements.setResetReason(reason);

  /** Initialize local configure */
  configuration.begin();
  configuration.setConfigurationUpdatedCallback(configUpdateHandle);

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
  openMetrics.setAirGradient(ag);
  localServer.setAirGraident(ag);
  measurements.setAirGradient(ag);

  /** Init sensor */
  boardInit();
  setMeasurementMaxPeriod();

  bool connectToNetwork = true;
  if (ag->isOne()) { // Offline mode only available for indoor monitor
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
  }

  /** Set offline mode without saving, cause wifi is not configured */
  if (wifiConnector.hasConfigurated() == false && networkOption == UseWifi) {
    Serial.println("Set offline mode cause wifi is not configurated");
    configuration.setOfflineModeWithoutSave(true);
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
    // Allocate queue memory to avoid always reallocation
    measurementCycleQueue.reserve(RESERVED_MEASUREMENT_CYCLE_CAPACITY);
    // Initialize mutex to access mesurementCycleQueue
    mutexMeasurementCycleQueue = xSemaphoreCreateMutex();
  }

  // Only run network task if monitor is not in offline mode
  if (configuration.isOfflineMode() == false) {
    BaseType_t xReturned =
      xTaskCreate(networkingTask, "NetworkingTask", 4096, null, 5, &handleNetworkTask);
    if (xReturned == pdPASS) {
      Serial.println("Success create networking task");
    } else {
      assert("Failed to create networking task");
    }
  }

  // Log monitor mode for debugging purpose
  if (configuration.isOfflineMode()) {
    Serial.println("Running monitor in offline mode");
  }
  else if (configuration.isCloudConnectionDisabled()) {
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

  // Schedule to update display and led
  dispLedSchedule.run();

  if (networkOption == UseCellular) {
    // Queue now only applied for cellular
    measurementSchedule.run();
  }

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
      static bool pmsConnected = false;
      if (pmsConnected != ag->pms5003.connected()) {
        pmsConnected = ag->pms5003.connected();
        Serial.printf("PMS sensor %s \n", pmsConnected?"connected":"removed");
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

void printMeasurements() {
  measurements.printCurrentAverage();
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
            String payload = measurements.toString(true, fwMode, wifiConnector.RSSI());
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
            oledDisplay.setText("","","");
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
      oledDisplay.setAirGradient(0);
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
  } else {
    if (ag->isOne()) {
      stateMachine.displayHandle(AgStateMachineWiFiOkServerConnectFailed);
    }
    stateMachine.handleLeds(AgStateMachineWiFiOkServerConnectFailed);
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

void initializeNetwork() {
  // Check if cellular module available
  agSerial = new AgSerial(Wire);
  agSerial->init(GPIO_IIC_RESET);
  if (agSerial->open()) {
    Serial.println("Cellular module found");
    // Initialize cellular module and use cellular as agClient
    cellularCard = new CellularModuleA7672XX(agSerial, GPIO_POWER_MODULE_PIN);
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

  if (!agClient->begin(ag->deviceId().c_str())) {
    oledDisplay.setText("Client", "initialization", "failed");
    delay(5000);
    oledDisplay.showRebooting();
    delay(2500);
    oledDisplay.setText("", "", "");
    ESP.restart();
  }

  // Provide openmetrics to have access to last transmission result
  openMetrics.setAirgradientClient(agClient);

  if (networkOption == UseCellular) {
    // Disabling it again
    agSerial->setDebug(false);
  }

  if (networkOption == UseWifi) {
    if (!wifiConnector.connect()) {
      Serial.println("Cannot initiate wifi connection");
      return;
    }

    if (!wifiConnector.isConnected()) {
      Serial.println("Failed connect to WiFi");
      if (wifiConnector.isConfigurePorttalTimeout()) {
        oledDisplay.showRebooting();
        delay(2500);
        oledDisplay.setText("", "", "");
        ESP.restart();
      }

      // Directly return because the rest of the function applied if wifi is connect only
      return;
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

  std::string config = agClient->httpFetchConfig();
  configSchedule.update();
  // Check if fetch configuration failed or fetch succes but parsing failed
  if (agClient->isLastFetchConfigSucceed() == false ||
      configuration.parse(config.c_str(), false) == false) {
    if (ag->isOne()) {
      if (agClient->isRegisteredOnAgServer() == false) {
        stateMachine.displaySetAddToDashBoard();
        stateMachine.displayHandle(AgStateMachineWiFiOkServerOkSensorConfigFailed);
      } else {
        stateMachine.displayClearAddToDashBoard();
      }
    }
    stateMachine.handleLeds(AgStateMachineWiFiOkServerOkSensorConfigFailed);
    delay(DISPLAY_DELAY_SHOW_CONTENT_MS);
  }
  else {
    ledBarEnabledUpdate();
  }
}

static void configurationUpdateSchedule(void) {
  if (configuration.getConfigurationControl() ==
      ConfigurationControl::ConfigurationControlLocal) {
    Serial.println("Ignore fetch server configuration, configurationControl set to local");
    agClient->resetFetchConfigurationStatus();
    return;
  }

  std::string config = agClient->httpFetchConfig();
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
  }

  // Update display and led bar notification based on updated configuration
  updateDisplayAndLedBar();
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
  }
  else if (networkOption == UseCellular) {
    if (agClient->isClientReady() == false)  {
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

static void updatePm(void) {
  if (ag->isOne()) {
    updatePMS5003();
    return;
  }

  // Open Air Monitor series, can have two PMS5003T sensor
  bool newPMS1Value = false;
  bool newPMS2Value = false;

  // Read PMS channel 1 if available
  int channel = 1;
  if (configuration.hasSensorPMS1) {
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

      // flag that new valid PMS value exists
      newPMS1Value = true;
    } else {
      // PMS channel 1 now is not connected, update using invalid value
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
  }

  // Read PMS channel 2 if available
  channel = 2;
  if (configuration.hasSensorPMS2) {
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

      // flag that new valid PMS value exists
      newPMS2Value = true;
    } else {
      // PMS channel 2 now is not connected, update using invalid value
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
  }

  if (configuration.hasSensorSGP) {
    float temp, hum;
    if (newPMS1Value && newPMS2Value) {
      // Both PMS has new valid value
      temp = (measurements.getFloat(Measurements::Temperature, 1) +
              measurements.getFloat(Measurements::Temperature, 2)) /
             2.0f;
      hum = (measurements.getFloat(Measurements::Humidity, 1) +
             measurements.getFloat(Measurements::Humidity, 2)) /
            2.0f;
    } else if (newPMS1Value) {
      // Only PMS1 has new valid value
      temp = measurements.getFloat(Measurements::Temperature, 1);
      hum = measurements.getFloat(Measurements::Humidity, 1);
    } else {
      // Only PMS2 has new valid value
      temp = measurements.getFloat(Measurements::Temperature, 2);
      hum = measurements.getFloat(Measurements::Humidity, 2);
    }

    // Update compensation temperature and humidity for SGP41
    ag->sgp41.setCompensationTemperatureHumidity(temp, hum);
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
  // Aquire queue mutex to get queue size
  xSemaphoreTake(mutexMeasurementCycleQueue, portMAX_DELAY);

  // Make sure measurement cycle available
  int queueSize = measurementCycleQueue.size();
  if (queueSize == 0) {
    Serial.println("Skipping transmission, measurementCycle empty");
    xSemaphoreGive(mutexMeasurementCycleQueue);
    return;
  }

  // Check queue size if its ready to transmit
  // It is ready if size is divisible by 3
  if (!forcePost && (queueSize % MEASUREMENT_TRANSMIT_CYCLE) > 0) {
    Serial.printf("Not ready to transmit, queue size are %d\n", queueSize);
    xSemaphoreGive(mutexMeasurementCycleQueue);
    return;
  }

  // Build payload include all measurements from queue
  std::string payload;
  payload += std::to_string(CELLULAR_MEASUREMENT_INTERVAL / 1000); // Convert to seconds
  for (int i = 0; i < queueSize; i++) {
    auto mc = measurementCycleQueue.at(i);
    payload += ",";
    payload += measurements.buildMeasuresPayload(mc);
  }

  // Release before actually post measures that might takes too long
  xSemaphoreGive(mutexMeasurementCycleQueue);

  // Attempt to send
  if (agClient->httpPostMeasures(payload) == false) {
    // Consider network has a problem, retry in next schedule
    Serial.println("Post measures failed, retry in next schedule");
    return;
  }

  // Post success, remove the data that previously sent from queue
  xSemaphoreTake(mutexMeasurementCycleQueue, portMAX_DELAY);

  if (measurementCycleQueue.capacity() > RESERVED_MEASUREMENT_CYCLE_CAPACITY) {
    Serial.println("measurementCycleQueue capacity more than reserved space, resizing..");
    std::vector<Measurements::Measures> tmp;
    tmp.reserve(RESERVED_MEASUREMENT_CYCLE_CAPACITY);
    measurementCycleQueue.swap(tmp);
  } else {
    // If not more than the capacity, then just clear all the values
    measurementCycleQueue.clear();
  }

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
      (MINUTES() - agCeClientProblemDetectedTime) >
          TIMEOUT_WAIT_FOR_CELLULAR_MODULE_READY) {
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
    }
    else if (networkOption == UseCellular) {
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
        agCeClientProblemDetectedTime = 0; // reset to default
        agSerial->setDebug(false); // disable at command debug
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
  if (xSemaphoreTake(mutexMeasurementCycleQueue, portMAX_DELAY) == pdTRUE) {
    // Make sure queue not overflow
    if (measurementCycleQueue.size() >= MAXIMUM_MEASUREMENT_CYCLE_QUEUE) {
      // Remove the oldest data from queue if queue reach max
      measurementCycleQueue.erase(measurementCycleQueue.begin());
    }

    // Get current measures
    auto mc = measurements.getMeasures();
    mc.signal = cellularCard->csqToDbm(lastCellSignalQuality); // convert to RSSI

    measurementCycleQueue.push_back(mc);
    Serial.println("New measurement cycle added to queue");
    // Release mutex
    xSemaphoreGive(mutexMeasurementCycleQueue);
    // Log current free heap size
    Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
  }
}

