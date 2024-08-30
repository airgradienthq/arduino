#include "PMS5003TBase.h"

PMS5003TBase::PMS5003TBase() {}

PMS5003TBase::~PMS5003TBase() {}

float PMS5003TBase::compensateTemp(float temp) {
  if (temp < 10.0f) {
    return temp * 1.327f - 6.738f;
  }
  return temp * 1.181f - 5.113f;
}

float PMS5003TBase::compensateHum(float hum) {
  hum = hum * 1.259f + 7.34f;

  if (hum > 100.0f) {
    hum = 100.0f;
  }
  return hum;
}
