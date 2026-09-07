#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#ifdef ESP32

#include "Libraries/EMC230x/EMC230x.h"

#include <Arduino.h>
#include <Wire.h>

#define FAN_CONTROLLER_I2C_ADDRESS 0x2F
#define FAN_CONTROLLER_UPDATE_INTERVAL_MS 10000 // 10s

#define FAN_CONTROLLER_CHANNEL 1
#define FAN_CONTROLLER_PM25_TARGET_UGM3 5.0f
#define FAN_CONTROLLER_CO2_PERFECT_PPM 500.0f
#define FAN_CONTROLLER_CO2_TARGET_PPM 1000.0f
#define FAN_CONTROLLER_DEFAULT_SPEED_PERCENT 40
#define FAN_CONTROLLER_MIN_SPEED_PERCENT 30
#define FAN_CONTROLLER_MAX_SPEED_PERCENT 100

class FanController {
public:
  explicit FanController(TwoWire &wire);

  bool begin(void);
  bool update(float pm25Ugm3, bool hasPm25, float co2Ppm, bool hasCo2);

  bool isActive(void) const;
  uint8_t getSpeedPercent(void) const;
  int getTachCount(void);
  uint8_t getProductID(void) const;

private:
  static uint8_t _calculateSpeedPercent(float pm25Ugm3, bool hasPm25, float co2Ppm, bool hasCo2);

  EMC230x emc230x;
  bool active;
  uint8_t speedPercent;
  uint8_t productId;
};

#endif // ESP32
#endif // FAN_CONTROLLER_H
