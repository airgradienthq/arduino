#ifndef _PMS5003_BASE_H_
#define _PMS5003_BASE_H_

#include <Arduino.h>

#define PMS_FAIL_COUNT_SET_INVALID 3

/**
 * Known to work with these sensors: Plantower PMS5003, Plantower PMS5003, Cubic PM2009X
 */
class PMSBase {
public:
  bool begin(Stream *stream);
  void handle();
  bool isFailed(void);
  void updateFailCount(void);
  void resetFailCount(void);
  int getFailCount(void);
  int getFailCountMax(void);
  uint16_t getRaw0_1(void);
  uint16_t getRaw2_5(void);
  uint16_t getRaw10(void);
  uint16_t getPM0_1(void);
  uint16_t getPM2_5(void);
  uint16_t getPM10(void);
  uint16_t getCount0_3(void);
  uint16_t getCount0_5(void);
  uint16_t getCount1_0(void);
  uint16_t getCount2_5(void);

  /** For PMS5003 */
  uint16_t getCount5_0(void);
  uint16_t getCount10(void);

  /** For PMS5003T*/
  int16_t getTemp(void);
  uint16_t getHum(void);
  uint8_t getFirmwareVersion(void); 
  uint8_t getErrorCode(void);

  int pm25ToAQI(int pm02);
  int compensate(int pm25, float humidity);

private:
  Stream *stream;
  char package[32];
  int packageIndex;
  bool failed = false;
  uint32_t lastRead;
  const int failCountMax = 10;
  int failCount = 0;

  int16_t toI16(char *buf);
  uint16_t toU16(char* buf);
  bool validate(char *buf);
};

#endif /** _PMS5003_BASE_H_ */
