#ifndef _AIR_GRADIENT_H_
#define _AIR_GRADIENT_H_

#include "Display/Display.h"
#include "Main/BoardDef.h"
#include "Main/HardwareWatchdog.h"
#include "Main/LedBar.h"
#include "Main/PushButton.h"
#include "Main/StatusLed.h"
#include "PMS/PMS5003.h"
#include "PMS/PMS5003T.h"
#include "S8/S8.h"
#include "Sgp41/Sgp41.h"
#include "Sht/Sht.h"
#include "Main/utils.h"

#ifndef GIT_VERSION
#define GIT_VERSION "3.1.9-snap"
#endif

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
   * @brief Get the Board Name object
   *
   * @return String
   */
  String getBoardName(void);

  /**
   * @brief Round double value with for 2 decimal
   *
   * @param valuem Round value
   * @return double
   */
  double round2(double value);

  /**
   * @brief Check that Airgradient object is ONE_INDOOR
   *
   * @return true Yes
   * @return false No
   */
  bool isOne(void);

  /**
   * @brief Check that Airgradient object is OPEN_AIR
   * 
   * @return true 
   * @return false 
   */
  bool isOpenAir(void);

  /**
   * @brief Check that Airgradient object is DIY_PRO 4.2 indoor
   *
   * @return true Yes
   * @return false No
   */
  bool isPro4_2(void);
  /**
   * @brief Check that Airgradient object is DIY_PRO 3.7 indoor
   *
   * @return true Yes
   * @return false No
   */
  bool isPro3_3(void);

  /**
   * @brief Check that Airgradient object is DIY_BASIC
   *
   * @return true Yes
   * @return false No
   */
  bool isBasic(void);

  /**
   * @brief Get device Id
   *
   * @return String
   */
  String deviceId(void);

private:
  BoardType boardType;
};

#endif /** _AIR_GRADIENT_H_ */
