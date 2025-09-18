#include "PMS.h"
#include "../Main/BoardDef.h"

/**
 * @brief Initializes the sensor and attempts to read data.
 *
 * @param stream UART stream
 * @return true Sucecss
 * @return false Failure
 */
bool PMSBase::begin(Stream *stream) {
  Serial.printf("initializing PM sensor\n");

  failCount = 0;
  _connected = false;

  // empty first
  int bytesCleared = 0;
  while (stream->read() != -1) {
    bytesCleared++;
  }
  Serial.printf("cleared %d byte(s)\n", bytesCleared);

  // explicitly put the sensor into active mode, this seems to be be needed for the Cubic PM2009X
  Serial.printf("setting active mode\n");
  uint8_t activeModeCommand[] = { 0x42, 0x4D, 0xE1, 0x00, 0x01, 0x01, 0x71 };
  size_t bytesWritten = stream->write(activeModeCommand, sizeof(activeModeCommand));
  Serial.printf("%d byte(s) written\n", bytesWritten);

  // Run and check sensor data for 4sec
  unsigned long lastInit = millis();
  while (true) {
    readPackage(stream);
    if (_connected) {
      break;
    }

    delay(1);
    unsigned long ms = (unsigned long)(millis() - lastInit);
    if (ms >= 4000) {
      break;
    }
  }
  return _connected;
}

/**
 * @brief Read PMS package send to device each 1sec
 *
 * @param serial
 */
void PMSBase::readPackage(Stream *serial) {
  /** If readPackage has process as period larger than READ_PACKAGE_TIMEOUT,
   * should be clear the lastPackage and readBufferIndex */
  if (lastReadPackage) {
    unsigned long ms = (unsigned long)(millis() - lastReadPackage);
    if (ms >= READ_PACKGE_TIMEOUT) {
      /** Clear buffer */
      readBufferIndex = 0;

      /** Disable check read package timeout */
      lastPackage = 0;

      Serial.println("Last process timeout, clear buffer and last handle package");
    }

    lastReadPackage = millis();
    if (!lastReadPackage) {
      lastReadPackage = 1;
    }
  } else {
    lastReadPackage = millis();
    if (!lastReadPackage) {
      lastReadPackage = 1;
    }
  }

  /** Count to call delay() to release the while loop MCU resource for avoid the
   * watchdog time reset */
  uint8_t delayCount = 0;
  while (serial->available()) {
    /** Get value */
    uint8_t value = (uint8_t)serial->read();

    /** Process receiving package... */
    switch (readBufferIndex) {
    case 0: /** Start byte 1 */
      if (value == 0x42) {
        readBuffer[readBufferIndex++] = value;
      }
      break;
    case 1: /** Start byte 2 */
      if (value == 0x4d) {
        readBuffer[readBufferIndex++] = value;
      } else {
        readBufferIndex = 0;
      }
      break;
    case 2: /** Frame length */
      if (value == 0x00) {
        readBuffer[readBufferIndex++] = value;
      } else {
        readBufferIndex = 0;
      }
      break;
    case 3: /** Frame length */
      if (value == 0x1C) {
        readBuffer[readBufferIndex++] = value;
      } else {
        readBufferIndex = 0;
      }
      break;
    default: /** Data */
    {
      readBuffer[readBufferIndex++] = value;

      /** Check that received full bufer */
      if (readBufferIndex >= sizeof(readBuffer)) {
        /** validata package */
        if (validate(readBuffer)) {
          _connected = true; /** Set connected status */

          /** Parse data */
          parse(readBuffer);

          /** Set last received package */
          lastPackage = millis();
          if (lastPackage == 0) {
            lastPackage = 1;
          }
        }

        /** Clear buffer index */
        readBufferIndex = 0;
      }
      break;
    }
    }

    /** Avoid task watchdog timer reset... */
    delayCount++;
    if (delayCount >= 32) {
      delayCount = 0;
      delay(1);
    }
  }

  /** Check that sensor removed */
  if (lastPackage) {
    unsigned long ms = (unsigned long)(millis() - lastPackage);
    if (ms >= READ_PACKGE_TIMEOUT) {
      lastPackage = 0;
      _connected = false;
      Serial.println("PMS disconnected");
    }
  }
}

/**
 * @brief Increate number of fail
 *
 */
void PMSBase::updateFailCount(void) {
  if (failCount < failCountMax) {
    failCount++;
  }
}

void PMSBase::resetFailCount(void) { failCount = 0; }

/**
 * @brief Get number of fail
 *
 * @return int
 */
int PMSBase::getFailCount(void) { return failCount; }

int PMSBase::getFailCountMax(void) { return failCountMax; }

/**
 * @brief Read PMS 0.1 ug/m3 with CF = 1 PM estimates
 *
 * @return uint16_t
 */
uint16_t PMSBase::getRaw0_1(void) { return pms_raw0_1; }

/**
 * @brief Read PMS 2.5 ug/m3 with CF = 1 PM estimates
 *
 * @return uint16_t
 */
uint16_t PMSBase::getRaw2_5(void) { return pms_raw2_5; }

/**
 * @brief Read PMS 10 ug/m3 with CF = 1 PM estimates
 *
 * @return uint16_t
 */
uint16_t PMSBase::getRaw10(void) { return pms_raw10; }

/**
 * @brief Read PMS 0.1 ug/m3
 *
 * @return uint16_t
 */
uint16_t PMSBase::getPM0_1(void) { return pms_pm0_1; }

/**
 * @brief Read PMS 2.5 ug/m3
 *
 * @return uint16_t
 */
uint16_t PMSBase::getPM2_5(void) { return pms_pm2_5; }

/**
 * @brief Read PMS 10 ug/m3
 *
 * @return uint16_t
 */
uint16_t PMSBase::getPM10(void) { return pms_pm10; }

/**
 * @brief Get numnber concentrations over 0.3 um/0.1L
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount0_3(void) { return pms_count0_3; }

/**
 * @brief Get numnber concentrations over 0.5 um/0.1L
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount0_5(void) { return pms_count0_5; }

/**
 * @brief Get numnber concentrations over 1.0 um/0.1L
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount1_0(void) { return pms_count1_0; }

/**
 * @brief Get numnber concentrations over 2.5 um/0.1L
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount2_5(void) { return pms_count2_5; }

bool PMSBase::connected(void) { return _connected; }

/**
 * @brief Get numnber concentrations over 5.0 um/0.1L (only PMS5003)
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount5_0(void) { return pms_count5_0; }

/**
 * @brief Get numnber concentrations over 10.0 um/0.1L (only PMS5003)
 *
 * @return uint16_t
 */
uint16_t PMSBase::getCount10(void) { return pms_count10; }

/**
 * @brief Get temperature (only PMS5003T)
 *
 * @return uint16_t
 */
int16_t PMSBase::getTemp(void) { return pms_temp; }

/**
 * @brief Get humidity (only PMS5003T)
 *
 * @return uint16_t
 */
uint16_t PMSBase::getHum(void) { return pms_hum; }

/**
 * @brief Get firmware version code
 *
 * @return uint8_t
 */
uint8_t PMSBase::getFirmwareVersion(void) { return pms_firmwareVersion; }

/**
 * @brief Ge PMS5003 error code
 *
 * @return uint8_t
 */
uint8_t PMSBase::getErrorCode(void) { return pms_errorCode; }

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
    return ((100 - 51) / (35.4 - 9.1) * (pm02 - 9.0) + 51);
  else if (pm02 <= 55.4)
    return ((150 - 101) / (55.4 - 35.5) * (pm02 - 35.5) + 101);
  else if (pm02 <= 125.4)
    return ((200 - 151) / (125.4 - 55.5) * (pm02 - 55.5) + 151);
  else if (pm02 <= 225.4)
    return ((300 - 201) / (225.4 - 125.5) * (pm02 - 125.5) + 201);
  else if (pm02 <= 325.4)
    return ((500 - 301) / (325.4 - 225.5) * (pm02 - 225.5) + 301);
  else
    return 500;
}


/**
 * @brief SLR correction for PM2.5
 *
 * Reference: https://www.airgradient.com/blog/low-readings-from-pms5003/
 *
 * @param pm25 PM2.5 raw value
 * @param pm003Count PM0.3 count
 * @param scalingFactor Scaling factor
 * @param intercept Intercept
 * @return float Calibrated PM2.5 value
 */
float PMSBase::slrCorrection(float pm25, float pm003Count, float scalingFactor, float intercept) {
  float calibrated;

  float lowCalibrated = (scalingFactor * pm003Count) + intercept;
  if (lowCalibrated < 31) {
    calibrated = lowCalibrated;
  } else {
    calibrated = pm25;
  }

  // No negative value for pm2.5
  if (calibrated < 0) {
    return 0.0;
  }

  return calibrated;
}

/**
 * @brief Correction PM2.5
 *
 * Formula: https://www.airgradient.com/documentation/correction-algorithms/
 *
 * @param pm25 Raw PM2.5 value
 * @param humidity Humidity value (%)
 * @return compensated pm25 value
 */
float PMSBase::compensate(float pm25, float humidity) {
  float value;

  // Correct invalid humidity value
  if (humidity < 0) {
    humidity = 0;
  }
  if (humidity > 100) {
    humidity = 100.0f;
  }

  // If its already 0, do not proceed
  if (pm25 == 0) {
    return 0.0;
  }

  if (pm25 < 30) { /** pm2.5 < 30 */
    value = (pm25 * 0.524f) - (humidity * 0.0862f) + 5.75f;
  } else if (pm25 < 50) { /** 30 <= pm2.5 < 50 */
    value = (0.786f * (pm25 * 0.05f - 1.5f) + 0.524f * (1.0f - (pm25 * 0.05f - 1.5f))) * pm25 -
            (0.0862f * humidity) + 5.75f;
  } else if (pm25 < 210) { /** 50 <= pm2.5 < 210 */
    value = (0.786f * pm25) - (0.0862f * humidity) + 5.75f;
  } else if (pm25 < 260) { /** 210 <= pm2.5 < 260 */
    value = (0.69f * (pm25 * 0.02f - 4.2f) + 0.786f * (1.0f - (pm25 * 0.02f - 4.2f))) * pm25 -
            (0.0862f * humidity * (1.0f - (pm25 * 0.02f - 4.2f))) +
            (2.966f * (pm25 * 0.02f - 4.2f)) + (5.75f * (1.0f - (pm25 * 0.02f - 4.2f))) +
            (8.84f * (1.e-4) * pm25 * pm25 * (pm25 * 0.02f - 4.2f));
  } else { /** 260 <= pm2.5 */
    value = 2.966f + (0.69f * pm25) + (8.84f * (1.e-4) * pm25 * pm25);
  }

  // No negative value for pm2.5
  if (value < 0) {
    return 0.0;
  }

  return value;
}

/**
 * @brief   Convert two byte value to uint16_t value
 *
 * @param buf bytes array (must be >= 2)
 * @return int16_t
 */
int16_t PMSBase::toI16(const uint8_t *buf) {
  int16_t value = buf[0];
  value = (value << 8) | buf[1];
  return value;
}

uint16_t PMSBase::toU16(const uint8_t *buf) {
  uint16_t value = buf[0];
  value = (value << 8) | buf[1];
  return value;
}

/**
 * @brief Validate package data
 *
 * @param buf Package buffer
 * @return true Success
 * @return false Failed
 */
bool PMSBase::validate(const uint8_t *buf) {
  uint16_t sum = 0;
  for (int i = 0; i < 30; i++) {
    sum += buf[i];
  }
  if (sum == toU16(&buf[30])) {
    return true;
  }
  return false;
}

void PMSBase::parse(const uint8_t *buf) {
  // Standard particle
  pms_raw0_1 = toU16(&buf[4]);
  pms_raw2_5 = toU16(&buf[6]);
  pms_raw10 = toU16(&buf[8]);
  // atmospheric
  pms_pm0_1 = toU16(&buf[10]);
  pms_pm2_5 = toU16(&buf[12]);
  pms_pm10 = toU16(&buf[14]);

  // particle count
  pms_count0_3 = toU16(&buf[16]);
  pms_count0_5 = toU16(&buf[18]);
  pms_count1_0 = toU16(&buf[20]);
  pms_count2_5 = toU16(&buf[22]);
  pms_count5_0 = toU16(&buf[24]); // PMS5003 only
  pms_count10 = toU16(&buf[26]);  // PMS5003 only

  // Others
  pms_temp = toU16(&buf[24]); // PMS5003T only
  pms_hum = toU16(&buf[26]);  // PMS5003T only
  pms_firmwareVersion = buf[28];
  pms_errorCode = buf[29];
}
