#ifndef _AIR_GRADIENT_LED_H_
#define _AIR_GRADIENT_LED_H_

#include <Arduino.h>

#include "BoardDef.h"

class LedBar {
public:
#if defined(ESP8266)
  void begin(Stream &debugStream);
#else
#endif
  LedBar(BoardType type);
  void begin(void);
  void setColor(uint8_t red, uint8_t green, uint8_t blue, int ledNum);
  void setColor(uint8_t red, uint8_t green, uint8_t blue);
  void setBrighness(uint8_t brightness);
  int getNumberOfLed(void);
  void show(void);
  void clear(void);

private:
  const BoardDef *_bsp;
  bool _isInit = false;
  uint8_t _ledState = 0;
  BoardType _boardType;
  void *pixels = nullptr;
#if defined(ESP8266)
  Stream *_debugStream = NULL;
  const char *TAG = "LED";
#else
#endif
  bool checkInit(void);
  bool ledNumInvalid(int ledNum);
};

#endif /** _AIR_GRADIENT_LED_H_ */
