#ifndef _AIR_GRADIENT_PMS5003_H_
#define _AIR_GRADIENT_PMS5003_H_

#include "../Main/BoardDef.h"
#include "PMS.h"
#include "Stream.h"
#ifdef ESP8266
#include <SoftwareSerial.h>
#endif

/**
 * @brief The class define how to handle PMS5003 sensor bas on @ref PMS class
 */
class PMS5003 {
public:
  PMS5003(BoardType def);
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
  int getPm5ParticleCount(void);
  int getPm10ParticleCount(void);

  int convertPm25ToUsAqi(int pm25);
  float slrCorrection(float pm25, float pm003Count, float scalingFactor, float intercept);
  float compensate(float pm25, float humidity);
  int getFirmwareVersion(void);
  uint8_t getErrorCode(void);
  bool connected(void);

private:
  bool _isBegin = false;
  int _ver;
  BoardType _boardDef;
  PMSBase pms;
  const BoardDef *bsp;
#if defined(ESP8266)
  Stream *_debugStream;
  const char *TAG = "PMS5003";
  SoftwareSerial *_serial;
#else
  HardwareSerial *_serial;
#endif
  bool begin(void);
  bool isBegin(void);
};
#endif /** _AIR_GRADIENT_PMS5003_H_ */
