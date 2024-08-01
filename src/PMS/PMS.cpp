#include "PMS.h"
#include "../Main/BoardDef.h"

/**
 * @brief Init and check that sensor has connected
 *
 * @param stream UART stream
 * @return true Sucecss
 * @return false Failure
 */
bool PMSBase::begin(Stream *stream) {
  this->stream = stream;

  failed = true;
  lastRead = 0; // To read buffer on handle without wait after 1.5sec

  this->stream->flush();

  // Run and check sensor data for 4sec
  while (1) {
    handle();
    if (failed == false) {
      return true;
    }

    delay(1);
    uint32_t ms = (uint32_t)(millis() - lastRead);
    if (ms >= 4000) {
      break;
    }
  }
  return false;
}

/**
 * @brief Check and read sensor data then update variable.
 * Check result from method @isFailed before get value
 */
void PMSBase::handle() {
  uint32_t ms;
  if (lastRead == 0) {
    lastRead = millis();
    if (lastRead == 0) {
      lastRead = 1;
    }
  } else {
    ms = (uint32_t)(millis() - lastRead);
    /**
     * The PMS in Active mode sends an update data every 1 second. If we read
     * exactly every 1 sec then we may or may not get an update (depending on
     * timing tolerances). Hence we read every 2.5 seconds and expect 2 ..3
     * updates,
     */
    if (ms < 2500) {
      return;
    }
  }
  bool result = false;
  char buf[32];
  int bufIndex;
  int step = 0;
  int len = 0;
  int bcount = 0;

  while (stream->available()) {
    char value = stream->read();
    switch (step) {
    case 0: {
      if (value == 0x42) {
        step = 1;
        bufIndex = 0;
        buf[bufIndex++] = value;
      }
      break;
    }
    case 1: {
      if (value == 0x4d) {
        step = 2;
        buf[bufIndex++] = value;
        // Serial.println("Got 0x4d");
      } else {
        step = 0;
      }
      break;
    }
    case 2: {
      buf[bufIndex++] = value;
      if (bufIndex >= 4) {
        len = toValue(&buf[2]);
        if (len != 28) {
          // Serial.printf("Got good bad len %d\r\n", len);
          len += 4;
          step = 3;
        } else {
          // Serial.println("Got good len");
          step = 4;
        }
      }
      break;
    }
    case 3: {
      bufIndex++;
      if (bufIndex >= len) {
        step = 0;
        // Serial.println("Bad lengh read all buffer");
      }
      break;
    }
    case 4: {
      buf[bufIndex++] = value;
      if (bufIndex >= 32) {
        result |= validate(buf);
        step = 0;
        // Serial.println("Got data");
      }
      break;
    }
    default:
      break;
    }

    // Reduce core panic: delay 1 ms each 32bytes data
    bcount++;
    if ((bcount % 32) == 0) {
      delay(1);
    }
  }

  if (result) {
    lastRead = millis();
    if (lastRead == 0) {
      lastRead = 1;
    }
    failed = false;
  } else {
    if (ms > 5000) {
      failed = true;
    }
  }
}

/**
 * @brief Check that PMS send is failed or disconnected
 *
 * @return true Failed
 * @return false No problem
 */
bool PMSBase::isFailed(void) { return failed; }

/**
 * @brief Read PMS 0.1 ug/m3 with CF = 1 PM estimates
 *
 * @return uint16_t
 */
uint16_t PMSBase::getRaw0_1(void) { return toValue(&package[4]); }

/**
 * @brief Read PMS 2.5 ug/m3 with CF = 1 PM estimates
 *
 * @return uint16_t
 */
uint16_t PMSBase::getRaw2_5(void) { return toValue(&package[6]); }

/**
 * @brief Read PMS 10 ug/m3 with CF = 1 PM estimates
 *
 * @return uint16_t
 */
uint16_t PMSBase::getRaw10(void) { return toValue(&package[8]); }

/**
 * @brief Read PMS 0.1 ug/m3
 *
 * @return uint16_t
 */
uint16_t PMSBase::getPM0_1(void) { return toValue(&package[10]); }

/**
 * @brief Read PMS 2.5 ug/m3
 *
 * @return uint16_t
 */
uint16_t PMSBase::getPM2_5(void) { return toValue(&package[12]); }

/**
 * @brief Read PMS 10 ug/m3
 *
 * @return uint16_t
 */
uint16_t PMSBase::getPM10(void) { return toValue(&package[14]); }

/**
 * @brief Get numnber concentrations over 0.3 um/0.1L
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount0_3(void) { return toValue(&package[16]); }

/**
 * @brief Get numnber concentrations over 0.5 um/0.1L
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount0_5(void) { return toValue(&package[18]); }

/**
 * @brief Get numnber concentrations over 1.0 um/0.1L
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount1_0(void) { return toValue(&package[20]); }

/**
 * @brief Get numnber concentrations over 2.5 um/0.1L
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount2_5(void) { return toValue(&package[22]); }

/**
 * @brief Get numnber concentrations over 5.0 um/0.1L (only PMS5003)
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount5_0(void) { return toValue(&package[24]); }

/**
 * @brief Get numnber concentrations over 10.0 um/0.1L (only PMS5003)
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount10(void) { return toValue(&package[26]); }

/**
 * @brief Get temperature (only PMS5003T)
 *
 * @return uint16_t
 */
uint16_t PMSBase::getTemp(void) { return toValue(&package[24]); }

/**
 * @brief Get humidity (only PMS5003T)
 *
 * @return uint16_t
 */
uint16_t PMSBase::getHum(void) { return toValue(&package[26]); }

/**
 * @brief Convert PMS2.5 to US AQI unit
 *
 * @param pm02
 * @return int
 */
int PMSBase::pm25ToAQI(int pm02) {
  if (pm02 <= 9.0)
    return ((50 - 0) / (9.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4)
    return ((100 - 50) / (35.4 - 9.0) * (pm02 - 9.0) + 5);
  else if (pm02 <= 55.4)
    return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 125.4)
    return ((200 - 150) / (125.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 225.4)
    return ((300 - 200) / (225.4 - 125.4) * (pm02 - 125.4) + 200);
  else if (pm02 <= 325.4)
    return ((500 - 300) / (325.4 - 225.4) * (pm02 - 225.4) + 300);
  else
    return 500;
}

/**
 * @brief   Convert two byte value to uint16_t value
 *
 * @param buf bytes array (must be >= 2)
 * @return uint16_t
 */
uint16_t PMSBase::toValue(char *buf) { return (buf[0] << 8) | buf[1]; }

/**
 * @brief Validate package data
 *
 * @param buf Package buffer
 * @return true Success
 * @return false Failed
 */
bool PMSBase::validate(char *buf) {
  uint16_t sum = 0;
  for (int i = 0; i < 30; i++) {
    sum += buf[i];
  }
  if (sum == toValue(&buf[30])) {
    for (int i = 0; i < 32; i++) {
      package[i] = buf[i];
    }
    return true;
  }
  return false;
}
