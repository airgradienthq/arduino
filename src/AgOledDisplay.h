#ifndef _AG_OLED_DISPLAY_H_
#define _AG_OLED_DISPLAY_H_

#include "AgConfigure.h"
#include "AgValue.h"
#include "AirGradient.h"
#include "Main/PrintLog.h"
#include <Arduino.h>

class OledDisplay : public PrintLog {
private:
  Configuration &config;
  AirGradient *ag;
  bool isBegin = false;
  void *u8g2 = NULL;
  Measurements &value;

  void showTempHum(bool hasStatus);
public:
  OledDisplay(Configuration &config, Measurements &value,
                Stream &log);
  ~OledDisplay();

  void setAirGradient(AirGradient *ag);
  bool begin(void); 
  void end(void);
  void setText(String &line1, String &line2, String &line3);
  void setText(const char *line1, const char *line2, const char *line3);
  void setText(String &line1, String &line2, String &line3, String &line4);
  void setText(const char *line1, const char *line2, const char *line3,
               const char *line4);
  void showDashboard(void);
  void showDashboard(const char *status);
  void showWiFiQrCode(String content, String label);
  void setBrightness(int percent);
};

#endif /** _AG_OLED_DISPLAY_H_ */
