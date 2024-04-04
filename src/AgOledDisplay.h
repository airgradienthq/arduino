#ifndef _AG_OLED_DISPLAY_H_
#define _AG_OLED_DISPLAY_H_

#include "AgConfigure.h"
#include "AgValue.h"
#include "AirGradient.h"
#include "Main/PrintLog.h"
#include <Arduino.h>

class AgOledDisplay : public PrintLog {
private:
  AgConfigure &config;
  AirGradient *ag;
  bool isBegin = false;
  void *u8g2 = NULL;
  AgValue &value;

  void showTempHum(bool hasStatus);
public:
  AgOledDisplay(AgConfigure &config, AgValue &value,
                Stream &log);
  ~AgOledDisplay();

  void setAirGradient(AirGradient *ag);
  bool begin(void); 
  void end(void);
  void setStatus(String &status);
  void setStatus(const char *status);
  void setText(String &line1, String &line2, String &line3);
  void setText(const char *line1, const char *line2, const char *line3);
  void setText(const char *text);
  void setText(String &text);
  void setText(String &line1, String &line2, String &line3, String &line4);
  void setText(const char *line1, const char *line2, const char *line3,
               const char *line4);
  void showDashboard(void);
  void showDashboard(const char *status);
};

#endif /** _AG_OLED_DISPLAY_H_ */
