#include "Display.h"
#include "../Libraries/Adafruit_SH110x/Adafruit_SH110X.h"
#include "../Libraries/Adafruit_SSD1306_Wemos_OLED/Adafruit_SSD1306.h"

#define disp(func)                                                             \
  if (this->_boardType == DIY_BASIC) {                               \
    ((Adafruit_SSD1306 *)(this->oled))->func;                                  \
  } else {                                                                     \
    ((Adafruit_SH110X *)(this->oled))->func;                                   \
  }

#if defined(ESP8266)
void Display::begin(TwoWire &wire, Stream &debugStream) {
  this->_debugStream = &debugStream;
  this->begin(wire);
}
#else
#endif

Display::Display(BoardType type) : _boardType(type) {}

/**
 * @brief Initialize display, should be call this function before call of ther,
 * if not it's always return failure.
 *
 * @param wire TwoWire instance, Must be initialized
 */
void Display::begin(TwoWire &wire) {
  if (_isBegin) {
    AgLog("Initialized, call end() then try again");
    return;
  }

  this->_bsp = getBoardDef(this->_boardType);
  if ((this->_bsp == nullptr) || (this->_bsp->I2C.supported == false) ||
      (this->_bsp->OLED.supported == false)) {
    AgLog("Init failed: board not supported");
    return;
  }

  /** Init OLED */
  if (this->_boardType == DIY_BASIC) {
    AgLog("Init Adafruit_SSD1306");
    Adafruit_SSD1306 *_oled = new Adafruit_SSD1306();
    _oled->begin(wire, SSD1306_SWITCHCAPVCC, this->_bsp->OLED.addr);
    this->oled = _oled;
  } else {
    AgLog("Init Adafruit_SH1106G");
    Adafruit_SH1106G *_oled = new Adafruit_SH1106G(
        this->_bsp->OLED.width, this->_bsp->OLED.height, &wire);
    _oled->begin(this->_bsp->OLED.addr, false);
    this->oled = _oled;
  }

  this->_isBegin = true;
  disp(clearDisplay());
  AgLog("Initialize");
}

/**
 * @brief Clear display buffer
 *
 */
void Display::clear(void) {
  if (this->isBegin() == false) {
    return;
  }
  disp(clearDisplay());
}

/**
 * @brief Invert display color
 *
 * @param i 0: black, other is white
 */
void Display::invertDisplay(uint8_t i) {
  if (this->isBegin() == false) {
    return;
  }
  disp(invertDisplay(i));
}

/**
 * @brief Send display frame buffer to OLED
 *
 */
void Display::show() {
  if (this->isBegin() == false) {
    return;
  }
  disp(display());
}

/**
 * @brief Set display contract
 *
 * @param value Contract (0;255);
 */
void Display::setContrast(uint8_t value) {
  if (this->isBegin() == false) {
    return;
  }
  disp(setContrast(value));
}

/**
 * @brief Draw pixel into display frame buffer, call show to draw to
 * display(OLED)
 *
 * @param x X Position
 * @param y Y Position
 * @param color Color (0: black, other white)
 */
void Display::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if (this->isBegin() == false) {
    return;
  }
  disp(drawPixel(x, y, color));
}

/**
 * @brief Set text size, it's scale default font instead of point to multiple
 * font has define for special size
 *
 * @param size Size of text (default = 1)
 */
void Display::setTextSize(int size) {
  if (this->isBegin() == false) {
    return;
  }
  disp(setTextSize(size));
}

/**
 * @brief Move draw cursor into new position
 *
 * @param x X Position
 * @param y Y Position
 */
void Display::setCursor(int16_t x, int16_t y) {
  if (this->isBegin() == false) {
    return;
  }
  disp(setCursor(x, y));
}

/**
 * @brief Set Text Color
 *
 * @param color 0:black, 1: While
 */
void Display::setTextColor(uint16_t color) {
  if (this->isBegin() == false) {
    return;
  }
  disp(setTextColor(color));
}

/**
 * @brief Set text foreground color and background color
 *
 * @param foreGroundColor Text Color (foreground color)
 * @param backGroundColor Text background color
 */
void Display::setTextColor(uint16_t foreGroundColor, uint16_t backGroundColor) {
  if (this->isBegin() == false) {
    return;
  }
  disp(setTextColor(foreGroundColor, backGroundColor));
}

/**
 * @brief Draw text to display framebuffer, call show() to draw to display
 * (OLED)
 *
 * @param text String
 */
void Display::setText(String text) {
  if (this->isBegin() == false) {
    return;
  }
  disp(print(text));
}

/**
 * @brief Draw bitmap into display framebuffer, call show() to draw to display
 * (OLED)
 *
 * @param x X Position
 * @param y Y Position
 * @param bitmap Bitmap buffer
 * @param w Bitmap width
 * @param h Bitmap hight
 * @param color Bitmap color
 */
void Display::drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[],
                         int16_t w, int16_t h, uint16_t color) {
  if (this->isBegin() == false) {
    return;
  }
  disp(drawBitmap(x, y, bitmap, w, h, color));
}

/**
 * @brief Set text to display framebuffer, call show() to draw into to display
 * (OLED)
 *
 * @param text Character buffer
 */
void Display::setText(const char text[]) {
  if (this->isBegin() == false) {
    return;
  }
  disp(print(text));
}

/**
 * @brief Draw line to display framebuffer, call show() to draw to
 * display(OLED)
 *
 * @param x0 Start X position
 * @param y0 Start Y position
 * @param x1 End X Position
 * @param y1 End Y Position
 * @param color Color (0: black, otherwise white)
 */
void Display::drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
  if (this->isBegin() == false) {
    return;
  }
  disp(drawLine(x0, y0, x1, y1, color));
}

/**
 * @brief Draw circle to display framebuffer,
 *
 * @param x
 * @param y
 * @param r
 * @param color
 */
void Display::drawCircle(int x, int y, int r, uint16_t color) {
  if (this->isBegin() == false) {
    return;
  }
  disp(drawCircle(x, y, r, color));
}

void Display::drawRect(int x0, int y0, int x1, int y1, uint16_t color) {
  if (this->isBegin() == false) {
    return;
  }
  disp(drawRect(x0, y0, x1, y1, color));
}

bool Display::isBegin(void) {
  if (this->_isBegin) {
    return true;
  }
  AgLog("Display not-initialized");
  return false;
}

void Display::setRotation(uint8_t r) {
  if (isBegin() == false) {
    return;
  }
  disp(setRotation(r));
}

void Display::end(void) {
  if (this->_isBegin == false) {
    return;
  }
  _isBegin = false;
  if (this->_boardType == DIY_BASIC) {
    delete ((Adafruit_SSD1306 *)(this->oled));
  } else {
    delete ((Adafruit_SH110X *)(this->oled));
  }
  AgLog("De-initialize");
}
