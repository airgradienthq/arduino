#ifndef _AIR_GRADIENT_LED_H_
#define _AIR_GRADIENT_LED_H_

#include <Arduino.h>

#include "BoardDef.h"

/**
 * @brief The class define how to handle the RGB LED bar
 *
 */
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
  int getNumberOfLeds(void);
  void show(void);
  void clear(void);
  void setEnable(bool enable);
  bool isEnabled(void);

private:
  bool enabled = true;
  const BoardDef *_bsp;
  bool _isBegin = false;
  uint8_t _ledState = 0;
  BoardType _boardType;
  void *pixels = nullptr;
#if defined(ESP8266)
  Stream *_debugStream = NULL;
  const char *TAG = "LED";
#else
#endif
  bool isBegin(void);
  bool ledNumInvalid(int ledNum);
};

#endif /** _AIR_GRADIENT_LED_H_ */
