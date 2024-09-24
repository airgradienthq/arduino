#include "utils.h"

#define VALID_TEMPERATURE_MAX (125)
#define VALID_TEMPERATURE_MIN (-40)
#define INVALID_TEMPERATURE   (-1000)

#define VALID_HUMIDITY_MAX    (100)
#define VALID_HUMIDITY_MIN    (0)
#define INVALID_HUMIDITY      (-1)

#define VALID_PMS_MAX         (1000)
#define VALID_PMS_MIN         (0)
#define INVALID_PMS           (-1)

#define VALID_PMS03COUNT_MIN  (0)

#define VALID_CO2_MAX         (10000)
#define VALID_CO2_MIN         (0)
#define INVALID_CO2           (-1)

#define VALID_NOX_MIN         (0)
#define VALID_VOC_MIN         (0)
#define INVALID_NOX           (-1)
#define INVALID_VOC           (-1)

utils::utils(/* args */) {}

utils::~utils() {}

bool utils::isValidTemperature(float value) {
  if ((value >= VALID_TEMPERATURE_MIN) && (value <= VALID_TEMPERATURE_MAX)) {
    return true;
  }
  return false;
}

bool utils::isValidHumidity(float value) {
  if ((value >= VALID_HUMIDITY_MIN) && (value <= VALID_HUMIDITY_MAX)) {
    return true;
  }
  return false;
}

bool utils::isValidCO2(int16_t value) {
  if ((value >= VALID_CO2_MIN) && (value <= VALID_CO2_MAX)) {
    return true;
  }
  return false;
}

bool utils::isValidPm(int value) {
  if ((value >= VALID_PMS_MIN) && (value <= VALID_PMS_MAX)) {
    return true;
  }
  return false;
}

bool utils::isValidPm03Count(int value) {
  if (value >= VALID_PMS03COUNT_MIN) {
    return true;
  }
  return false;
}

bool utils::isValidNOx(int value) {
  if (value >= VALID_NOX_MIN) {
    return true;
  }
  return false;
}

bool utils::isValidVOC(int value) {
  if (value >= VALID_VOC_MIN) {
    return true;
  }
  return false;
}

float utils::getInvalidTemperature(void) { return INVALID_TEMPERATURE; }

float utils::getInvalidHumidity(void) { return INVALID_HUMIDITY; }

int utils::getInvalidCO2(void) { return INVALID_CO2; }

int utils::getInvalidPmValue(void) { return INVALID_PMS; }

int utils::getInvalidNOx(void) { return INVALID_NOX; }

int utils::getInvalidVOC(void) { return INVALID_VOC; }

float utils::degreeC_To_F(float t) {
  /** (t * 9)/5 + 32 */
  return t * 1.8f + 32.0f;
}
