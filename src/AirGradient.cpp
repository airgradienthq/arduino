#include "AirGradient.h"

#define AG_LIB_VER "3.0.9"

const char *AgFirmwareModeName(AgFirmwareMode mode) {
  switch (mode) {
  case FW_MODE_I_9PSL:
    return "I-9PSL";
  case FW_MODE_O_1PP:
    return "O-1PP";
  case FW_MODE_O_1PPT:
    return "O-1PPT";
  case FW_MODE_O_1PST:
    return "O-1PST";
  case FW_MDOE_O_1PS:
    return "0-1PS";
  default:
    break;
  }
  return "UNKNOWN";
}

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

String AirGradient::getVersion(void) { return AG_LIB_VER; }

BoardType AirGradient::getBoardType(void) { return boardType; }

double AirGradient::round2(double value) {
  return (int)(value * 100 + 0.5) / 100.0;
}

String AirGradient::getBoardName(void) {
  return String(getBoardDefName(boardType));
}
