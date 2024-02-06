#ifndef _AIR_GRADIENT_SHT3X_H_
#define _AIR_GRADIENT_SHT3X_H_

#include "../main/BoardDef.h"
#include <Arduino.h>
#include <Wire.h>

/**
 * @brief The class with define how to handle the Sensirion sensor SHT3x
 * (temperature and humidity sensor).
 */
class Sht3x {
private:
  BoardType _boarType;
  bool _isBegin = false;
  const BoardDef *_bsp = NULL;
  void *_sensor;
#ifdef ESP8266
  Stream *_debugStream = nullptr;
  const char *TAG = "SHT3x";
#else
#endif

  bool isBegin(void);
  bool boardSupported(void);
  bool measure(float &temp, float &hum);

public:
  Sht3x(BoardType type);
  ~Sht3x();

#ifdef ESP8266
  bool begin(TwoWire &wire, Stream &debugStream);
#else
#endif
  bool begin(TwoWire &wire);
  void end(void);
  float getTemperature(void);
  float getRelativeHumidity(void);
};

#endif /** _AIR_GRADIENT_SHT3X_H_ */
