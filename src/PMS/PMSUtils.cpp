#include "PMSUtils.h"

/**
 * @brief Convert PM2.5 to US AQI
 *
 * @param pm02
 * @return int
 */
int pm25ToAQI(int pm02) {
  if (pm02 <= 12.0)
    return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4)
    return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4)
    return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4)
    return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4)
    return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4)
    return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4)
    return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else
    return 500;
}
