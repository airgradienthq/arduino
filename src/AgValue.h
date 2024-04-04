#ifndef _AG_VALUE_H_
#define _AG_VALUE_H_

class AgValue {
private:
public:
  AgValue() {}
  ~AgValue() {}

  float Temperature = -1001;
  int Humidity = -1;
  int CO2 = -1;
  int PM25 = -1;
  int TVOC = -1;
  int TVOCRaw = -1;
  int NOx = -1;
  int NOxRaw = -1;
};

#endif /** _AG_VALUE_H_ */
