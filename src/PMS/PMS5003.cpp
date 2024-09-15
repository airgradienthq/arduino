#include "PMS5003.h"
#include "Arduino.h"
#include "../Main/utils.h"

#if defined(ESP8266)
#include <SoftwareSerial.h>
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
 * @brief Init sensor
 *
 * @return true Success
 * @return false Failure
 */
bool PMS5003::begin(void) {
  if (this->_isBegin) {
    AgLog("Initialized, call end() then try again");
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
  bsp->Pms5003.uart_tx_pin;
  SoftwareSerial *uart =
      new SoftwareSerial(bsp->Pms5003.uart_tx_pin, bsp->Pms5003.uart_rx_pin);
  uart->begin(9600);
  if (pms.begin(uart) == false) {
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
 * @brief Read PM1.0 must call this function after @ref readData success
 *
 * @return int PM1.0 index
 */
int PMS5003::getPm01Ae(void) { return pms.getPM0_1(); }

/**
 * @brief Read PM2.5 must call this function after @ref readData success
 *
 * @return int PM2.5 index
 */
int PMS5003::getPm25Ae(void) { return pms.getPM2_5(); }

/**
 * @brief Read PM10.0 must call this function after @ref readData success
 *
 * @return int PM10.0 index
 */
int PMS5003::getPm10Ae(void) { return pms.getPM10(); }

/**
 * @brief Read PM0.3 must call this function after @ref readData success
 *
 * @return int PM0.3 index
 */
int PMS5003::getPm03ParticleCount(void) {
  return pms.getCount0_3();
}

/**
 * @brief Convert PM2.5 to US AQI
 *
 * @param pm25 PM2.5 index
 * @return int PM2.5 US AQI
 */
int PMS5003::convertPm25ToUsAqi(int pm25) { return pms.pm25ToAQI(pm25); }

/**
 * @brief Correct PM2.5
 * 
 * Reference formula: https://www.airgradient.com/documentation/correction-algorithms/
 * 
 * @param pm25 PM2.5 raw value
 * @param humidity Humidity value
 * @return int 
 */
int PMS5003::compensate(int pm25, float humidity) {
  return pms.compensate(pm25, humidity);
}

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
void PMS5003::handle(void) { pms.handle(); }

/**
 * @brief Get sensor status
 * 
 * @return true No problem
 * @return false Communication timeout or sensor has removed
 */
bool PMS5003::isFailed(void) { return pms.isFailed(); }

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
