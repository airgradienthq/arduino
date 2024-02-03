#include "pms5003.h"
#include "Arduino.h"

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
  if (this->_isInit) {
    return true;
  }

#if defined(ESP32)
  // if (this->_serial != &Serial) {
  //   AgLog("Hardware serial must be Serial(0)");
  //   return false;
  // }
#endif

  this->bsp = getBoardDef(this->_boardDef);
  if (bsp == NULL) {
    AgLog("Board [%d] not supported", this->_boardDef);
    return false;
  }

  if (bsp->PMS5003.supported == false) {
    AgLog("Board [%d] PMS50035003 not supported", this->_boardDef);
    return false;
  }

#if defined(ESP8266)
  bsp->PMS5003.uart_tx_pin;
  SoftwareSerial *uart =
      new SoftwareSerial(bsp->PMS5003.uart_tx_pin, bsp->PMS5003.uart_rx_pin);
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

  this->_isInit = true;
  return true;
}

/**
 * @brief Convert PM2.5 to US AQI
 *
 * @param pm02
 * @return int
 */
int PMS5003::pm25ToAQI(int pm02) {
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

/**
 * @brief Read all package data then call to @ref getPMxxx to get the target
 * data
 *
 * @return true Success
 * @return false Failure
 */
bool PMS5003::readData(void) {
  if (this->checkInit() == false) {
    return false;
  }

  return pms.readUntil(pmsData);
}

/**
 * @brief Read PM1.0 must call this function after @ref readData success
 *
 * @return int PM1.0 index
 */
int PMS5003::getPm01Ae(void) { return pmsData.PM_AE_UG_1_0; }

/**
 * @brief Read PM2.5 must call this function after @ref readData success
 *
 * @return int PM2.5 index
 */
int PMS5003::getPm25Ae(void) { return pmsData.PM_AE_UG_2_5; }

/**
 * @brief Read PM10.0 must call this function after @ref readData success
 *
 * @return int PM10.0 index
 */
int PMS5003::getPm10Ae(void) { return pmsData.PM_AE_UG_10_0; }

/**
 * @brief Read PM3.0 must call this function after @ref readData success
 *
 * @return int PM3.0 index
 */
int PMS5003::getPm03ParticleCount(void) { return pmsData.PM_RAW_0_3; }

/**
 * @brief Convert PM2.5 to US AQI
 *
 * @param pm25 PM2.5 index
 * @return int PM2.5 US AQI
 */
int PMS5003::convertPm25ToUsAqi(int pm25) { return this->pm25ToAQI(pm25); }

/**
 * @brief Check device initialized or not
 *
 * @return true Initialized
 * @return false No-initialized
 */
bool PMS5003::checkInit(void) {
  if (this->_isInit == false) {
    AgLog("No initialized");
    return false;
  }
  return true;
}
