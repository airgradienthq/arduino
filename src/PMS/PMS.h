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
  void readPackage(Stream *stream);
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
  bool connected(void);

  /** For PMS5003 */
  uint16_t getCount5_0(void);
  uint16_t getCount10(void);

  /** For PMS5003T*/
  int16_t getTemp(void);
  uint16_t getHum(void);
  uint8_t getFirmwareVersion(void);
  uint8_t getErrorCode(void);

  int pm25ToAQI(int pm02);
  float slrCorrection(float pm25, float pm003Count, float scalingFactor, float intercept);
  float compensate(float pm25, float humidity);

private:
  static const uint8_t package_size = 32;

  /** In normal package interval is 200-800ms, In case small changed on sensor
   * it's will interval reach to 2.3sec
   */
  const uint16_t READ_PACKGE_TIMEOUT = 3000;   /** ms */
  const int failCountMax = 10;
  int failCount = 0;

  uint8_t readBuffer[package_size];
  uint8_t readBufferIndex = 0;

  /**
   * Save last time received package success. 0 to disable check package
   * timeout.
   */
  unsigned long lastPackage = 0;
  bool _connected;

  unsigned long lastReadPackage = 0;

  uint16_t pms_raw0_1;
  uint16_t pms_raw2_5;
  uint16_t pms_raw10;
  uint16_t pms_pm0_1;
  uint16_t pms_pm2_5;
  uint16_t pms_pm10;
  uint16_t pms_count0_3;
  uint16_t pms_count0_5;
  uint16_t pms_count1_0;
  uint16_t pms_count2_5;
  uint16_t pms_count5_0;
  uint16_t pms_count10;
  int16_t  pms_temp;
  uint16_t pms_hum;
  uint8_t  pms_errorCode;
  uint8_t  pms_firmwareVersion;

  int16_t toI16(const uint8_t *buf);
  uint16_t toU16(const uint8_t *buf);
  bool validate(const uint8_t *buf);
  void parse(const uint8_t* buf);
};

#endif /** _PMS5003_BASE_H_ */
