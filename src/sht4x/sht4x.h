#ifndef _AIR_GRADIENT_SHT_H_
#define _AIR_GRADIENT_SHT_H_

#include <Arduino.h>
#include <Wire.h>

#include "../main/BoardDef.h"

/**
 * @brief The class with define how to handle the Sensirion sensor SHT41
 * (temperature and humidity sensor).
 */
class Sht {
public:
#if defined(ESP8266)
  bool begin(TwoWire &wire, Stream &debugStream);
#else
#endif
  Sht(BoardType type);
  bool begin(TwoWire &wire);
  void end(void);
  float getTemperature(void);
  float getRelativeHumidity(void);

private:
  BoardType _boardType;
  bool _isBegin = false; /** Flag indicate that sensor initialized or not */
  void *_sensor;
  const BoardDef *_bsp = NULL;
#if defined(ESP8266)
  Stream *_debugStream = nullptr;
  const char *TAG = "SHT4x";
#else
#endif
  bool isBegin(void);
  bool boardSupported(void);
  bool measureMediumPrecision(float &temperature, float &humidity);
};

#endif /** _AIR_GRADIENT_SHT_H_ */
