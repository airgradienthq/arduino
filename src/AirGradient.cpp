#include "AirGradient.h"
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else 
#include "WiFi.h"
#endif

AirGradient::AirGradient(BoardType type)
    : pms5003(type), pms5003t_1(type), pms5003t_2(type), s8(type), sgp41(type),
      display(type), boardType(type), button(type), statusLed(type),
      ledBar(type), watchdog(type), sht(type) {}

/**
 * @brief Get pin number for I2C SDA
 *
 * @return int -1 if invalid
 */
int AirGradient::getI2cSdaPin(void) {
  const BoardDef *bsp = getBoardDef(this->boardType);
  if ((bsp == nullptr) || (bsp->I2C.supported == false)) {
    return -1;
  }
  return bsp->I2C.sda_pin;
}

/**
 * @brief Get pin number for I2C SCL
 *
 * @return int -1 if invalid
 */
int AirGradient::getI2cSclPin(void) {
  const BoardDef *bsp = getBoardDef(this->boardType);
  if ((bsp == nullptr) || (bsp->I2C.supported == false)) {
    return -1;
  }
  return bsp->I2C.scl_pin;
}

String AirGradient::getVersion(void) { return GIT_VERSION; }

BoardType AirGradient::getBoardType(void) { return boardType; }

double AirGradient::round2(double value) {
  return (int)(value * 100 + 0.5) / 100.0;
}

String AirGradient::getBoardName(void) {
  return String(getBoardDefName(boardType));
}

/**
 * @brief Board Type is ONE_INDOOR
 * 
 * @return true ONE_INDOOR
 * @return false Other
 */
bool AirGradient::isOne(void) {
  return boardType == BoardType::ONE_INDOOR;
}

bool AirGradient::isPro4_2(void) {
  return boardType == BoardType::DIY_PRO_INDOOR_V4_2;
}

bool AirGradient::isPro3_7(void) { 
  return boardType == BoardType::DIY_PRO_INDOOR_V3_7;
}

String AirGradient::deviceId(void) {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}
