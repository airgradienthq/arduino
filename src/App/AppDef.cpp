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
  case FW_MODE_I_8PSL:
    return "I-8PSL";
  default:
    break;
  }
  return "UNKNOWN";
}
