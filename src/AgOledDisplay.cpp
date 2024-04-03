#include "AgOledDisplay.h"
#include "Libraries/U8g2/src/U8g2lib.h"

#define DISP() (U8G2_SH1106_128X64_NONAME_F_HW_I2C *)(this->u8g2)

AgOledDisplay::AgOledDisplay(AirGradient &ag, AgConfigure &config, Stream &log) : PrintLog(log, "AgOledDisplay"),
                                                                                  ag(ag), config(config)
{
}

AgOledDisplay::~AgOledDisplay()
{
}

/**
 * @brief Initialize display
 *
 * @return true Success
 * @return false Failure
 */
bool AgOledDisplay::begin(void)
{
  if (isBegin)
  {
    logWarning("Already begin, call 'end' and try again");
    return true;
  }

  /** Create u8g2 instance */
  u8g2 = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
  if (u8g2 == NULL)
  {
    logError("Create 'U8G2' failed");
    return false;
  }

  /** Init u8g2 */
  if (DISP()->begin() == false)
  {
    logError("U8G2 'begin' failed");
    return false;
  }

  isBegin = true;
  logInfo("begin");
  return true;
}

/**
 * @brief De-Initialize display
 *
 */
void AgOledDisplay::end(void)
{
  if (!isBegin)
  {
    logWarning("Already end, call 'begin' and try again");
    return;
  }

  /** Free u8g2 */
  delete u8g2;
  u8g2 = NULL;

  isBegin = false;
  logInfo("end");
}

void AgOledDisplay::setStatus(String &status)
{
  setStatus(status.c_str());
}

void AgOledDisplay::setStatus(const char *status)
{
}

void AgOledDisplay::setText(String &line1, String &line2, String &line3)
{
  setText(line1.c_str(), line2.c_str(), line3.c_str());
}

void AgOledDisplay::setText(const char *line1, const char *line2, const char *line3)
{
  DISP()->firstPage();
  DISP()->setFont(u8g2_font_t0_16_tf);
  DISP()->drawStr(1, 10, line1);
  DISP()->drawStr(1, 30, line2);
  DISP()->drawStr(1, 50, line3);
}

void AgOledDisplay::setText(const char *text)
{
}

void AgOledDisplay::setText(String &text)
{
}

void AgOledDisplay::setText(String &line1, String &line2, String &line3, String &line4)
{
  setText(line1.c_str(), line2.c_str(), line3.c_str(), line4.c_str());
}

void AgOledDisplay::setText(const char *line1, const char *line2, const char *line3, const char *line4)
{
  DISP()->firstPage();
  DISP()->setFont(u8g2_font_t0_16_tf);
  DISP()->drawStr(1, 10, line1);
  DISP()->drawStr(1, 25, line2);
  DISP()->drawStr(1, 40, line3);
  DISP()->drawStr(1, 55, line4);
}
