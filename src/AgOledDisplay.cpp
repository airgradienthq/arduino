#include "AgOledDisplay.h"
#include "Libraries/U8g2/src/U8g2lib.h"
#include "Main/utils.h"

/** Cast U8G2 */
#define DISP() ((U8G2_SH1106_128X64_NONAME_F_HW_I2C *)(this->u8g2))

/**
 * @brief Show dashboard temperature and humdity
 *
 * @param hasStatus
 */
void OledDisplay::showTempHum(bool hasStatus, char *buf, int buf_size) {
  /** Temperature */
  float temp = value.getAverage(Measurements::Temperature);
  if (utils::isValidTemperature(temp)) {
    float t = 0.0f;
    if (config.isTemperatureUnitInF()) {
      t = utils::degreeC_To_F(temp);
    } else {
      t = temp;
    }

    if (config.isTemperatureUnitInF()) {
      if (hasStatus) {
        snprintf(buf, buf_size, "%0.1f", t);
      } else {
        snprintf(buf, buf_size, "%0.1f°F", t);
      }
    } else {
      if (hasStatus) {
        snprintf(buf, buf_size, "%.1f", t);
      } else {
        snprintf(buf, buf_size, "%.1f°C", t);
      }
    }
  } else { /** Show invalid value */
    if (config.isTemperatureUnitInF()) {
      snprintf(buf, buf_size, "-°F");
    } else {
      snprintf(buf, buf_size, "-°C");
    }
  }
  DISP()->drawUTF8(1, 10, buf);

  /** Show humidity */
  int rhum = round(value.getAverage(Measurements::Humidity));
  if (utils::isValidHumidity(rhum)) {
    snprintf(buf, buf_size, "%d%%", rhum);
  } else {
    snprintf(buf, buf_size, "-%%");
  }

  if (rhum > 99.0) {
    DISP()->drawStr(97, 10, buf);
  } else {
    DISP()->drawStr(105, 10, buf);
  }
}

void OledDisplay::setCentralText(int y, String text) {
  setCentralText(y, text.c_str());
}

void OledDisplay::setCentralText(int y, const char *text) {
  int x = (DISP()->getWidth() - DISP()->getStrWidth(text)) / 2;
  DISP()->drawStr(x, y, text);
}

/**
 * @brief Construct a new Ag Oled Display:: Ag Oled Display object
 *
 * @param config AgConfiguration
 * @param value Measurements
 * @param log Serial Stream
 */
OledDisplay::OledDisplay(Configuration &config, Measurements &value,
                         Stream &log)
    : PrintLog(log, "OledDisplay"), config(config), value(value) {}

/**
 * @brief Set AirGradient instance
 *
 * @param ag Point to AirGradient instance
 */
void OledDisplay::setAirGradient(AirGradient *ag) { this->ag = ag; }

OledDisplay::~OledDisplay() {}

/**
 * @brief Initialize display
 *
 * @return true Success
 * @return false Failure
 */
bool OledDisplay::begin(void) {
  if (isBegin) {
    logWarning("Already begin, call 'end' and try again");
    return true;
  }

  if (ag->isOne() || ag->isPro3_3() || ag->isPro4_2()) {
    /** Create u8g2 instance */
    u8g2 = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
    if (u8g2 == NULL) {
      logError("Create 'U8G2' failed");
      return false;
    }

    /** Init u8g2 */
    if (DISP()->begin() == false) {
      logError("U8G2 'begin' failed");
      return false;
    }
  } else if (ag->isBasic()) {
    logInfo("DIY_BASIC init");
    ag->display.begin(Wire);
    ag->display.setTextColor(1);
    ag->display.clear();
    ag->display.show();
  }

  /** Show low brightness on startup. then it's completely turn off on main
   * application */
  int brightness = config.getDisplayBrightness();
  if (brightness == 0) {
    setBrightness(1);
  }

  isBegin = true;
  logInfo("begin");
  return true;
}

/**
 * @brief De-Initialize display
 *
 */
void OledDisplay::end(void) {
  if (!isBegin) {
    logWarning("Already end, call 'begin' and try again");
    return;
  }

  if (ag->isOne() || ag->isPro3_3() || ag->isPro4_2()) {
    /** Free u8g2 */
    delete DISP();
    u8g2 = NULL;
  } else if (ag->isBasic()) {
    ag->display.end();
  }

  isBegin = false;
  logInfo("end");
}

/**
 * @brief Show text on 3 line of display
 *
 * @param line1
 * @param line2
 * @param line3
 */
void OledDisplay::setText(String &line1, String &line2, String &line3) {
  setText(line1.c_str(), line2.c_str(), line3.c_str());
}

/**
 * @brief Show text on 3 line of display
 *
 * @param line1
 * @param line2
 * @param line3
 */
void OledDisplay::setText(const char *line1, const char *line2,
                          const char *line3) {
  if (isDisplayOff) {
    return;
  }

  if (ag->isOne() || ag->isPro3_3() || ag->isPro4_2()) {
    DISP()->firstPage();
    do {
      DISP()->setFont(u8g2_font_t0_16_tf);
      DISP()->drawStr(1, 10, line1);
      DISP()->drawStr(1, 30, line2);
      DISP()->drawStr(1, 50, line3);
    } while (DISP()->nextPage());
  } else if (ag->isBasic()) {
    ag->display.clear();

    ag->display.setCursor(1, 1);
    ag->display.setText(line1);
    ag->display.setCursor(1, 17);
    ag->display.setText(line2);
    ag->display.setCursor(1, 33);
    ag->display.setText(line3);

    ag->display.show();
  }
}

/**
 * @brief Set Text on 4 line
 *
 * @param line1
 * @param line2
 * @param line3
 * @param line4
 */
void OledDisplay::setText(String &line1, String &line2, String &line3,
                          String &line4) {
  setText(line1.c_str(), line2.c_str(), line3.c_str(), line4.c_str());
}

/**
 * @brief Set Text on 4 line
 *
 * @param line1
 * @param line2
 * @param line3
 * @param line4
 */
void OledDisplay::setText(const char *line1, const char *line2,
                          const char *line3, const char *line4) {
  if (isDisplayOff) {
    return;
  }

  if (ag->isOne() || ag->isPro3_3() || ag->isPro4_2()) {
    DISP()->firstPage();
    do {
      DISP()->setFont(u8g2_font_t0_16_tf);
      DISP()->drawStr(1, 10, line1);
      DISP()->drawStr(1, 25, line2);
      DISP()->drawStr(1, 40, line3);
      DISP()->drawStr(1, 55, line4);
    } while (DISP()->nextPage());
  } else if (ag->isBasic()) {
    ag->display.clear();
    ag->display.setCursor(0, 0);
    ag->display.setText(line1);
    ag->display.setCursor(0, 10);
    ag->display.setText(line2);
    ag->display.setCursor(0, 20);
    ag->display.setText(line3);
    ag->display.show();
  }
}

/**
 * @brief Update dashboard content
 *
 */
void OledDisplay::showDashboard(void) { showDashboard(NULL); }

/**
 * @brief Update dashboard content and error status
 *
 */
void OledDisplay::showDashboard(const char *status) {
  if (isDisplayOff) {
    return;
  }

  char strBuf[16];
  if (ag->isOne() || ag->isPro3_3() || ag->isPro4_2()) {
    DISP()->firstPage();
    do {
      DISP()->setFont(u8g2_font_t0_16_tf);
      if ((status == NULL) || (strlen(status) == 0)) {
        showTempHum(false, strBuf, sizeof(strBuf));
      } else {
        String strStatus = "Show status: " + String(status);
        logInfo(strStatus);

        int strWidth = DISP()->getStrWidth(status);
        DISP()->drawStr((DISP()->getWidth() - strWidth) / 2, 10, status);

        /** Show WiFi NA*/
        if (strcmp(status, "WiFi N/A") == 0) {
          DISP()->setFont(u8g2_font_t0_12_tf);
          showTempHum(true, strBuf, sizeof(strBuf));
        }
      }

      /** Draw horizonal line */
      DISP()->drawLine(1, 13, 128, 13);

      /** Show CO2 label */
      DISP()->setFont(u8g2_font_t0_12_tf);
      DISP()->drawUTF8(1, 27, "CO2");

      DISP()->setFont(u8g2_font_t0_22b_tf);
      int co2 = round(value.getAverage(Measurements::CO2));
      if (utils::isValidCO2(co2)) {
        sprintf(strBuf, "%d", co2);
      } else {
        sprintf(strBuf, "%s", "-");
      }
      DISP()->drawStr(1, 48, strBuf);

      /** Show CO2 value index */
      DISP()->setFont(u8g2_font_t0_12_tf);
      DISP()->drawStr(1, 61, "ppm");

      /** Draw vertical line */
      DISP()->drawLine(52, 14, 52, 64);
      DISP()->drawLine(97, 14, 97, 64);

      /** Draw PM2.5 label */
      DISP()->setFont(u8g2_font_t0_12_tf);
      DISP()->drawStr(55, 27, "PM2.5");

      /** Draw PM2.5 value */
      int pm25 = round(value.getAverage(Measurements::PM25));
      if (utils::isValidPm(pm25)) {
        if (config.hasSensorSHT && config.isPMCorrectionEnabled()) {
          pm25 = round(value.getCorrectedPM25(*ag, config, true));
        }
        if (config.isPmStandardInUSAQI()) {
          sprintf(strBuf, "%d", ag->pms5003.convertPm25ToUsAqi(pm25));
        } else {
          sprintf(strBuf, "%d", pm25);
        }
      } else { /** Show invalid value. */
        sprintf(strBuf, "%s", "-");
      }
      DISP()->setFont(u8g2_font_t0_22b_tf);
      DISP()->drawStr(55, 48, strBuf);

      /** Draw PM2.5 unit */
      DISP()->setFont(u8g2_font_t0_12_tf);
      if (config.isPmStandardInUSAQI()) {
        DISP()->drawUTF8(55, 61, "AQI");
      } else {
        DISP()->drawUTF8(55, 61, "ug/m³");
      }

      /** Draw tvocIndexlabel */
      DISP()->setFont(u8g2_font_t0_12_tf);
      DISP()->drawStr(100, 27, "VOC:");

      /** Draw tvocIndexvalue */
      int tvoc = round(value.getAverage(Measurements::TVOC));
      if (utils::isValidVOC(tvoc)) {
        sprintf(strBuf, "%d", tvoc);
      } else {
        sprintf(strBuf, "%s", "-");
      }
      DISP()->drawStr(100, 39, strBuf);

      /** Draw NOx label */
      int nox = round(value.getAverage(Measurements::NOx));
      DISP()->drawStr(100, 53, "NOx:");
      if (utils::isValidNOx(nox)) {
        sprintf(strBuf, "%d", nox);
      } else {
        sprintf(strBuf, "%s", "-");
      }
      DISP()->drawStr(100, 63, strBuf);
    } while (DISP()->nextPage());
  } else if (ag->isBasic()) {
    ag->display.clear();

    /** Set CO2 */
    int co2 = round(value.getAverage(Measurements::CO2));
    if (utils::isValidCO2(co2)) {
      snprintf(strBuf, sizeof(strBuf), "CO2:%d", co2);
    } else {
      snprintf(strBuf, sizeof(strBuf), "CO2:-");
    }

    ag->display.setCursor(0, 0);
    ag->display.setText(strBuf);

    /** Set PM */
    int pm25 = round(value.getAverage(Measurements::PM25));
    if (config.hasSensorSHT && config.isPMCorrectionEnabled()) {
      pm25 = round(value.getCorrectedPM25(*ag, config, true));
    }

    ag->display.setCursor(0, 12);
    if (utils::isValidPm(pm25)) {
      snprintf(strBuf, sizeof(strBuf), "PM2.5:%d", pm25);
    } else {
      snprintf(strBuf, sizeof(strBuf), "PM2.5:-");
    }
    ag->display.setText(strBuf);

    /** Set temperature and humidity */
    float temp = value.getAverage(Measurements::Temperature);
    if (utils::isValidTemperature(temp)) {
      if (config.isTemperatureUnitInF()) {
        snprintf(strBuf, sizeof(strBuf), "T:%0.1f F", utils::degreeC_To_F(temp));
      } else {
        snprintf(strBuf, sizeof(strBuf), "T:%0.1f C", temp);
      }
    } else {
      if (config.isTemperatureUnitInF()) {
        snprintf(strBuf, sizeof(strBuf), "T:-F");
      } else {
        snprintf(strBuf, sizeof(strBuf), "T:-C");
      }
    }

    ag->display.setCursor(0, 24);
    ag->display.setText(strBuf);

    int rhum = round(value.getAverage(Measurements::Humidity));
    if (utils::isValidHumidity(rhum)) {
      snprintf(strBuf, sizeof(strBuf), "H:%d %%", rhum);
    } else {
      snprintf(strBuf, sizeof(strBuf), "H:- %%");
    }

    ag->display.setCursor(0, 36);
    ag->display.setText(strBuf);

    ag->display.show();
  }
}

void OledDisplay::setBrightness(int percent) {
  if (ag->isOne() || ag->isPro3_3() || ag->isPro4_2()) {
    if (percent == 0) {
      isDisplayOff = true;

      // Clear display.
      DISP()->firstPage();
      do {
      } while (DISP()->nextPage());

    } else {
      isDisplayOff = false;
      DISP()->setContrast((127 * percent) / 100);
    }
  } else if (ag->isBasic()) {
    if (percent == 0) {
      isDisplayOff = true;

      // Clear display.
      ag->display.clear();
      ag->display.show();
    }
    else {
      isDisplayOff = false;
      ag->display.setContrast((255 * percent) / 100);
    }
  }
}

#ifdef ESP32
void OledDisplay::showFirmwareUpdateVersion(String version) {
  if (isDisplayOff) {
    return;
  }

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    setCentralText(20, "Firmware Update");
    setCentralText(40, "New version");
    setCentralText(60, version.c_str());
  } while (DISP()->nextPage());
}

void OledDisplay::showFirmwareUpdateProgress(int percent) {
  if (isDisplayOff) {
    return;
  }

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    setCentralText(20, "Firmware Update");
    setCentralText(50, String("Updating... ") + String(percent) + String("%"));
  } while (DISP()->nextPage());
}

void OledDisplay::showFirmwareUpdateSuccess(int count) {
  if (isDisplayOff) {
    return;
  }

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    setCentralText(20, "Firmware Update");
    setCentralText(40, "Success");
    setCentralText(60, String("Rebooting... ") + String(count));
  } while (DISP()->nextPage());
}

void OledDisplay::showFirmwareUpdateFailed(void) {
  if (isDisplayOff) {
    return;
  }

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    setCentralText(20, "Firmware Update");
    setCentralText(40, "fail, will retry");
    // setCentralText(60, "will retry");
  } while (DISP()->nextPage());
}

void OledDisplay::showFirmwareUpdateSkipped(void) {
  if (isDisplayOff) {
    return;
  }

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    setCentralText(20, "Firmware Update");
    setCentralText(40, "skipped");
  } while (DISP()->nextPage());
}

void OledDisplay::showFirmwareUpdateUpToDate(void) {
  if (isDisplayOff) {
    return;
  }

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    setCentralText(20, "Firmware Update");
    setCentralText(40, "up to date");
  } while (DISP()->nextPage());
}
#else

#endif

void OledDisplay::showRebooting(void) {
  if (ag->isOne() || ag->isPro3_3() || ag->isPro4_2()) {
    DISP()->firstPage();
    do {
      DISP()->setFont(u8g2_font_t0_16_tf);
      // setCentralText(20, "Firmware Update");
      setCentralText(40, "Rebooting...");
      // setCentralText(60, String("Retry after 24h"));
    } while (DISP()->nextPage());
  } else if (ag->isBasic()) {
    ag->display.clear();

    ag->display.setCursor(0, 20);
    ag->display.setText("Rebooting...");

    ag->display.show();
  }
}
