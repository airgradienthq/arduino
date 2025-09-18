#include "PMS5003TBase.h"

PMS5003TBase::PMS5003TBase() {}

PMS5003TBase::~PMS5003TBase() {}

/**
 * @brief Compensate the temperature
 *
 * Reference formula: https://www.airgradient.com/documentation/correction-algorithms/
 *
 * @param temp
 * @return * float
 */
float PMS5003TBase::compensateTemp(float temp) {
  if (temp < 10.0f) {
    return temp * 1.327f - 6.738f;
  }
  return temp * 1.181f - 5.113f;
}

/**
 * @brief Compensate the humidity
 *
 * Reference formula: https://www.airgradient.com/documentation/correction-algorithms/
 *
 * @param temp
 * @return * float
 */
float PMS5003TBase::compensateHum(float hum) {
  hum = hum * 1.259f + 7.34f;

  if (hum > 100.0f) {
    hum = 100.0f;
  }
  return hum;
}
