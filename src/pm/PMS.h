#ifndef _PMS_BASE_H_
#define _PMS_BASE_H_

#include <Arduino.h>

class PMS {
public:
  static const uint16_t SINGLE_RESPONSE_TIME = 1000;
  static const uint16_t TOTAL_RESPONSE_TIME  = 1000 * 10;
  static const uint16_t STEADY_RESPONSE_TIME = 1000 * 30;

  // static const uint16_t BAUD_RATE = 9600;

  struct DATA {
    // Standard Particles, CF=1
    uint16_t PM_SP_UG_1_0;
    uint16_t PM_SP_UG_2_5;
    uint16_t PM_SP_UG_10_0;

    // Atmospheric environment
    uint16_t PM_AE_UG_1_0;
    uint16_t PM_AE_UG_2_5;
    uint16_t PM_AE_UG_10_0;

    // Raw particles count (number of particles in 0.1l of air
    uint16_t PM_RAW_0_3;
    uint16_t PM_RAW_0_5;
    uint16_t PM_RAW_1_0;
    uint16_t PM_RAW_2_5;
    uint16_t PM_RAW_5_0;
    uint16_t PM_RAW_10_0;

    // Formaldehyde (HCHO) concentration in mg/m^3 - PMSxxxxST units only
    uint16_t AMB_HCHO;

    // Temperature & humidity - PMSxxxxST units only
    int16_t AMB_TMP;
    uint16_t AMB_HUM;
  };

  bool begin(Stream* stream);
  void sleep();
  void wakeUp();
  void activeMode();
  void passiveMode();

  void requestRead();
  bool read(DATA &data);
  bool readUntil(DATA &data, uint16_t timeout = SINGLE_RESPONSE_TIME);

private:
  enum STATUS { STATUS_WAITING, STATUS_OK };
  enum MODE { MODE_ACTIVE, MODE_PASSIVE };

  uint8_t _payload[50];
  Stream *_stream;
  DATA *_data;
  STATUS _status;
  MODE _mode = MODE_ACTIVE;

  uint8_t _index = 0;
  uint16_t _frameLen;
  uint16_t _checksum;
  uint16_t _calculatedChecksum;

  void loop();
  char Char_PM2[10];
};

#endif
