#ifndef _AIR_GRADIENT_OLED_H_
#define _AIR_GRADIENT_OLED_H_

#include "../Main/BoardDef.h"
#include <Arduino.h>
#include <Wire.h>

/**
 * @brief The class define how to handle the OLED display on Airgradient has
 * attached or support OLED display like: ONE-V9, Basic-V4
 */
class Display {
public:
  const uint16_t COLOR_WHILTE = 1;
  const uint16_t COLOR_BLACK = 0;
#if defined(ESP8266)
  void begin(TwoWire &wire, Stream &debugStream);
#else
#endif
  Display(BoardType type);
  void begin(TwoWire &wire);
  void end(void);

  void clear(void);
  void invertDisplay(uint8_t i);
  void show();

  void setContrast(uint8_t value);
  void drawPixel(int16_t x, int16_t y, uint16_t color);
  void setTextSize(int size);
  void setCursor(int16_t x, int16_t y);
  void setTextColor(uint16_t color);
  void setTextColor(uint16_t foreGroundColor, uint16_t backGroundColor);
  void setText(String text);
  void setText(const char text[]);
  void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w,
                  int16_t h, uint16_t color);
  void drawLine(int x0, int y0, int x1, int y1, uint16_t color);
  void drawCircle(int x, int y, int r, uint16_t color);
  void drawRect(int x0, int y0, int x1, int y1, uint16_t color);
  void setRotation(uint8_t r);

private:
  BoardType _boardType;
  const BoardDef *_bsp = nullptr;
  void *oled;
  bool _isBegin = false;
#if defined(ESP8266)
  const char *TAG = "oled";
  Stream *_debugStream = nullptr;
#else

#endif
  bool isBegin(void);
};

#endif /** _AIR_GRADIENT_OLED_H_ */
