#ifndef _PMS5003T_H_
#define _PMS5003T_H_

#include <HardwareSerial.h>
#include "../Main/BoardDef.h"
#include "PMS.h"
#include "Stream.h"

/**
 * @brief The class define how to handle PMS5003T sensor bas on @ref PMS class
 */
class PMS5003T {
public:
  PMS5003T(BoardType def);
#if defined(ESP8266)
  bool begin(Stream *_debugStream);
#else
  bool begin(HardwareSerial &serial);
#endif
  void end(void);

  bool readData(void);
  int getPm01Ae(void);
  int getPm25Ae(void);
  int getPm10Ae(void);
  int getPm03ParticleCount(void);
  int convertPm25ToUsAqi(int pm25);
  float getTemperature(void);
  float getRelativeHumidity(void);

private:
  bool _isBegin = false;
  bool _isSleep = false;

  BoardType _boardDef;
  const BoardDef *bsp;
#if defined(ESP8266)
  Stream *_debugStream;
  const char *TAG = "PMS5003T";
#else
  HardwareSerial *_serial;
#endif

  bool begin(void);
  int pm25ToAQI(int pm02);
  PMS pms;
  PMS::DATA pmsData;
  bool isBegin(void);
  float correctionTemperature(float inTemp);
  float correctionRelativeHumidity(float inHum);
};

#endif /** _PMS5003T_H_ */
