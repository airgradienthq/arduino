#ifndef _AG_SCHEDULE_H_
#define _AG_SCHEDULE_H_

#include <Arduino.h>

class AgSchedule {
private:
  unsigned int period;
  void (*handler)(void);
  unsigned int count;

public:
  AgSchedule(unsigned int period, void (*handler)(void));
  ~AgSchedule();
  void run(void);
  void update(void);
  void setPeriod(unsigned int period);
};

#endif /** _AG_SCHEDULE_H_ */
