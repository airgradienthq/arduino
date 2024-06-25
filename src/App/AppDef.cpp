#include "AppDef.h"

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
  case FW_MODE_O_1PS:
    return "0-1PS";
  case FW_MODE_O_1P:
    return "O-1P";
  case FW_MODE_I_42PS:
    return "DIY-PRO-I-4.2PS";
  case FW_MODE_I_33PS:
    return "DIY-PRO-I-3.3PS";
  case FW_MODE_I_BASIC_40PS:
    return "DIY-BASIC-I-4.0PS";
  default:
    break;
  }
  return "UNKNOWN";
}
