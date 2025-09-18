#ifndef _PMS5003T_BASE_H_
#define _PMS5003T_BASE_H_

class PMS5003TBase
{
private:

public:
  PMS5003TBase();
  ~PMS5003TBase();
  float compensateTemp(float temp);
  float compensateHum(float hum);
};

#endif
