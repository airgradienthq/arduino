#ifndef _AIR_GRADIENT_H_
#define _AIR_GRADIENT_H_

#include "display/oled.h"
#include "main/BoardDef.h"
#include "main/HardwareWatchdog.h"
#include "main/LedBar.h"
#include "main/PushButton.h"
#include "main/StatusLed.h"
#include "pms/pms5003.h"
#include "pms/pms5003t.h"
#include "s8/s8.h"
#include "sgp41/sgp41.h"
#include "sht/sht.h"

/**
 * @brief Class with define all the sensor has supported by Airgradient. Each
 * sensor usage must be init before use.
 */
class AirGradient {
public:
  AirGradient(BoardType type);

  /**
   * @brief Plantower PMS5003 sensor
   */
  PMS5003 pms5003;
  /**
   * @brief Plantower PMS5003T sensor: connect to PM1 connector on
   * OPEN_AIR_OUTDOOR.
   */
  PMS5003T pms5003t_1;
  /**
   * @brief Plantower PMS5003T sensor: connect to PM2 connector on
   * OPEN_AIR_OUTDOOR.
   */
  PMS5003T pms5003t_2;

  /**
   * @brief SenseAirS8 CO2 sensor
   */
  S8 s8;

  /**
   * @brief Temperature and humidity sensor supported SHT3x and SHT4x
   *
   */
  Sht sht;

  /**
   * @brief  SGP41 TVOC and NOx sensor
   *
   */
  Sgp41 sgp41;

  /**
   * @brief OLED Display
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

  /**
   * @brief RGB LED array
   *
   */
  LedBar ledBar;

  /**
   * @brief External hardware watchdog
   */
  HardwareWatchdog watchdog;

  /**
   * @brief Get I2C SDA pin has of board supported
   *
   * @return int Pin number if -1 invalid
   */
  int getI2cSdaPin(void);
  /**
   * @brief Get I2C SCL pin has of board supported
   *
   * @return int Pin number if -1 invalid
   */
  int getI2cSclPin(void);

  /**
   * @brief Get the Board Type
   *
   * @return BoardType @ref BoardType
   */
  BoardType getBoardType(void);

  /**
   * @brief Get the library version string
   *
   * @return String
   */
  String getVersion(void);

  /**
   * @brief Round double value with for 2 decimal
   * 
   * @param valuem Round value
   * @return double 
   */
  double round2(double value);

private:
  BoardType boardType;
};

#endif /** _AIR_GRADIENT_H_ */
