#include "PMS5003.h"
#include "Arduino.h"
#include "../Main/utils.h"

#if defined(ESP8266)
/**
 * @brief Init sensor
 *
 * @param _debugStream Serial use for print debug log
 * @return true Success
 * @return false Failure
 */
bool PMS5003::begin(Stream *_debugStream) {
  this->_debugStream = _debugStream;
  return this->begin();
}
#else

/**
 * @brief Init Sensor
 *
 * @param serial Serial communication with sensor
 * @return true Success
 * @return false Failure
 */
bool PMS5003::begin(HardwareSerial &serial) {
  this->_serial = &serial;
  return this->begin();
}
#endif

/**
 * @brief Construct a new PMS5003::PMS5003 object
 *
 * @param def Board type @ref BoardType
 */
PMS5003::PMS5003(BoardType def) : _boardDef(def) {}

/**
 * Initializes the sensor.
 */
bool PMS5003::begin(void) {
  if (this->_isBegin) {
    AgLog("Already initialized, call end() then try again");
    return true;
  }

#if defined(ESP32)
#endif
  this->bsp = getBoardDef(this->_boardDef);
  if (bsp == NULL) {
    AgLog("Board [%d] not supported", this->_boardDef);
    return false;
  }

  if (bsp->Pms5003.supported == false) {
    AgLog("Board [%d] PMS50035003 not supported", this->_boardDef);
    return false;
  }

#if defined(ESP8266)
  this->_serial =
      new SoftwareSerial(bsp->Pms5003.uart_tx_pin, bsp->Pms5003.uart_rx_pin);
  this->_serial->begin(9600);
  if (pms.begin(this->_serial) == false) {
    AgLog("PMS failed");
    return false;
  }
#else
  this->_serial->begin(9600);
  if (pms.begin(this->_serial) == false) {
    AgLog("PMS failed");
    return false;
  }
#endif
  _ver = pms.getFirmwareVersion();
  this->_isBegin = true;
  return true;
}

/**
 * @brief Read PM1.0
 *
 * @return int PM1.0 index (atmospheric environment)
 */
int PMS5003::getPm01Ae(void) { return pms.getPM0_1(); }

/**
 * @brief Read PM2.5
 *
 * @return int PM2.5 index (atmospheric environment)
 */
int PMS5003::getPm25Ae(void) { return pms.getPM2_5(); }

/**
 * @brief Read PM10.0
 *
 * @return int PM10.0 index (atmospheric environment)
 */
int PMS5003::getPm10Ae(void) { return pms.getPM10(); }

/**
 * @brief Read PM1.0
 *
 * @return int PM1.0 index (standard particle)
 */
int PMS5003::getPm01Sp(void) { return pms.getRaw0_1(); }

/**
 * @brief Read PM2.5
 *
 * @return int PM2.5 index (standard particle)
 */
int PMS5003::getPm25Sp(void) { return pms.getRaw2_5(); }

/**
 * @brief Read PM10
 *
 * @return int PM10 index (standard particle)
 */
int PMS5003::getPm10Sp(void) { return pms.getRaw10(); }

/**
 * @brief Read particle 0.3 count
 *
 * @return int PM0.3 index
 */
int PMS5003::getPm03ParticleCount(void) {
  return pms.getCount0_3();
}

/**
 * @brief Read particle 1.0 count
 *
 * @return int particle 1.0 count index
 */
int PMS5003::getPm01ParticleCount(void) { return pms.getCount1_0(); }

/**
 * @brief Read particle 0.5 count
 *
 * @return int particle 0.5 count index
 */
int PMS5003::getPm05ParticleCount(void) { return pms.getCount0_5(); }

/**
 * @brief Read particle 2.5 count
 *
 * @return int particle 2.5 count index
 */
int PMS5003::getPm25ParticleCount(void) { return pms.getCount2_5(); }

/**
 * @brief Read particle 5.0 count
 *
 * @return int particle 5.0 count index
 */
int PMS5003::getPm5ParticleCount(void) { return pms.getCount5_0(); }

/**
 * @brief Read particle 10 count
 *
 * @return int particle 10 count index
 */
int PMS5003::getPm10ParticleCount(void) { return pms.getCount10(); }

/**
 * @brief Convert PM2.5 to US AQI
 *
 * @param pm25 PM2.5 index
 * @return int PM2.5 US AQI
 */
int PMS5003::convertPm25ToUsAqi(int pm25) { return pms.pm25ToAQI(pm25); }

float PMS5003::slrCorrection(float pm25, float pm003Count, float scalingFactor, float intercept) {
  return pms.slrCorrection(pm25, pm003Count, scalingFactor, intercept);
}

/**
 * @brief Correct PM2.5
 *
 * Reference formula: https://www.airgradient.com/documentation/correction-algorithms/
 *
 * @param pm25 PM2.5 raw value
 * @param humidity Humidity value
 * @return compensated value in float
 */
float PMS5003::compensate(float pm25, float humidity) { return pms.compensate(pm25, humidity); }

/**
 * @brief Get sensor firmware version
 *
 * @return int
 */
int PMS5003::getFirmwareVersion(void) { return _ver; }

/**
 * @brief Get sensor error code
 *
 * @return uint8_t
 */
uint8_t PMS5003::getErrorCode(void) { return pms.getErrorCode(); }

/**
 * @brief Is sensor connect with device
 *
 * @return true Connected
 * @return false Removed
 */
bool PMS5003::connected(void) { return pms.connected(); }

/**
 * @brief Check device initialized or not
 *
 * @return true Initialized
 * @return false No-initialized
 */
bool PMS5003::isBegin(void) {
  if (this->_isBegin == false) {
    AgLog("Not-initialized");
    return false;
  }
  return true;
}

/**
 * @brief De-initialize sensor
 */
void PMS5003::end(void) {
  if (_isBegin == false) {
    return;
  }
  _isBegin = false;
#if defined(ESP8266)
  _debugStream = NULL;
#else
  delete _serial;
#endif
  AgLog("De-initialize");
}

/**
 * @brief Check and read PMS sensor data. This method should be callack from
 * loop process to continoue check sensor data if it's available
 */
void PMS5003::handle(void) { pms.readPackage(this->_serial); }

void PMS5003::updateFailCount(void) {
  pms.updateFailCount();
}

void PMS5003::resetFailCount(void) {
  pms.resetFailCount();
}

/**
 * @brief Get number of fail count
 *
 * @return int
 */
int PMS5003::getFailCount(void) { return pms.getFailCount(); }

/**
 * @brief Get number of fail count max
 *
 * @return int
 */
int PMS5003::getFailCountMax(void) { return pms.getFailCountMax(); }
