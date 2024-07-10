#ifndef _UTILS_H_
#define _UTILS_H_

#include <Arduino.h>

class utils
{
private:
  /* data */
public:
  utils(/* args */);
  ~utils();

  static float correctTemperature(float value);
  static float correctHumidity(float value);
  static int16_t correctCO2(int16_t value);
  static int correctPMS(int value);
};


#endif /** _UTILS_H_ */
