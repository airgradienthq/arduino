#ifndef _AIR_GRADIENT_SW_H_
#define _AIR_GRADIENT_SW_H_

#include "BoardDef.h"
#include <Arduino.h>

class PushButton {
public:
  /**
   * @brief Enum button state
   */
  enum State { BUTTON_PRESSED, BUTTON_RELEASED };

#if defined(ESP8266)
  void begin(Stream &debugStream);
#else
#endif
  PushButton(BoardType type);
  void begin(void);
  State getState(void);
  String toString(State state);

private:
  /** BSP constant variable */
  const BoardDef *_bsp;
  /** Board type */
  BoardType _boardType;
  /** Is inititalize flag */
  bool _isInit = false;

  /** Special variable for ESP8266 */
#if defined(ESP8266)
  Stream *_debugStream = nullptr;
  const char *TAG = "PushButton";
#else
#endif

  /** Method */

  bool checkInit(void);
};

#endif /** _AIR_GRADIENT_SW_H_ */
