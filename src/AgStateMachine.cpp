#include "AgStateMachine.h"

#define LED_FAST_BLINK_DELAY 250  /** ms */
#define LED_SLOW_BLINK_DELAY 1000 /** ms */
#define LED_SHORT_BLINK_DELAY 500 /** ms */
#define LED_LONG_BLINK_DELAY 2000 /** ms */

/**
 * @brief Animation LED bar with color
 * 
 * @param r 
 * @param g 
 * @param b 
 */
void AgStateMachine::ledBarSingleLedAnimation(uint8_t r, uint8_t g, uint8_t b) {
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
void AgStateMachine::ledStatusBlinkDelay(uint32_t ms) {
  ag->statusLed.setOn();
  delay(ms);
  ag->statusLed.setOff();
  delay(ms);
}

/**
 * @brief Led bar show led color status
 * 
 */
void AgStateMachine::sensorLedHandle(void) {
  switch (config.getLedBarMode()) {
  case LedBarMode::LedBarModeCO2:
    co2LedHandle();
    break;
  case LedBarMode::LedBarModePm:
    pm25LedHandle();
    break;
  default:
    ag->ledBar.clear();
    break;
  }
}

/**
 * @brief Show CO2 LED status
 * 
 */
void AgStateMachine::co2LedHandle(void) {
  int co2Value = value.CO2;
  if (co2Value <= 400) {
    /** G; 1 */
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
  } else if (co2Value <= 700) {
    /** GG; 2 */
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
  } else if (co2Value <= 1000) {
    /** YYY; 3 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
  } else if (co2Value <= 1333) {
    /** YYYY; 4 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 4);
  } else if (co2Value <= 1666) {
    /** YYYYY; 5 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 5);
  } else if (co2Value <= 2000) {
    /** RRRRRR; 6 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
  } else if (co2Value <= 2666) {
    /** RRRRRRR; 7 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
  } else if (co2Value <= 3333) {
    /** RRRRRRRR; 8 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
  } else if (co2Value <= 4000) {
    /** RRRRRRRRR; 9 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 9);
  } else { /** > 4000 */
    /* PRPRPRPRP; 9 */
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 9);
  }
}

/**
 * @brief Show PM2.5 LED status
 * 
 */
void AgStateMachine::pm25LedHandle(void) {
  int pm25Value = value.PM25;
  if (pm25Value <= 5) {
    /** G; 1 */
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
  } else if (pm25Value <= 10) {
    /** GG; 2 */
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(0, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
  } else if (pm25Value <= 20) {
    /** YYY; 3 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
  } else if (pm25Value <= 35) {
    /** YYYY; 4 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 4);
  } else if (pm25Value <= 45) {
    /** YYYYY; 5 */
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 255, 0, ag->ledBar.getNumberOfLeds() - 5);
  } else if (pm25Value <= 55) {
    /** RRRRRR; 6 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
  } else if (pm25Value <= 65) {
    /** RRRRRRR; 7 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
  } else if (pm25Value <= 150) {
    /** RRRRRRRR; 8 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
  } else if (pm25Value <= 250) {
    /** RRRRRRRRR; 9 */
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 9);
  } else { /** > 250 */
    /* PRPRPRPRP; 9 */
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 1);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 2);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 3);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 4);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 5);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 6);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 7);
    ag->ledBar.setColor(255, 0, 0, ag->ledBar.getNumberOfLeds() - 8);
    ag->ledBar.setColor(153, 153, 0, ag->ledBar.getNumberOfLeds() - 9);
  }
}

/**
 * @brief Construct a new Ag State Machine:: Ag State Machine object
 * 
 * @param disp AgOledDisplay
 * @param log Serial Stream
 * @param value AgValue
 * @param config AgConfigure
 */
AgStateMachine::AgStateMachine(AgOledDisplay &disp, Stream &log, AgValue &value,
                               AgConfigure &config)
    : PrintLog(log, "AgStateMachine"), disp(disp), value(value),
      config(config) {}

AgStateMachine::~AgStateMachine() {}

/**
 * @brief OLED display show content from state value
 * 
 * @param state 
 */
void AgStateMachine::displayHandle(AgStateMachineState state) {
  // Ignore handle if not ONE_INDOOR board
  if (!ag->isOneIndoor()) {
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
      String line1 = String(wifiConnectCountDown) + "s to connect";
      String line2 = "to WiFi hotspot:";
      String line3 = "\"airgradient-";
      String line4 = ag->deviceId() + "\"";
      disp.setText(line1, line2, line3, line4);
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
    disp.setText("Connecting to", "Server", "...");
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
    disp.setText("Monitor not", "setup on", "dashboard");
    break;
  }
  case AgStateMachineWiFiLost: {
    disp.showDashboard("WiFi N/A");
    break;
  }
  case AgStateMachineServerLost: {
    disp.showDashboard("Server N/A");
    break;
  }
  case AgStateMachineSensorConfigFailed: {
    uint32_t ms = (uint32_t)(millis() - addToDashboardTime);
    if (ms >= 5000) {
      addToDashboardTime = millis();
      if (addToDashBoard) {
        disp.showDashboard("Add to Dashboard");
      } else {
        disp.showDashboard(ag->deviceId().c_str());
      }
      addToDashBoard = !addToDashBoard;
    }
    break;
  }
  case AgStateMachineNormal: {
    disp.showDashboard();
    break;
  }
  default:
    break;
  }
}

/**
 * @brief OLED display show content as previous state updated
 * 
 */
void AgStateMachine::displayHandle(void) { displayHandle(dispState); }

/**
 * @brief Update status add to dashboard
 * 
 */
void AgStateMachine::displaySetAddToDashBoard(void) {
  addToDashBoard = true;
  addToDashboardTime = millis();
}

void AgStateMachine::displayClearAddToDashBoard(void) {
  addToDashBoard = false;
}

/**
 * @brief Set WiFi connection coundown on dashboard
 * 
 * @param count Seconds
 */
void AgStateMachine::displayWiFiConnectCountDown(int count) {
  wifiConnectCountDown = count;
}

/**
 * @brief Init before start LED bar animation
 * 
 */
void AgStateMachine::ledAnimationInit(void) { ledBarAnimationCount = -1; }

/**
 * @brief Handle LED from state
 * 
 * @param state 
 */
void AgStateMachine::ledHandle(AgStateMachineState state) {
  if (state > AgStateMachineNormal) {
    logError("ledHandle: state invalid");
    return;
  }

  ledState = state;
  if (ag->isOneIndoor()) {
    ag->ledBar.clear(); // Set all LED OFF
  }
  switch (state) {
  case AgStateMachineWiFiManagerMode: {
    /** In WiFi Manager Mode */
    /** Turn LED OFF */
    /** Turn midle LED Color */
    if (ag->isOneIndoor()) {
      ag->ledBar.setColor(0, 0, 255, ag->ledBar.getNumberOfLeds() / 2);
    } else {
      ag->statusLed.setToggle();
    }
    break;
  }
  case AgStateMachineWiFiManagerPortalActive: {
    /** WiFi Manager has connected to mobile phone */
    if (ag->isOneIndoor()) {
      ag->ledBar.setColor(0, 0, 255);
    } else {
      ag->statusLed.setOn();
    }
    break;
  }
  case AgStateMachineWiFiManagerStaConnecting: {
    /** after SSID and PW entered and OK clicked, connection to WiFI network is
     * attempted */
    if (ag->isOneIndoor()) {
      ledBarSingleLedAnimation(255, 255, 255);
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineWiFiManagerStaConnected: {
    /** Connecting to WiFi worked */
    if (ag->isOneIndoor()) {
      ag->ledBar.setColor(255, 255, 255);
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineWiFiOkServerConnecting: {
    /** once connected to WiFi an attempt to reach the server is performed */
    if (ag->isOneIndoor()) {
      ledBarSingleLedAnimation(0, 255, 0);
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineWiFiOkServerConnected: {
    /** Server is reachable, all ﬁne */
    if (ag->isOneIndoor()) {
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
    if (ag->isOneIndoor()) {
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
    if (ag->isOneIndoor()) {
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
    if (ag->isOneIndoor()) {
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
    if (ag->isOneIndoor()) {
      /** WIFI failed status LED color */
      ag->ledBar.setColor(255, 0, 0, 0);
      /** Show CO2 or PM color status */
      // sensorLedColorHandler();
      sensorLedHandle();
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineServerLost: {
    /** Connected to WiFi network but the server cannot be reached through the
     * internet, e.g. blocked by firewall */
    if (ag->isOneIndoor()) {
      ag->ledBar.setColor(233, 183, 54, 0);

      /** Show CO2 or PM color status */
      sensorLedHandle();
      // sensorLedColorHandler();
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineSensorConfigFailed: {
    /** Server is reachable but there is some conﬁguration issue to be fixed on
     * the server side */
    if (ag->isOneIndoor()) {
      ag->ledBar.setColor(139, 24, 248, 0);

      /** Show CO2 or PM color status */
      sensorLedHandle();
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  case AgStateMachineNormal: {
    if (ag->isOneIndoor()) {
      sensorLedHandle();
    } else {
      ag->statusLed.setOff();
    }
    break;
  }
  default:
    break;
  }

  // Show LED bar color
  if (ag->isOneIndoor()) {
    ag->ledBar.show();
  }
}

/**
 * @brief Handle LED as previous state updated
 * 
 */
void AgStateMachine::ledHandle(void) { ledHandle(ledState); }

/**
 * @brief Set display state
 * 
 * @param state 
 */
void AgStateMachine::setDisplayState(AgStateMachineState state) {
  dispState = state;
}

/**
 * @brief Get current display state
 * 
 * @return AgStateMachineState 
 */
AgStateMachineState AgStateMachine::getDisplayState(void) { return dispState; }

/**
 * @brief Set AirGradient instance
 * 
 * @param ag Point to AirGradient instance
 */
void AgStateMachine::setAirGradient(AirGradient *ag) { this->ag = ag; }

/**
 * @brief Get current LED state
 * 
 * @return AgStateMachineState 
 */
AgStateMachineState AgStateMachine::getLedState(void) { return ledState; }
