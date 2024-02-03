#ifndef _HARDWARE_WATCHDOG_H_
#define _HARDWARE_WATCHDOG_H_

#include <Arduino.h>

#include "BoardDef.h"

class HardwareWatchdog {
public:
  HardwareWatchdog(BoardType type);
#if defined(ESP8266)
  void begin(Stream &debugStream);
#else
#endif
  void begin(void);
  void reset(void);
  int getTimeout(void);

private:
  bool _isInit = false;
  const BoardDef *bsp;
  BoardType boardType;
#if defined(ESP8266)
  Stream *_debugStream;
  const char *TAG = "HardwareWatchdog";
#else
#endif

  bool isInitInvalid(void);
  void _feed(void);
};

#endif /** _HARDWARE_WATCHDOG_H_ */
