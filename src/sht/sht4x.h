#ifndef _AIR_GRADIENT_SHT_H_
#define _AIR_GRADIENT_SHT_H_

#include <Arduino.h>
#include <Wire.h>

#include "../bsp/BoardDef.h"

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
  bool _isInit = false;
  void *_sensor;
  const BoardDef *_bsp = NULL;
#if defined(ESP8266)
  Stream *_debugStream = nullptr;
  const char *TAG = "SHT4x";
#else
#endif
  bool checkInit(void);
  bool boardSupported(void);
  int sdaPin(void);
  int sclPin(void);

  bool measureHighPrecision(float &temperature, float &humidity);
  bool measureMediumPrecision(float &temperature, float &humidity);
  bool measureLowestPrecision(float &temperature, float &humidity);

  bool activateHighestHeaterPowerShort(float &temperature, float &humidity);
  bool activateMediumHeaterPowerLong(float &temperature, float &humidity);
  bool activateLowestHeaterPowerLong(float &temperature, float &humidity);

  bool getSerialNumber(uint32_t &serialNumber);
  bool softReset(void);
};

#endif /** _AIR_GRADIENT_SHT_H_ */
