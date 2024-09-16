#include "AgSchedule.h"

AgSchedule::AgSchedule(unsigned int period, void (*handler)(void))
    : period(period), handler(handler) {}

AgSchedule::~AgSchedule() {}

void AgSchedule::run(void) {
  if (count == 0) {
    count = millis();
    if (count == 0) {
      count = 1;
    }
  }

  unsigned int ms = (unsigned int)(millis() - count);
  if (ms >= period) {
    handler();

    count = millis();
    if (count == 0) {
      count = 1;
    }
  }
}

/**
 * @brief Set schedule period
 *
 * @param period Period in ms
 */
void AgSchedule::setPeriod(unsigned int period) { this->period = period; }

/**
 * @brief Update period
 */
void AgSchedule::update(void) { count = millis(); }
