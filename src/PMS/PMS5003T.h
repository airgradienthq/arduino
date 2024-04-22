#ifndef _PMS5003T_H_
#define _PMS5003T_H_

#include "../Main/BoardDef.h"
#include "PMS.h"
#include "PMS5003TBase.h"
#include "Stream.h"
#include <HardwareSerial.h>

/**
 * @brief The class define how to handle PMS5003T sensor bas on @ref PMS class
 */
class PMS5003T: public PMS5003TBase  {
public:
  PMS5003T(BoardType def);
#if defined(ESP8266)
  bool begin(Stream *_debugStream);
#else
  bool begin(HardwareSerial &serial);
#endif
  void end(void);

  void handle(void);
  bool isFailed(void);
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
  PMSBase pms;
  bool begin(void);
  bool isBegin(void);
};

#endif /** _PMS5003T_H_ */
