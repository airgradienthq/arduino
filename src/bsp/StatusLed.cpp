#include "StatusLed.h"

StatusLed::StatusLed(BoardType boardType) : boardType(boardType) {}

#if defined(ESP8266)
void StatusLed::begin(Stream &debugStream) {
  this->_debugStream = &debugStream;
  this->begin();
}
#else
#endif

/**
 * @brief Initialized LED
 *
 */
void StatusLed::begin(void) {
  bsp = getBoardDef(this->boardType);
  if ((bsp == nullptr) || (bsp->LED.supported == false)) {
    AgLog("Board not support StatusLed");
    return;
  }

  pinMode(bsp->LED.pin, OUTPUT);
  digitalWrite(bsp->LED.pin, !bsp->LED.onState);

  this->state = LED_OFF;
  this->isInit = true;

  AgLog("Init");
}

/**
 * @brief Turn LED on
 *
 */
void StatusLed::setOn(void) {
  if (this->checkInit() == false) {
    return;
  }
  digitalWrite(bsp->LED.pin, bsp->LED.onState);
  this->state = LED_ON;
  AgLog("Turn ON");
}

/**
 * @brief Turn LED off
 *
 */
void StatusLed::setOff(void) {
  if (this->checkInit() == false) {
    return;
  }
  digitalWrite(bsp->LED.pin, !bsp->LED.onState);
  this->state = LED_OFF;
  AgLog("Turn OFF");
}

/**
 * @brief Set LED toggle
 *
 */
void StatusLed::setToggle(void) {
  if (this->state == LED_ON) {
    this->setOff();
  } else {
    this->setOn();
  }
}

/**
 * @brief Get current LED state
 *
 * @return StatusLed::State
 */
StatusLed::State StatusLed::getState(void) { return this->state; }

/**
 * @brief Convert LED state to string
 *
 * @param state LED state
 * @return String
 */
String StatusLed::toString(StatusLed::State state) {
  if (state == LED_ON) {
    return "On";
  }
  return "Off";
}

bool StatusLed::checkInit(void) {
  if (this->isInit == false) {
    AgLog("No-Initialized");
    return false;
  }

  return true;
}
