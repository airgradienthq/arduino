#include "HardwareWatchdog.h"

HardwareWatchdog::HardwareWatchdog(BoardType type) : boardType(type) {}

#if defined(ESP8266)
void HardwareWatchdog::begin(Stream &debugStream) {
  this->_debugStream = &debugStream;
  this->begin();
}
#else
#endif

/**
 * @brief Initialize external watchdog
 *
 */
void HardwareWatchdog::begin(void) {
  if (this->_isInit) {
    return;
  }

  /** Get BSP */
  this->bsp = getBoardDef(this->boardType);
  if ((this->bsp == nullptr) || (this->bsp->WDG.supported == false)) {
    AgLog("Board not supported Watchdog");
    return;
  }

  /** Init GPIO and first feed external watchdog */
  pinMode(this->bsp->WDG.resetPin, OUTPUT);
  this->_feed();

  this->_isInit = true;
  AgLog("Inittialized");
}

/**
 * @brief Reset Watchdog
 *
 */
void HardwareWatchdog::reset(void) {
  if (this->isInitInvalid()) {
    return;
  }

  this->_feed();
}

/**
 * @brief Wathdog timeout
 *
 * @return int Millisecionds
 */
int HardwareWatchdog::getTimeout(void) { return 5 * 1000 * 60; }

bool HardwareWatchdog::isInitInvalid(void) {
  if (this->_isInit == false) {
    AgLog("No-initialized");
    return true;
  }
  return false;
}

/**
 * @brief Reset external watchdog
 *
 */
void HardwareWatchdog::_feed(void) {
  digitalWrite(this->bsp->WDG.resetPin, HIGH);
  delay(25);
  digitalWrite(this->bsp->WDG.resetPin, LOW);

  AgLog("Reset");
}
