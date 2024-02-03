#include <AirGradient.h>
#include <HardwareSerial.h>
#include <Wire.h>

/**
 * AirGradient use ESP32C3 has default Serial0 use for PMS5003, to print log
 * should use esp-hal-log instead.
 */
#include <esp32-hal-log.h>

/**
 * @brief Define test board
 */
#define TEST_BOARD_OUTDOOR_MONITOR_V1_3 0
#define TEST_BOARD_ONE_INDOOR_MONITOR_V9_0 1
/**
 * @brief Define test sensor
 */
#define TEST_SENSOR_SenseAirS8 0
#define TEST_SENSOR_SHT4x 0
#define TEST_SENSOR_SGP4x 0
#define TEST_SWITCH 0
#define TEST_OLED 0
#if TEST_BOARD_OUTDOOR_MONITOR_V1_3
#define TEST_STATUS_LED 0
#define TEST_PMS5003T 1
#endif
#define TEST_WATCHDOG 1

#if TEST_BOARD_ONE_INDOOR_MONITOR_V9_0
#define TEST_LED_BAR 1
#define TEST_SENSOR_PMS5003 0
#endif

#if TEST_BOARD_OUTDOOR_MONITOR_V1_3
AirGradient ag(BOARD_OUTDOOR_MONITOR_V1_3);
#elif TEST_BOARD_ONE_INDOOR_MONITOR_V9_0
AirGradient ag(BOARD_ONE_INDOOR_MONITOR_V9_0);
#else
#error "Must enable board test
#endif

void setup() {

  /** Print All AirGradient board define */
  printBoardDef(NULL);

#if TEST_SENSOR_SenseAirS8
  /** Cause Serial is use default for PMS, CO2S8 should be use Serial 1
   * Serial 1 will be init by SenseAirS8 don't need to init any more on user
   * code
   */
  if (ag.s8.begin(Serial1)) {
    log_i("CO2S8 sensor init success");
  } else {
    log_i("CO2S8 sensor init failure");
  }

  log_i("Start baseline calib");
  if (ag.s8.setBaselineCalibration()) {
    log_i("Calib success");
  } else {
    log_e("Calib failure");
  }
  delay(5000); // Wait for calib done
#endif

#if TEST_SENSOR_PMS5003
  if (ag.pms5003.begin(Serial0)) {
    log_i("PMS5003 sensor init success");
  } else {
    log_i("PMS5003 sensor init failure");
  }
#endif

#if TEST_PMS5003T
  /**
   * @brief PMS5003T_1 alway connect to Serial (esp32c3 RXD0, RXD0)
   */
  if (ag.pms5003t_1.begin(Serial)) {
    log_i("PMS5003T_1 sensor init success");
  } else {
    log_i("PMS5003T_1 sensor init failure");
  }

  // TODO Only test without senseair s8 because it's share the UART bus
#if TEST_SENSOR_SenseAirS8 == 0
  if (ag.pms5003t_2.begin(Serial1)) {
    log_i("PMS5003T_2 sensor init success");
  } else {
    log_i("PMS5003T_2 sensor init failure");
  }
#endif
#endif

#if TEST_SENSOR_SHT4x || TEST_SENSOR_SGP4x || TEST_OLED
  Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin());
#endif

#if TEST_SENSOR_SHT4x

  if (ag.sht.begin(Wire)) {
    log_i("SHT init success");
  } else {
    log_i("SHT init failed");
  }
#endif

#if TEST_SENSOR_SGP4x
  if (ag.sgp41.begin(Wire)) {
    log_i("SGP init success");
  } else {
    log_e("SGP init failure");
  }
#endif

#if TEST_LED
  led.begin();
#endif

#if TEST_SWITCH
  ag.button.begin();
#endif

#if TEST_OLED
  ag.display.begin(Wire);
  ag.display.setTextSize(1);
  ag.display.setCursor(0, 0);
  ag.display.setTextColor(1);
  ag.display.setText("180s to connect to wifi hostpost AC-xxxxx");
  ag.display.show();
#endif

#if TEST_STATUS_LED
  ag.statusLed.begin();
#endif

#if TEST_LED_BAR
  ag.ledBar.begin();
#endif

#if TEST_WATCHDOG
  ag.watchdog.begin();
#endif
}

void loop() {
  uint32_t ms;
#if TEST_SENSOR_SenseAirS8
  static uint32_t lastTime = 0;

  /** Wait for sensor ready */
  ms = (uint32_t)(millis() - lastTime);
  if (ms >= 1000) {
    lastTime = millis();
    log_i("CO2: %d (PPM)", ag.s8.getCo2());
  }
#endif

#if TEST_SENSOR_PMS5003
  static uint32_t pmsTime = 0;
  ms = (uint32_t)(millis() - pmsTime);

  if (ms >= 1000) {
    pmsTime = millis();
    if (ag.pms5003.readData()) {
      log_i("Passive mode PM 1.0  (ug/m3): %d", ag.pms5003.getPm10Ae());
      log_i("Passive mode PM 2.5  (ug/m3): %d", ag.pms5003.getPm25Ae());
      log_i("Passive mode PM 10.0 (ug/m3): %d", ag.pms5003.getPm10Ae());
    } else {
      log_i("PMS sensor read failure");
    }
  }
#endif

#if TEST_PMS5003T
  static uint32_t pmsTime = 0;
  ms = (uint32_t)(millis() - pmsTime);

  if (ms >= 1000) {
    pmsTime = millis();
    if (ag.pms5003t_1.readData()) {
      log_i("PMS5003_1 PM 1.0  (ug/m3): %d", ag.pms5003t_1.getPm10Ae());
      log_i("PMS5003_1 PM 2.5  (ug/m3): %d", ag.pms5003t_1.getPm25Ae());
      log_i("PMS5003_1 PM 10.0 (ug/m3): %d", ag.pms5003t_1.getPm10Ae());
      log_i("PMS5003_1 PM 3.0  (ug/m3): %d",
            ag.pms5003t_1.getPm03ParticleCount());
      log_i("Temperature                 : %02f °C",
            ag.pms5003t_1.getTemperature());
      log_i("Humidity                    : %02f %%",
            ag.pms5003t_1.getRelativeHumidity());
    } else {
      log_i("PMS5003_1 sensor read failure");
    }

    if (ag.pms5003t_2.readData()) {
      log_i("PMS5003_2 PM 1.0  (ug/m3): %d", ag.pms5003t_2.getPm10Ae());
      log_i("PMS5003_2 PM 2.5  (ug/m3): %d", ag.pms5003t_2.getPm25Ae());
      log_i("PMS5003_2 PM 10.0 (ug/m3): %d", ag.pms5003t_2.getPm10Ae());
      log_i("PMS5003_2 PM 3.0  (ug/m3): %d",
            ag.pms5003t_2.getPm03ParticleCount());
      // log_i("Temperature                 : %02f °C",
      //       ag.pms5003t_1.getTemperature());
      // log_i("Humidity                    : %02f %%",
      //       ag.pms5003t_1.getRelativeHumidity());
    } else {
      log_i("PMS5003_2 sensor read failure");
    }
  }
#endif

#if TEST_SENSOR_SHT4x
  /**
   * @brief Get SHT sensor data each 1sec
   *
   */
  static uint32_t shtTime = 0;
  ms = (uint32_t)(millis() - shtTime);
  if (ms >= 1000) {
    shtTime = millis();
    log_i("Get sht temperature: %0.2f (degree celsius)",
          ag.sht.getTemperature());
    log_i("Get sht temperature: %0.2f (%%)", ag.sht.getRelativeHumidity());
  }
#endif

#if TEST_SENSOR_SGP4x
  static uint32_t sgpTime;
  ms = (uint32_t)(millis() - sgpTime);
  if (ms >= 1000) {
    sgpTime = millis();
    uint16_t rawVOC;
    log_i("Get TVOC: %d", ag.sgp41.getTvocIndex());
    log_i("Get NOx: %d", ag.sgp41.getNoxIndex());
  }
#endif

#if TEST_LED
  static uint32_t ledTime;
#if TEST_BOARD_OUTDOOR_MONITOR_V1_3
  // ms = (uint32_t)(millis() - ledTime);
  // if(ms >= 500)
  // {
  //   ledTime = millis();
  //   led.ledToggle();
  // }
#elif TEST_BOARD_ONE_INDOOR_MONITOR_V9_0

  static int ledIndex;
  static int ledIndexOld;
  ms = (uint32_t)(millis() - ledTime);
  if (ms >= 50) {
    ledTime = millis();
    if (ledIndex == ledIndexOld) {
      led.ledOff();
    } else {
      // Turn last LED off
      led.ledSetColor(0, 0, 0, ledIndexOld);
    }

    // Turn new led ON
    led.ledSetColor(255, 0, 0, ledIndex);
    ledIndexOld = ledIndex;
    ledIndex++;
    if (ledIndex >= led.getNumberOfLed()) {
      ledIndex = 0;
    }
  }
#else
#endif

#endif

#if TEST_SWITCH
  static PushButton::State stateOld = PushButton::State::BUTTON_RELEASED;
  PushButton::State state = ag.button.getState();
  if (state != stateOld) {
    stateOld = state;
    log_i("Button state changed: %s", ag.button.toString(state).c_str());
    if (state == PushButton::State::BUTTON_PRESSED) {
      ag.statusLed.setOn();
    } else {
      ag.statusLed.setOff();
    }
  }
#endif

#if TEST_LED_BAR
  static uint32_t ledTime;
  static uint8_t ledNum = 0;
  static uint8_t ledIndex = 0;
  static uint8_t ledStep = 0;
  static bool ledOn = false;

  if (ledNum == 0) {
    ledNum = ag.ledBar.getNumberOfLed();
    log_i("Get number of led: %d", ledNum);
    if (ledNum) {
      ag.ledBar.setBrighness(0xff);
      for (int i = 0; i < ledNum; i++) {
        // ag.ledBar.setColor(0xff, 0xff, 0xff, i);
        // ag.ledBar.setColor(204, 136, 153, i);
        // ag.ledBar.setColor(204, 0, 0, i);
        // ag.ledBar.setColor(204, 100, 153, i);
        ag.ledBar.setColor(0, 136, 255, i);
      }
      ag.ledBar.show();
    }
  } else {
    ms = (uint32_t)(millis() - ledTime);
    if (ms >= 500) {
      ledTime = millis();
      switch (ledStep) {
      case 0: {
        ag.ledBar.setColor(255, 0, 0, ledIndex);
        ledIndex++;
        if (ledIndex >= ledNum) {
          ag.ledBar.setColor(0, 0, 0);
          ledIndex = 0;
          ledStep = 1;
        }
        ag.ledBar.show();
        break;
      }
      case 1: {
        ledIndex++;
        if (ledIndex >= ledNum) {
          ag.ledBar.setColor(255, 0, 0);
          ag.ledBar.show();
          ledIndex = ledNum - 1;
          ledStep = 2;
        }
        break;
      }
      case 2: {
        if (ledOn) {
          ag.ledBar.setColor(255, 0, 0);
        } else {
          ag.ledBar.setColor(0, 0, 0);
        }
        ledOn = !ledOn;
        ag.ledBar.show();

        ledIndex--;
        if (ledIndex == 0) {
          ag.ledBar.setColor(0, 0, 0);
          ag.ledBar.show();
          ledStep = 0;
          ledIndex = 0;
        }
        break;
      }
      default:
        break;
      }
    }
  }
#endif

#if TEST_WATCHDOG
  static uint32_t wdgTime;
  ms = (uint32_t)(millis() - wdgTime);
  if (ms >= (1000 * 60)) {
    wdgTime = millis();
    /** Reset watchdog reach 1 minutes */
    ag.watchdog.reset();
  }
#endif
}
