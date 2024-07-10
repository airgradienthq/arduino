#include "utils.h"

utils::utils(/* args */)
{
}

utils::~utils()
{
}

float utils::correctTemperature(float value) {
  if (value < -40) {
    return -40;
  }
  if (value > 100) {
    return 125;
  }
  return value;
}

float utils::correctHumidity(float value) {
  if (value < 0) {
    return 0;
  }
  if (value > 100) {
    return 100;
  }
  return value;
}

int16_t utils::correctCO2(int16_t value) {
  if (value < 0) {
    return 0;
  }
  if (value > 10000) {
    return 10000;
  }
  return value;
}

int utils::correctPMS(int value) {
  if (value < 10) {
    return 10;
  }
  if (value > 1000) {
    return 1000;
  }
  return value;
}
