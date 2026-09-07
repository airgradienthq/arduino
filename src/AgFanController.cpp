#ifdef ESP32

#include "AgFanController.h"
#include <cmath>

FanController::FanController(TwoWire &wire)
    : emc230x(FAN_CONTROLLER_I2C_ADDRESS, wire), active(false),
      speedPercent(FAN_CONTROLLER_DEFAULT_SPEED_PERCENT), productId(0) {}

bool FanController::begin(void) {
  active = false;
  productId = 0;

  if (!emc230x.beginWithoutWireInit()) {
    return false;
  }
  productId = emc230x.getProductID();

  if (!emc230x.setPWMFrequency(FAN_CONTROLLER_CHANNEL, PWM_FREQ_4882_HZ) ||
      !emc230x.setPWMOutputType(FAN_CONTROLLER_CHANNEL, true)) {
    return false;
  }

  FanConfig fanConfig{};
  fanConfig.enableClosedLoop = false;
  fanConfig.minRPM = 0;
  fanConfig.edges = 1;
  fanConfig.updateTime = 0;
  fanConfig.enableRampRate = false;
  fanConfig.enableGlitchFilter = true;
  fanConfig.errorWindow = 2;
  if (!emc230x.setFanConfig(FAN_CONTROLLER_CHANNEL, fanConfig)) {
    return false;
  }

  speedPercent = FAN_CONTROLLER_DEFAULT_SPEED_PERCENT;
  if (!emc230x.setFanSpeedPercent(FAN_CONTROLLER_CHANNEL, speedPercent)) {
    return false;
  }

  active = true;
  return true;
}

bool FanController::update(float pm25Ugm3, bool hasPm25, float co2Ppm, bool hasCo2) {
  if (!active) {
    return false;
  }

  const uint8_t speed = _calculateSpeedPercent(pm25Ugm3, hasPm25, co2Ppm, hasCo2);
  if (speed == speedPercent) {
    return true;
  }

  if (!emc230x.setFanSpeedPercent(FAN_CONTROLLER_CHANNEL, speed)) {
    return false;
  }

  speedPercent = speed;
  return true;
}

bool FanController::isActive(void) const { return active; }

uint8_t FanController::getSpeedPercent(void) const { return speedPercent; }

int FanController::getTachCount(void) {
  const uint16_t tachCount = emc230x.getTachCount(FAN_CONTROLLER_CHANNEL);
  if (tachCount == 0 || tachCount >= 0x1FFF) {
    return -1;
  }
  return tachCount;
}

uint8_t FanController::getProductID(void) const { return productId; }

uint8_t FanController::_calculateSpeedPercent(float pm25Ugm3, bool hasPm25, float co2Ppm,
                                              bool hasCo2) {
  float speedBasedOnPm = FAN_CONTROLLER_DEFAULT_SPEED_PERCENT;
  if (hasPm25) {
    float pmRatio = 0.0f;
    if (pm25Ugm3 > 0.0f) {
      pmRatio = pm25Ugm3 / FAN_CONTROLLER_PM25_TARGET_UGM3;
      if (pmRatio > 1.0f) {
        pmRatio = 1.0f;
      }
    }
    speedBasedOnPm =
        FAN_CONTROLLER_MIN_SPEED_PERCENT +
        ((FAN_CONTROLLER_MAX_SPEED_PERCENT - FAN_CONTROLLER_MIN_SPEED_PERCENT) * pmRatio);
  }

  float speedBasedOnCo2 = 0.0f;
  if (hasCo2 && co2Ppm > FAN_CONTROLLER_CO2_PERFECT_PPM) {
    float co2Ratio = (co2Ppm - FAN_CONTROLLER_CO2_PERFECT_PPM) /
                     (FAN_CONTROLLER_CO2_TARGET_PPM - FAN_CONTROLLER_CO2_PERFECT_PPM);
    if (co2Ratio > 1.0f) {
      co2Ratio = 1.0f;
    }
    speedBasedOnCo2 =
        FAN_CONTROLLER_MIN_SPEED_PERCENT +
        ((FAN_CONTROLLER_MAX_SPEED_PERCENT - FAN_CONTROLLER_MIN_SPEED_PERCENT) * co2Ratio);
  }

  float speed = speedBasedOnPm;
  if (speedBasedOnCo2 > speed) {
    speed = speedBasedOnCo2;
  }
  if (!hasPm25 && !hasCo2) {
    speed = FAN_CONTROLLER_DEFAULT_SPEED_PERCENT;
  }

  if (speed < 0.0f) {
    speed = 0.0f;
  } else if (speed > 100.0f) {
    speed = 100.0f;
  }
  return static_cast<uint8_t>(std::round(speed));
}

#endif
