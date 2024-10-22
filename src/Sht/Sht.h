#ifndef _SHT_H_
#define _SHT_H_

#include "../Main/BoardDef.h"
#include <Arduino.h>
#include <Wire.h>

/**
 * @brief This class with define how to handlet sensirion sensor sht4x and
 * sht3x(Temperature and humidity sensor)
 *
 */
class Sht {
private:
  const int measureFailedTimeout = 30 * 1000; /** ms */
  BoardType _boardType;
  bool _isBegin = false;
  void *_sensor;
  const BoardDef *_bsp = NULL;

  /** Hold the last call "measure" method by millis() */
  unsigned int lastMeasureFailed = 0;
  bool measureFailed = false;
#if defined(ESP8266)
  Stream *_debugStream = nullptr;
  const char *TAG = "SHT";
#else
#endif
  bool isBegin(void);
  bool boardSupported(void);

public:
  Sht(BoardType type);
  ~Sht();

#if defined(ESP8266)
  bool begin(TwoWire &wire, Stream &debugStream);
#else
#endif
  bool begin(TwoWire &wire);
  void end(void);
  bool measure(void);
  float getTemperature(void);
  float getRelativeHumidity(void);
};

#endif /** _SHT_H_ */
