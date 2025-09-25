#ifndef _STATUS_LED_H_
#define _STATUS_LED_H_

#include "BoardDef.h"
#include <Arduino.h>

/**
 * @brief The class define how to handle the LED
 *
 */
class StatusLed {
public:
  enum State {
    LED_OFF,
    LED_ON,
  };

  StatusLed(BoardType boardType);
#if defined(ESP8266)
  void begin(Stream &debugStream);
#else
#endif
  void begin(void);
  void end(void);
  void setOn(void);
  void setOff(void);
  void setToggle(void);
  State getState(void);
  String toString(StatusLed::State state);

private:
  const BoardDef *bsp = nullptr;
  BoardType boardType;
  bool _isBegin = false;
  State state;
#if defined(ESP8266)
  Stream *_debugStream;
  const char *TAG = "StatusLed";
#else
#endif

  bool isBegin(void);
};

#endif /** _STATUS_LED_H_ */
