#ifndef _PMS5003_BASE_H_
#define _PMS5003_BASE_H_

#include <Arduino.h>

class PMSBase {
public:
  bool begin(Stream *stream);
  void handle();
  bool isFailed(void);
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
  uint16_t getTemp(void);
  uint16_t getHum(void);
  uint8_t getFirmwareVersion(void); 

  int pm25ToAQI(int pm02);

private:
  Stream *stream;
  char package[32];
  int packageIndex;
  bool failed = false;
  uint32_t lastRead;

  uint16_t toValue(char *buf);
  bool validate(char *buf);
};

#endif /** _PMS5003_BASE_H_ */
