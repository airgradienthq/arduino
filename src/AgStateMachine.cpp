#include "AgStateMachine.h"
#include "AgOledDisplay.h"

#define LED_TEST_BLINK_DELAY 50   /** ms */
#define LED_FAST_BLINK_DELAY 250  /** ms */
#define LED_SLOW_BLINK_DELAY 1000 /** ms */
#define LED_SHORT_BLINK_DELAY 500 /** ms */
#define LED_LONG_BLINK_DELAY 2000 /** ms */

#define SENSOR_CO2_CALIB_COUNTDOWN_MAX 5 /** sec */

#define RGB_COLOR_R 255, 0, 0   /** Red */
#define RGB_COLOR_G 0, 255, 0   /** Green */
#define RGB_COLOR_Y 255, 255, 0 /** Yellow */
#define RGB_COLOR_O 255, 128, 0 /** Orange */
#define RGB_COLOR_P 180, 0, 255 /** Purple */
#define RGB_COLOR_CLEAR 0, 0, 0 /** No color */

/**
 * @brief Animation LED bar with color
 *
 * @param r
 * @param g
 * @param b
 */
void StateMachine::ledBarSingleLedAnimation(uint8_t r, uint8_t g, uint8_t b) {
  if (ledBarAnimationCount < 0) {
    ledBarAnimationCount = 0;
    ag->ledBar.setColor(r, g, b, ledBarAnimationCount);
  } else {
    ledBarAnimationCount++;
    if (ledBarAnimationCount >= ag->ledBar.getNumberOfLeds()) {
      ledBarAnimationCount = 0;
    }
    ag->ledBar.setColor(r, g, b, ledBarAnimationCount);
  }
}

/**
 * @brief LED status blink with delay
 *
 * @param ms Miliseconds
 */
void StateMachine::ledStatusBlinkDelay(uint32_t ms) {
  ag->statusLed.setOn();
  delay(ms);
  ag->statusLed.setOff();
  delay(ms);
}

/**
 * @brief Led bar show PM or CO2 led color status
 *
 * @return true if all led bar are used, false othwerwise
 */
bool StateMachine::sensorhandleLeds(void) {
  int totalLedUsed = 0;
  switch (config.getLedBarMode()) {
  case LedBarMode::LedBarModeCO2:
    totalLedUsed = co2handleLeds();
    break;
  case LedBarMode::LedBarModePm:
    totalLedUsed = pm25handleLeds();
    break;
  default:
    ag->ledBar.clear();
    break;
  }

  if (totalLedUsed == ag->ledBar.getNumberOfLeds()) {
    return true;
  }

  // Clear the rest of unused led
  int startIndex = totalLedUsed + 1;
  for (int i = startIndex; i <= ag->ledBar.getNumberOfLeds(); i++) {
    ag->ledBar.setColor(RGB_COLOR_CLEAR, ag->ledBar.getNumberOfLeds() - i);
  }

  return false;
}

/**
 * @brief Show CO2 LED status
 *
 * @return return total number of led that are used on the monitor
 */
int StateMachine::co2handleLeds(void) {
  int totalUsed = ag->ledBar.getNumberOfLeds();
  int co2Value = round(value.getAverage(Measurements::CO2));
  if (co2Value <= 600) {
    /** G; 1 */
    ag->ledBar.setColor(RGB_COLOR_G, ag->ledBar.getNumberOfLeds() - 1);
    totalUsed = 1;
  } else if (co2Value <= 800) {
    /** GG; 2 */
    ag->ledBar.setColor(RGB_COLOR_G, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_G, ag->ledBar.getNumberOfLeds() - 2);
    totalUsed = 2;
  } else if (co2Value <= 1000) {
    /** YYY; 3 */
    ag->ledBar.setColor(RGB_COLOR_Y, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_Y, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_Y, ag->ledBar.getNumberOfLeds() - 3);
    totalUsed = 3;
  } else if (co2Value <= 1250) {
    /** OOOO; 4 */
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 4);
    totalUsed = 4;
  } else if (co2Value <= 1500) {
    /** OOOOO; 5 */
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 5);
    totalUsed = 5;
  } else if (co2Value <= 1750) {
    /** RRRRRR; 6 */
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 6);
    totalUsed = 6;
  } else if (co2Value <= 2000) {
    /** RRRRRRR; 7 */
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 7);
    totalUsed = 7;
  } else if (co2Value <= 3000) {
    /** PPPPPPPP; 8 */
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 8);
    totalUsed = 8;
  } else { /** > 3000 */
    /* PRPRPRPRP; 9 */
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 9);
    totalUsed = 9;
  }

  return totalUsed;
}

/**
 * @brief Show PM2.5 LED status
 *
 * @return return total number of led that are used on the monitor
 */
int StateMachine::pm25handleLeds(void) {
  int totalUsed = ag->ledBar.getNumberOfLeds();

  int pm25Value = round(value.getAverage(Measurements::PM25));
  if (config.hasSensorSHT && config.isPMCorrectionEnabled()) {
    pm25Value = round(value.getCorrectedPM25(true));
  }

  if (pm25Value <= 5) {
    /** G; 1 */
    ag->ledBar.setColor(RGB_COLOR_G, ag->ledBar.getNumberOfLeds() - 1);
    totalUsed = 1;
  } else if (pm25Value <= 9) {
    /** GG; 2 */
    ag->ledBar.setColor(RGB_COLOR_G, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_G, ag->ledBar.getNumberOfLeds() - 2);
    totalUsed = 2;
  } else if (pm25Value <= 20) {
    /** YYY; 3 */
    ag->ledBar.setColor(RGB_COLOR_Y, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_Y, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_Y, ag->ledBar.getNumberOfLeds() - 3);
    totalUsed = 3;
  } else if (pm25Value <= 35) {
    /** YYYY; 4 */
    ag->ledBar.setColor(RGB_COLOR_Y, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_Y, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_Y, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_Y, ag->ledBar.getNumberOfLeds() - 4);
    totalUsed = 4;
  } else if (pm25Value <= 45) {
    /** OOOOO; 5 */
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 5);
    totalUsed = 5;
  } else if (pm25Value <= 55) {
    /** OOOOOO; 6 */
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(RGB_COLOR_O, ag->ledBar.getNumberOfLeds() - 6);
    totalUsed = 6;
  } else if (pm25Value <= 100) {
    /** RRRRRRR; 7 */
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 7);
    totalUsed = 7;
  } else if (pm25Value <= 125) {
    /** RRRRRRRR; 8 */
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 8);
    totalUsed = 8;
  } else if (pm25Value <= 225) {
    /** PPPPPPPPP; 9 */
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 9);
    totalUsed = 9;
  } else { /** > 225 */
    /* PRPRPRPRP; 9 */
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(RGB_COLOR_R, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(RGB_COLOR_P, ag->ledBar.getNumberOfLeds() - 9);
    totalUsed = 9;
  }

  return totalUsed;
}

void StateMachine::co2Calibration(void) {
  if (config.isCo2CalibrationRequested() && config.hasSensorS8) {
    logInfo("CO2 Calibration");

    /** Count down to 0 then start */
    for (int i = 0; i < SENSOR_CO2_CALIB_COUNTDOWN_MAX; i++) {
      if (ag->isOne() || (ag->isPro4_2()) || ag->isPro3_3()) {
        String str =
            "after " + String(SENSOR_CO2_CALIB_COUNTDOWN_MAX - i) + " sec";
        disp.setText("Start CO2 calib", str.c_str(), "");
      } else if (ag->isBasic()) {
        String str = String(SENSOR_CO2_CALIB_COUNTDOWN_MAX - i) + " sec";
        disp.setText("CO2 Calib", "after", str.c_str());
      } else {
        logInfo("Start CO2 calib after " +
                String(SENSOR_CO2_CALIB_COUNTDOWN_MAX - i) + " sec");
      }
      delay(1000);
    }

    if (ag->s8.setBaselineCalibration()) {
      if (ag->isOne() || (ag->isPro4_2()) || ag->isPro3_3()) {
        disp.setText("Calibration", "success", "");
      } else if (ag->isBasic()) {
        disp.setText("CO2 Calib", "success", "");
      } else {
        logInfo("CO2 Calibration: success");
      }
      delay(1000);
      if (ag->isOne() || (ag->isPro4_2()) || ag->isPro3_3() || ag->isBasic()) {
        disp.setText("Wait for", "calib done", "...");
      } else {
        logInfo("CO2 Calibration: Wait for calibration finish...");
      }

      /** Count down wait for finish */
      int count = 0;
      while (ag->s8.isBaseLineCalibrationDone() == false) {
        delay(1000);
        count++;
      }
      if (ag->isOne() || (ag->isPro4_2()) || ag->isPro3_3() || ag->isBasic()) {
        String str = "after " + String(count);
        disp.setText("Calib done", str.c_str(), "sec");
      } else {
        logInfo("CO2 Calibration: finish after " + String(count) + " sec");
      }
      delay(2000);
    } else {
      if (ag->isOne() || (ag->isPro4_2()) || ag->isPro3_3()) {
        disp.setText("Calibration", "failure!!!", "");
      } else if (ag->isBasic()) {
        disp.setText("CO2 calib", "failure!!!", "");
      } else {
        logInfo("CO2 Calibration: failure!!!");
      }
      delay(2000);
    }
  }

  if (config.getCO2CalibrationAbcDays() >= 0 && config.hasSensorS8) {
    int newHour = config.getCO2CalibrationAbcDays() * 24;
    int curHour = ag->s8.getAbcPeriod();
    if (curHour != newHour) {
      String resultStr = "failure";
      if (ag->s8.setAbcPeriod(config.getCO2CalibrationAbcDays() * 24)) {
        resultStr = "successful";
      }
      String fromStr = String(curHour / 24) + " days";
      if (curHour == 0) {
        fromStr = "off";
      }
      String toStr = String(config.getCO2CalibrationAbcDays()) + " days";
      if (config.getCO2CalibrationAbcDays() == 0) {
        toStr = "off";
      }
      String msg =
          "Setting S8 from " + fromStr + " to " + toStr + " " + resultStr;
      logInfo(msg);
    }
  } else {
    logWarning("CO2 S8 not available, set 'abcDays' ignored");
  }
}

void StateMachine::ledBarTest(void) {
  if (config.isLedBarTestRequested()) {
    if (ag->isOne()) {
      ag->ledBar.clear();
      if (config.getCountry() == "TH") {
        uint32_t tstart = millis();
        logInfo("Start run LED test for 2 min");
        while (1) {
          ledBarRunTest();
          uint32_t ms = (uint32_t)(millis() - tstart);
          if (ms >= (60 * 1000 * 2)) {
            logInfo("LED test after 2 min finish");
            break;
          }
        }
      } else {
        ledBarRunTest();
      }
    } else if (ag->isOpenAir()) {
      ledBarRunTest();
    }
  }
}

void StateMachine::ledBarPowerUpTest(void) {
  if (ag->isOne()) {
    ag->ledBar.clear();
  }
  ledBarRunTest();
}

void StateMachine::ledBarRunTest(void) {
  if (ag->isOne()) {
    disp.setText("LED Test", "running", ".....");
    runLedTest('r');
    ag->ledBar.show();
    delay(1000);
    runLedTest('g');
    ag->ledBar.show();
    delay(1000);
    runLedTest('b');
    ag->ledBar.show();
    delay(1000);
    runLedTest('w');
    ag->ledBar.show();
    delay(1000);
    runLedTest('n');
    ag->ledBar.show();
    delay(1000);
  } else if (ag->isOpenAir()) {
    for (int i = 0; i < 100; i++) {
      ag->statusLed.setOn();
      delay(LED_TEST_BLINK_DELAY);
      ag->statusLed.setOff();
      delay(LED_TEST_BLINK_DELAY);
    }
  }
}

void StateMachine::runLedTest(char color) {
  int r = 0;
  int g = 0;
  int b = 0;
  switch (color) {
  case 'g':
    g = 255;
    break;
  case 'y':
    r = 255;
    g = 255;
    break;
  case 'o':
    r = 255;
    g = 128;
    break;
  case 'r':
    r = 255;
    break;
  case 'b':
    b = 255;
    break;
  case 'w':
    r = 255;
    g = 255;
    b = 255;
    break;
  case 'p':
    r = 153;
    b = 153;
    break;
  case 'z':
    r = 102;
    break;
  case 'n':
  default:
    break;
  }
  ag->ledBar.setColor(r, g, b);
}

/**
 * @brief Construct a new Ag State Machine:: Ag State Machine object
 *
 * @param disp OledDisplay
 * @param log Serial Stream
 * @param value Measurements
 * @param config Configuration
 */
StateMachine::StateMachine(OledDisplay &disp, Stream &log, Measurements &value,
                           Configuration &config)
    : PrintLog(log, "StateMachine"), disp(disp), value(value), config(config) {}

StateMachine::~StateMachine() {}

/**
 * @brief OLED display show content from state value
 *
 * @param state
 */
void StateMachine::displayHandle(AgStateMachineState state) {
  // Ignore handle if not support display
  if (!(ag->isOne() || (ag->isPro4_2()) || ag->isPro3_3() || ag->isBasic())) {
    if (state == AgStateMachineCo2Calibration) {
      co2Calibration();
    }
    return;
  }

  if (state > AgStateMachineNormal) {
    logError("displayHandle: State invalid");
    return;
  }

  dispState = state;

  switch (state) {
  case AgStateMachineWiFiManagerMode:
  case AgStateMachineWiFiManagerPortalActive: {
    if (wifiConnectCountDown >= 0) {
      if (ag->isBasic()) {
        String ssid = "\"airgradient-" + ag->deviceId() + "\"    " +
                      String(wifiConnectCountDown) + String("s");
        disp.setText("Connect tohotspot:", ssid.c_str(), "");
      } else {
        String line1 = String(wifiConnectCountDown) + "s to connect";
        String line2 = "to WiFi hotspot:";
        String line3 = "\"airgradient-";
        String line4 = ag->deviceId() + "\"";
        disp.setText(line1, line2, line3, line4);
      }
      wifiConnectCountDown--;
    }
    break;
  }
  case AgStateMachineWiFiManagerStaConnecting: {
    disp.setText("Trying to", "connect to WiFi", "...");
    break;
  }
  case AgStateMachineWiFiManagerStaConnected: {
    disp.setText("WiFi connection", "successful", "");
    break;
  }
  case AgStateMachineWiFiOkServerConnecting: {
    if (ag->isBasic()) {
      disp.setText("Connecting", "to", "Server...");
    } else {
      disp.setText("Connecting to", "Server", "...");
    }

    break;
  }
  case AgStateMachineWiFiOkServerConnected: {
    disp.setText("Server", "connection", "successful");
    break;
  }
  case AgStateMachineWiFiManagerConnectFailed: {
    disp.setText("WiFi not", "connected", "");
    break;
  }
  case AgStateMachineWiFiOkServerConnectFailed: {
    // displayShowText("Server not", "reachable", "");
    break;
  }
  case AgStateMachineWiFiOkServerOkSensorConfigFailed: {
    if (ag->isBasic()) {
      disp.setText("Monitor", "not on", "dashboard");
    } else {
      disp.setText("Monitor not", "setup on", "dashboard");
    }
    break;
  }
  case AgStateMachineWiFiLost: {
    disp.showDashboard(OledDisplay::DashBoardStatusWiFiIssue);
    break;
  }
  case AgStateMachineServerLost: {
    disp.showDashboard(OledDisplay::DashBoardStatusServerIssue);
    break;
  }
  case AgStateMachineSensorConfigFailed: {
    if (addToDashBoard) {
      uint32_t ms = (uint32_t)(millis() - addToDashboardTime);
      if (ms >= 5000) {
        addToDashboardTime = millis();
        if (addToDashBoardToggle) {
          disp.showDashboard(OledDisplay::DashBoardStatusAddToDashboard);
        } else {
          disp.showDashboard(OledDisplay::DashBoardStatusDeviceId);
        }
        addToDashBoardToggle = !addToDashBoardToggle;
      }
    } else {
      disp.showDashboard();
    }
    break;
  }
  case AgStateMachineNormal: {
    if (config.isOfflineMode()) {
      disp.showDashboard(
          OledDisplay::DashBoardStatusOfflineMode);
    } else {
      disp.showDashboard();
    }
    break;
  }
  case AgStateMachineCo2Calibration:
    co2Calibration();
    break;
  default:
    break;
  }
}

/**
 * @brief OLED display show content as previous state updated
 *
 */
void StateMachine::displayHandle(void) { displayHandle(dispState); }

/**
 * @brief Update status add to dashboard
 *
 */
void StateMachine::displaySetAddToDashBoard(void) {
  if (addToDashBoard == false) {
    addToDashboardTime = 0;
    addToDashBoardToggle = true;
  }
  addToDashBoard = true;
}

void StateMachine::displayClearAddToDashBoard(void) { addToDashBoard = false; }

/**
 * @brief Set WiFi connection coundown on dashboard
 *
 * @param count Seconds
 */
void StateMachine::displayWiFiConnectCountDown(int count) {
  wifiConnectCountDown = count;
}

/**
 * @brief Init before start LED bar animation
 *
 */
void StateMachine::ledAnimationInit(void) { ledBarAnimationCount = -1; }

/**
 * @brief Handle LED from state, only handle LED if board type is: One Indoor or
 * Open Air
 *
 * @param state
 */
void StateMachine::handleLeds(AgStateMachineState state) {
  /** Ignore if board type if not ONE_INDOOR or OPEN_AIR_OUTDOOR */
  if ((ag->getBoardType() != BoardType::ONE_INDOOR) &&
      (ag->getBoardType() != BoardType::OPEN_AIR_OUTDOOR)) {
    return;
  }

  if (state > AgStateMachineNormal) {
    logError("ledHandle: state invalid");
    return;
  }

  ledState = state;
  switch (state) {
  case AgStateMachineWiFiManagerMode: {
    /** In WiFi Manager Mode */
    /** Turn LED OFF */
    /** Turn middle LED Color */
    if (ag->isOne()) {
      ag->ledBar.clear();
      ag->ledBar.setColor(0, 0, 255, ag->ledBar.getNumberOfLeds() / 2);
    } else {
      ag->statusLed.setToggle();
    }
    break;
  }
  case AgStateMachineWiFiManagerPortalActive: {
    /** WiFi Manager has connected to mobile phone */
    if (ag->isOne()) {
      ag->ledBar.clear();
      ag->ledBar.setColor(0, 0, 255);
    } else {
      ag->statusLed.setOn();
    }
    break;
  }
  case AgStateMachineWiFiManagerStaConnecting: {
    /** after SSID and PW entered and OK clicked, connection to WiFI network is
     * attempted */
    if (ag->isOne()) {
      ag->ledBar.clear();
      ledBarSingleLedAnimation(255, 255, 255);
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineWiFiManagerStaConnected: {
    /** Connecting to WiFi worked */
    if (ag->isOne()) {
      ag->ledBar.clear();
      ag->ledBar.setColor(255, 255, 255);
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineWiFiOkServerConnecting: {
    /** once connected to WiFi an attempt to reach the server is performed */
    if (ag->isOne()) {
      ag->ledBar.clear();
      ledBarSingleLedAnimation(0, 255, 0);
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineWiFiOkServerConnected: {
    /** Server is reachable, all ﬁne */
    if (ag->isOne()) {
      ag->ledBar.clear();
      ag->ledBar.setColor(0, 255, 0);
    } else {
      ag->statusLed.setOff();

      /** two time slow blink, then off */
      for (int i = 0; i < 2; i++) {
        ledStatusBlinkDelay(LED_SLOW_BLINK_DELAY);
      }

      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineWiFiManagerConnectFailed: {
    /** Cannot connect to WiFi (e.g. wrong password, WPA Enterprise etc.) */
    if (ag->isOne()) {
      ag->ledBar.clear();
      ag->ledBar.setColor(255, 0, 0);
    } else {
      ag->statusLed.setOff();

      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 3; i++) {
          ledStatusBlinkDelay(LED_FAST_BLINK_DELAY);
        }
        delay(2000);
      }
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineWiFiOkServerConnectFailed: {
    /** Connected to WiFi but server not reachable, e.g. firewall block/
     * whitelisting needed etc. */
    if (ag->isOne()) {
      ag->ledBar.clear();
      ag->ledBar.setColor(233, 183, 54); /** orange */
    } else {
      ag->statusLed.setOff();
      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 4; i++) {
          ledStatusBlinkDelay(LED_FAST_BLINK_DELAY);
        }
        delay(2000);
      }
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineWiFiOkServerOkSensorConfigFailed: {
    /** Server reachable but sensor not configured correctly */
    if (ag->isOne()) {
      ag->ledBar.clear();
      ag->ledBar.setColor(139, 24, 248); /** violet */
    } else {
      ag->statusLed.setOff();
      for (int j = 0; j < 3; j++) {
        for (int i = 0; i < 5; i++) {
          ledStatusBlinkDelay(LED_FAST_BLINK_DELAY);
        }
        delay(2000);
      }
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineWiFiLost: {
    /** Connection to WiFi network failed credentials incorrect encryption not
     * supported etc. */
    if (ag->isOne()) {
      bool allUsed = sensorhandleLeds();
      if (allUsed == false) {
        ag->ledBar.setColor(255, 0, 0, 0);
      }
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineServerLost: {
    /** Connected to WiFi network but the server cannot be reached through the
     * internet, e.g. blocked by firewall */
    if (ag->isOne()) {
      bool allUsed = sensorhandleLeds();
      if (allUsed == false) {
        ag->ledBar.setColor(233, 183, 54, 0);
      }
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineSensorConfigFailed: {
    /** Server is reachable but there is some configuration issue to be fixed on
     * the server side */
    if (ag->isOne()) {
      bool allUsed = sensorhandleLeds();
      if (allUsed == false) {
        ag->ledBar.setColor(139, 24, 248, 0);
      }
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineNormal: {
    if (ag->isOne()) {
      sensorhandleLeds();
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineLedBarTest:
    ledBarTest();
    break;
  case AgStateMachineLedBarPowerUpTest:
    ledBarPowerUpTest();
  default:
    break;
  }

  // Show LED bar color
  if (ag->isOne()) {
    ag->ledBar.show();
  }
}

/**
 * @brief Handle LED as previous state updated
 *
 */
void StateMachine::handleLeds(void) { handleLeds(ledState); }

/**
 * @brief Set display state
 *
 * @param state
 */
void StateMachine::setDisplayState(AgStateMachineState state) {
  dispState = state;
}

/**
 * @brief Get current display state
 *
 * @return AgStateMachineState
 */
AgStateMachineState StateMachine::getDisplayState(void) { return dispState; }

/**
 * @brief Set AirGradient instance
 *
 * @param ag Point to AirGradient instance
 */
void StateMachine::setAirGradient(AirGradient *ag) { this->ag = ag; }

/**
 * @brief Get current LED state
 *
 * @return AgStateMachineState
 */
AgStateMachineState StateMachine::getLedState(void) { return ledState; }

void StateMachine::executeCo2Calibration(void) {
  displayHandle(AgStateMachineCo2Calibration);
}

void StateMachine::executeLedBarTest(void) {
  handleLeds(AgStateMachineLedBarTest);
}

void StateMachine::executeLedBarPowerUpTest(void) {
  handleLeds(AgStateMachineLedBarPowerUpTest);
}
