#ifndef _PMS5003T_H_
#define _PMS5003T_H_

#include "../Main/BoardDef.h"
#include "PMS.h"
#include "PMS5003TBase.h"
#include "Stream.h"
#include <HardwareSerial.h>
#ifdef ESP8266
#include <SoftwareSerial.h>
#endif

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
  void updateFailCount(void);
  void resetFailCount(void);
  int getFailCount(void);
  int getFailCountMax(void);
  // Atmospheric environment
  int getPm01Ae(void);
  int getPm25Ae(void);
  int getPm10Ae(void);
  // Standard particle
  int getPm01Sp(void);
  int getPm25Sp(void);
  int getPm10Sp(void);
  // Particle count
  int getPm03ParticleCount(void);
  int getPm05ParticleCount(void);
  int getPm01ParticleCount(void);
  int getPm25ParticleCount(void);

  int convertPm25ToUsAqi(int pm25);
  float getTemperature(void);
  float getRelativeHumidity(void);
  float compensate(float pm25, float humidity);
  int getFirmwareVersion(void);
  uint8_t getErrorCode(void);
  bool connected(void);

private:
  bool _isBegin = false;
  bool _isSleep = false;
  int _ver;   /** Firmware version code */

  BoardType _boardDef;
  const BoardDef *bsp;
#if defined(ESP8266)
  Stream *_debugStream;
  const char *TAG = "PMS5003T";
  SoftwareSerial *_serial;
#else
  HardwareSerial *_serial;
#endif
  PMSBase pms;
  bool begin(void);
  bool isBegin(void);
};

#endif /** _PMS5003T_H_ */
