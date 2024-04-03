#include "AgSchedule.h"

AgSchedule::AgSchedule(int period, void (*handler)(void))
    : period(period), handler(handler) {}

AgSchedule::~AgSchedule() {}

void AgSchedule::run(void) {
  uint32_t ms = (uint32_t)(millis() - count);
  if (ms >= period) {
    handler();
    count = millis();
  }
}

/**
 * @brief Set schedule period
 * 
 * @param period Period in ms
 */
void AgSchedule::setPeriod(int period) { this->period = period; }
