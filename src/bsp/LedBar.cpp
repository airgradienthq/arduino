#include "LedBar.h"

#include "../library/Adafruit_NeoPixel/Adafruit_NeoPixel.h"

#define pixel() ((Adafruit_NeoPixel *)this->pixels)

#if defined(ESP8266)
void LedBar::begin(Stream &debugStream) {
  this->_debugStream = &debugStream;
  this->begin();
}
#else
#endif
LedBar::LedBar(BoardType type) : _boardType(type) {}

/**
 * @brief LED bar initialize
 *
 */
void LedBar::begin(void) {
  if (this->_isInit) {
    return;
  }

  /** Get board support package define */
  this->_bsp = getBoardDef(this->_boardType);
  if ((this->_bsp == nullptr) || (this->_bsp->LED.supported == false) ||
      (this->_bsp->LED.rgbNum == 0)) {
    AgLog("Board Not supported or LED not available on board");
    return;
  }

  /** Init pixels */
  this->pixels = new Adafruit_NeoPixel(
      this->_bsp->LED.rgbNum, this->_bsp->LED.pin, NEO_GRB + NEO_KHZ800);
  pixel()->begin();
  pixel()->clear();

  this->_isInit = true;

  AgLog("Init");
}

/**
 * @brief Set LED color, if LED is on the color update immedietly. Otherwise
 * must setOn to show LED color
 *
 * @param red Color Red (0 - 255)
 * @param green Color Green (0 - 255)
 * @param blue Color Blue (0 - 255)
 * @param ledNum Index of LED from 0 to getNumberOfLed() - 1
 */
void LedBar::setColor(uint8_t red, uint8_t green, uint8_t blue, int ledNum) {
  if (this->ledNumInvalid(ledNum)) {
    return;
  }

  pixel()->setPixelColor(ledNum, red, green, blue);
}

/**
 * @brief Set LED brightness apply for all LED bar
 *
 * @param brightness Brightness (0 - 255)
 */
void LedBar::setBrighness(uint8_t brightness) {
  if (this->checkInit() == false) {
    return;
  }
  pixel()->setBrightness(brightness);
}

/**
 * @brief Get number of LED on bar
 *
 * @return int Number of LED
 */
int LedBar::getNumberOfLed(void) {
  if (this->checkInit() == false) {
    return 0;
  }

  return this->_bsp->LED.rgbNum;
}

bool LedBar::checkInit(void) {
  if (this->_isInit) {
    return true;
  }
  AgLog("LED is not initialized");
  return false;
}

bool LedBar::ledNumInvalid(int ledNum) {
  if (this->checkInit() == false) {
    return true;
  }

  if ((ledNum < 0) || (ledNum >= this->_bsp->LED.rgbNum)) {
    AgLog("ledNum invalid: %d", ledNum);
    return true;
  }
  return false;
}

void LedBar::setColor(uint8_t red, uint8_t green, uint8_t blue) {
  for (int ledNum = 0; ledNum < this->_bsp->LED.rgbNum; ledNum++) {
    this->setColor(red, green, blue, ledNum);
  }
}

/**
 * @brief Call to turn LED on/off base on the setting color
 * 
 */
void LedBar::show(void) {
  if (pixel()->canShow()) {
    pixel()->show();
  }
}

/**
 * @brief Set all LED to off color (r,g,b) = (0,0,0)
 * 
 */
void LedBar::clear(void) { pixel()->clear(); }
