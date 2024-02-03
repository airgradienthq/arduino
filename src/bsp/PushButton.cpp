#include "PushButton.h"

PushButton::PushButton(BoardType type) : _boardType(type) {}

#if defined(ESP8266)
void PushButton::begin(Stream &debugStream) {
  this->_debugStream = &debugStream;
  this->begin();
}
#else
#endif

/**
 * @brief Initialize PushButton, If PushButton is not initialized the get state
 *
 */
void PushButton::begin(void) {
  if (this->_isInit) {
    return;
  }

  this->_bsp = getBoardDef(this->_boardType);
  if ((this->_bsp == nullptr) || (this->_bsp->SW.supported == false)) {
    AgLog("Board not supported or switch not available");
    return;
  }

  if (this->_boardType == BOARD_DIY_PRO_INDOOR_V4_2) {
    pinMode(this->_bsp->SW.pin, INPUT_PULLUP);
  } else {
    pinMode(this->_bsp->SW.pin, INPUT);
  }

  this->_isInit = true;
  AgLog("Init");
}

/**
 * @brief Get button state, Alway retrun State::BUTTON_RELEASED if no-initialize
 *
 * @return PushButton::State
 */
PushButton::State PushButton::getState(void) {
  if (this->checkInit() == false) {
    return State::BUTTON_RELEASED;
  }

  if (digitalRead(this->_bsp->SW.pin) == this->_bsp->SW.activeLevel) {
    return State::BUTTON_PRESSED;
  }
  return State::BUTTON_RELEASED;
}

/**
 * @brief Get PushButton::State as string
 *
 * @param state Buttons State
 * @return String
 */
String PushButton::toString(PushButton::State state) {
  if (state == BUTTON_PRESSED) {
    return "Presssed";
  }
  return "Released";
}

bool PushButton::checkInit(void) {
  if (this->_isInit) {
    return true;
  }
  AgLog("Switch not initialized");
  return false;
}
