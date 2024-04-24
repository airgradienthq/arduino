#ifndef _AG_SCHEDULE_H_
#define _AG_SCHEDULE_H_

#include <Arduino.h>

class AgSchedule {
private:
  int period;
  void (*handler)(void);
  uint32_t count;

public:
  AgSchedule(int period, void (*handler)(void));
  ~AgSchedule();
  void run(void);
  void update(void);
  void setPeriod(int period);
};

#endif /** _AG_SCHEDULE_H_ */
