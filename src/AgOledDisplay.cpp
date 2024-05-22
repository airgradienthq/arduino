#include "AgOledDisplay.h"
#include "Libraries/U8g2/src/U8g2lib.h"
#ifdef ESP32
#include "WiFi.h"
#else 
#include "ESP8266WiFi.h"
#endif

typedef struct {
  const int width;
  const int height;
  const unsigned char* icon;
} xbm_icon_t;

/** WiFi ICON */
enum {
  WIFI_ICON_ISSUE,
  WIFI_ICON_25,
  WIFI_ICON_50,
  WIFI_ICON_75,
  WIFI_ICON_100,
  WIFI_ICON_MAX
};

#define wifi_100_width 14
#define wifi_100_height 11
const unsigned char wifi_100_bits[] = {
    0xf8, 0xc7, 0xfe, 0xdf, 0x07, 0xf8, 0xf1, 0xe3, 0xfc, 0xcf, 0x0c,
    0xcc, 0xe0, 0xc1, 0xf0, 0xc3, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0};

#define wifi_75_width 14
#define wifi_75_height 11
const unsigned char wifi_75_bits[] = {
    0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0xf0, 0xc3, 0xfc, 0xcf, 0x0c,
    0xcc, 0xe0, 0xc1, 0xf0, 0xc3, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0};

#define wifi_50_width 14
#define wifi_50_height 11
const unsigned char wifi_50_bits[] = {
    0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00,
    0xc0, 0xe0, 0xc1, 0xf0, 0xc3, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0};

#define wifi_25_width 14
#define wifi_25_height 11
const unsigned char wifi_25_bits[] = {
    0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00,
    0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0};

#define wifi_issue_width 14
#define wifi_issue_height 11
const unsigned char wifi_issue_bits[] = {
    0xd8, 0xc6, 0xde, 0xde, 0xc7, 0xf8, 0xd1, 0xe2, 0xdc, 0xce, 0xcc,
    0xcc, 0xc0, 0xc0, 0xd0, 0xc2, 0x00, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0};

const xbm_icon_t wifi_icons[] = {
  [WIFI_ICON_ISSUE] = {.width = wifi_issue_width, .height = wifi_issue_height, .icon = wifi_issue_bits},
  [WIFI_ICON_25] = {.width = wifi_25_width, .height = wifi_25_height, .icon = wifi_25_bits},
  [WIFI_ICON_50] = {.width = wifi_50_width, .height = wifi_50_height, .icon = wifi_50_bits},
  [WIFI_ICON_75] = {.width = wifi_75_width, .height = wifi_75_height, .icon = wifi_75_bits},
  [WIFI_ICON_100] = {.width = wifi_100_width, .height = wifi_100_height, .icon = wifi_100_bits},
};

/** Cloud ICON */
enum
{
  CLOUD_ICON_NORMAL,
  CLOUD_ICON_ISSUE,
  CLOUD_ICON_MAX, 
};

#define cloud_issue_width 14
#define cloud_issue_height 11
const unsigned char cloud_issue_bits[] = {
    0x70, 0xc0, 0x88, 0xc0, 0x04, 0xc1, 0x04, 0xcf, 0x02, 0xd0, 0x01,
    0xe0, 0x01, 0xe0, 0x01, 0xe0, 0xa2, 0xd0, 0x4c, 0xce, 0xa0, 0xc0};

#define cloud_normal_width 14
#define cloud_normal_height 11
const unsigned char cloud_normal_bits[] = {
    0x70, 0xc0, 0x88, 0xc0, 0x04, 0xc1, 0x04, 0xcf, 0x02, 0xd0, 0x01,
    0xe0, 0x01, 0xe0, 0x01, 0xe0, 0x02, 0xd0, 0xfc, 0xcf, 0x00, 0xc0};

const xbm_icon_t cloud_icons[] = {
  [CLOUD_ICON_NORMAL] = {.width = cloud_normal_width, .height = cloud_normal_height, .icon = cloud_normal_bits},
  [CLOUD_ICON_ISSUE] = {.width = cloud_issue_width, .height = cloud_issue_height, .icon = cloud_issue_bits},
};

/** Cast U8G2 */
#define DISP() ((U8G2_SH1106_128X64_NONAME_F_HW_I2C *)(this->u8g2))

/**
 * @brief Show dashboard temperature and humdity
 * 
 * @param hasStatus 
 */
void OledDisplay::showTempHum(bool hasStatus) {
  char buf[10];
  if (value.Temperature > -1001) {
    if (config.isTemperatureUnitInF()) {
      float tempF = (value.Temperature * 9) / 5 + 32;
      if (hasStatus) {
        snprintf(buf, sizeof(buf), "%0.1f", tempF);
      } else {
        snprintf(buf, sizeof(buf), "%0.1f°F", tempF);
      }
    } else {
      if (hasStatus) {
        snprintf(buf, sizeof(buf), "%.1f", value.Temperature);
      } else {
        snprintf(buf, sizeof(buf), "%.1f°C", value.Temperature);
      }
    }
  } else {
    if (config.isTemperatureUnitInF()) {
      snprintf(buf, sizeof(buf), "-°F");
    } else {
      snprintf(buf, sizeof(buf), "-°C");
    }
  }
  DISP()->drawUTF8(1, 10, buf);

  /** Show humidty */
  if (value.Humidity >= 0) {
    snprintf(buf, sizeof(buf), "%d%%", value.Humidity);
  } else {
    snprintf(buf, sizeof(buf), "%-%%");
  }

  if (value.Humidity > 99) {
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

void OledDisplay::showIcon(int x, int y, const void* ico) {
  xbm_icon_t* icon = (xbm_icon_t*)ico;
  DISP()->drawXBM(x, y, icon->width, icon->height, icon->icon);
}

/**
 * @brief Construct a new Ag Oled Display:: Ag Oled Display object
 * 
 * @param config AgConfiguration
 * @param value Measurements
 * @param log Serial Stream
 */
OledDisplay::OledDisplay(Configuration &config, Measurements &value, Stream &log)
    : PrintLog(log, "OledDisplay"), config(config), value(value){}

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

  /** Show low brightness on startup. then it's completely turn off on main
   * application */
  int brightness = config.getDisplayBrightness();
  if(brightness == 0) {
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

  /** Free u8g2 */
  delete DISP();
  u8g2 = NULL;

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

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    DISP()->drawStr(1, 10, line1);
    DISP()->drawStr(1, 30, line2);
    DISP()->drawStr(1, 50, line3);
  } while (DISP()->nextPage());
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

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    DISP()->drawStr(1, 10, line1);
    DISP()->drawStr(1, 25, line2);
    DISP()->drawStr(1, 40, line3);
    DISP()->drawStr(1, 55, line4);
  } while (DISP()->nextPage());
}

/**
 * @brief Update dashboard content
 * 
 */
void OledDisplay::showDashboard(void) { showDashboard(DashBoardStatusNone); }

/**
 * @brief Update dashboard content and error status
 * 
 */
void OledDisplay::showDashboard(OledDisplay::DashboardStatus status) {
  if (isDisplayOff) {
    return;
  }

  char strBuf[10];
  const int icon_pos_x = 64;

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    switch (status) {
    case DashBoardStatusNone: {
      int rssi = WiFi.RSSI();
      int icon_type = WIFI_ICON_25;
      if (rssi > -40) {
        icon_type = WIFI_ICON_100;
      } else if (rssi > -67) {
        icon_type = WIFI_ICON_75;
      } else if (rssi > -70) {
        icon_type = WIFI_ICON_50;
      } else {
        icon_type = WIFI_ICON_25;
      }
      showIcon(icon_pos_x, 0, &wifi_icons[icon_type]);
      showTempHum(false);
      break;
    }
    case DashBoardStatusWiFiIssue: {
      showIcon(icon_pos_x, 0, &wifi_icons[WIFI_ICON_ISSUE]);
      showTempHum(false);
      break;
    }
    case DashBoardStatusServerIssue: {
      showIcon(icon_pos_x, 0, &cloud_icons[CLOUD_ICON_ISSUE]);
      showTempHum(false);
      break;
    }
    case DashBoardStatusAddToDashboard: {
      setCentralText(10, "Add To Dashboard");
      break;
    }
    case DashBoardStatusDeviceId: {
      setCentralText(10, ag->deviceId().c_str());
    }
    default:
      break;
    }

    /** Draw horizonal line */
    DISP()->drawLine(1, 13, 128, 13);

    /** Show CO2 label */
    DISP()->setFont(u8g2_font_t0_12_tf);
    DISP()->drawUTF8(1, 27, "CO2");

    DISP()->setFont(u8g2_font_t0_22b_tf);
    if (value.CO2 > 0) {
      int val = 9999;
      if (value.CO2 < 10000) {
        val = value.CO2;
      }
      sprintf(strBuf, "%d", val);
    } else {
      sprintf(strBuf, "%s", "-");
    }
    DISP()->drawStr(1, 48, strBuf);

    /** Show CO2 value index */
    DISP()->setFont(u8g2_font_t0_12_tf);
    DISP()->drawStr(1, 61, "ppm");

    /** Draw vertical line */
    DISP()->drawLine(45, 14, 45, 64);
    DISP()->drawLine(82, 14, 82, 64);

    /** Draw PM2.5 label */
    DISP()->setFont(u8g2_font_t0_12_tf);
    DISP()->drawStr(48, 27, "PM2.5");

    /** Draw PM2.5 value */
    DISP()->setFont(u8g2_font_t0_22b_tf);
    if (config.isPmStandardInUSAQI()) {
      if (value.pm25_1 >= 0) {
        sprintf(strBuf, "%d", ag->pms5003.convertPm25ToUsAqi(value.pm25_1));
      } else {
        sprintf(strBuf, "%s", "-");
      }
      DISP()->drawStr(48, 48, strBuf);
      DISP()->setFont(u8g2_font_t0_12_tf);
      DISP()->drawUTF8(48, 61, "AQI");
    } else {
      if (value.pm25_1 >= 0) {
        sprintf(strBuf, "%d", value.pm25_1);
      } else {
        sprintf(strBuf, "%s", "-");
      }
      DISP()->drawStr(48, 48, strBuf);
      DISP()->setFont(u8g2_font_t0_12_tf);
      DISP()->drawUTF8(48, 61, "ug/m³");
    }

    /** Draw tvocIndexlabel */
    DISP()->setFont(u8g2_font_t0_12_tf);
    DISP()->drawStr(85, 27, "tvoc:");

    /** Draw tvocIndexvalue */
    if (value.TVOC >= 0) {
      sprintf(strBuf, "%d", value.TVOC);
    } else {
      sprintf(strBuf, "%s", "-");
    }
    DISP()->drawStr(85, 39, strBuf);

    /** Draw NOx label */
    DISP()->drawStr(85, 53, "NOx:");
    if (value.NOx >= 0) {
      sprintf(strBuf, "%d", value.NOx);
    } else {
      sprintf(strBuf, "%s", "-");
    }
    DISP()->drawStr(85, 63, strBuf);
  } while (DISP()->nextPage());
}

void OledDisplay::setBrightness(int percent) {
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
}

void OledDisplay::showNewFirmwareVersion(String version) {
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

void OledDisplay::showNewFirmwareUpdating(String percent) {
  if (isDisplayOff) {
    return;
  }

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    setCentralText(20, "Firmware Update");
    setCentralText(50, String("Updating... ") + percent + String("%"));
  } while (DISP()->nextPage());
}

void OledDisplay::showNewFirmwareSuccess(String count) {
  if (isDisplayOff) {
    return;
  }

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    setCentralText(20, "Firmware Update");
    setCentralText(40, "Success");
    setCentralText(60, String("Rebooting... ") + count);
  } while (DISP()->nextPage());
}

void OledDisplay::showNewFirmwareFailed(void) {
  if (isDisplayOff) {
    return;
  }

  DISP()->firstPage();
  do {
    DISP()->setFont(u8g2_font_t0_16_tf);
    setCentralText(20, "Firmware Update");
    setCentralText(40, "Failed");
    setCentralText(60, String("Retry after 24h"));
  } while (DISP()->nextPage());
}
