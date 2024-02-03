#ifndef _AIR_GRADIENT_H_
#define _AIR_GRADIENT_H_

#include "bsp/BoardDef.h"
#include "bsp/LedBar.h"
#include "bsp/PushButton.h"
#include "bsp/StatusLed.h"
#include "bsp/HardwareWatchdog.h"
#include "co2/s8.h"
#include "display/oled.h"
#include "pm/pms5003.h"
#include "pm/pms5003t.h"
#include "sgp/sgp41.h"
#include "sht/sht4x.h"

class AirGradient {
public:
  AirGradient(BoardType type);

  /**
   * @brief Plantower PMS5003 sensor
   */
  PMS5003 pms5003;
  PMS5003T pms5003t_1;
  PMS5003T pms5003t_2;

  /**
   * @brief SenseAirS8 CO2 sensor
   */
  S8 s8;

  /**
   * @brief Temperature and humidity sensor
   */
  Sht sht;

  /**
   * @brief TVOC and NOx sensor
   *
   */
  Sgp41 sgp41;

  /**
   * @brief Display
   *
   */
  Display display;

  /**
   * @brief Push Button
   */
  PushButton button;

  /**
   * @brief LED
   */
  StatusLed statusLed;
  LedBar ledBar;

  /**
   * @brief Hardware watchdog
   */
  HardwareWatchdog watchdog;

  int getI2cSdaPin(void);
  int getI2cSclPin(void);

  BoardType getBoardType(void);

  String getVersion(void);

private:
  BoardType boardType;
};

#endif /** _AIR_GRADIENT_H_ */
