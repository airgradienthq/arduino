#ifndef _AIR_GRADIENT_SGP4X_H_
#define _AIR_GRADIENT_SGP4X_H_

#include "../Main/BoardDef.h"
#include <Arduino.h>
#include <Wire.h>

/**
 * @brief The class define how to handle Sensirion sensor SGP41 (VOC and NOx
 * sensor)
 *
 */
class Sgp41 {
public:
  Sgp41(BoardType type);
  bool begin(TwoWire &wire);
#if defined(ESP8266)
  bool begin(TwoWire &wire, Stream &stream);
  void handle(void);
#else
  void _handle(void);
#endif
  void end(void);
  int getTvocIndex(void);
  int getNoxIndex(void);
  int getTvocRaw(void);
  int getNoxRaw(void);
  void setCompensationTemperatureHumidity(float temp, float hum);
  void setNoxLearningOffset(int offset);
  void setTvocLearningOffset(int offset);
  int getNoxLearningOffset(void);
  int getTvocLearningOffset(void);

private:
  bool onConditioning = true;
  bool ready = false;
  bool _isBegin = false;
  uint8_t _handleFailCount = 0;
  void *_sensor;
  void *_vocAlgorithm;
  void *_noxAlgorithm;
  const BoardDef *bsp = nullptr;
  BoardType _boardType;
  uint16_t defaultRh = 0x8000;
  uint16_t defaultT = 0x6666;
  int tvoc = 0;
  int tvocRaw;
  int nox = 0;
  int noxRaw;

  int noxLearnOffset;
  int tvocLearnOffset;

#if defined(ESP8266)
  uint32_t conditioningPeriod;
  uint8_t conditioningCount;
  Stream *_debugStream = nullptr;
  const char *TAG = "SGP4x";
#else
  TaskHandle_t pollTask;
#endif
  bool isBegin(void);
  bool boardSupported(void);
  bool getRawSignal(uint16_t &raw_voc, uint16_t &raw_nox,
                    uint16_t defaultRh = 0x8000, uint16_t defaultT = 0x6666);
  bool _noxConditioning(void);
};

#endif /** _AIR_GRADIENT_SGP4X_H_ */
