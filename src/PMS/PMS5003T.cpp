#include "PMS5003T.h"
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
bool PMS5003T::begin(Stream *_debugStream) {
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
bool PMS5003T::begin(HardwareSerial &serial) {
  this->_serial = &serial;
  return this->begin();
}
#endif

/**
 * @brief Construct a new PMS5003T::PMS5003T object
 *
 * @param def Board type @ref BoardType
 */
PMS5003T::PMS5003T(BoardType def) : _boardDef(def) {}

/**
 * @brief Init sensor
 *
 * @return true Success
 * @return false Failure
 */
bool PMS5003T::begin(void) {
  if (this->_isBegin) {
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

  if (bsp->Pms5003.supported == false) {
    AgLog("Board [%d] PMS5003 not supported", this->_boardDef);
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

#if ARDUINO_USB_CDC_ON_BOOT // Serial used for USB CDC
  if (this->_serial == &Serial0) {
    AgLog("Init Serial0");
    _serial->begin(9600, SERIAL_8N1);
#else
  if (this->_serial == &Serial) {
    AgLog("Init Serial");
    this->_serial->begin(9600, SERIAL_8N1, bsp->Pms5003.uart_rx_pin,
                         bsp->Pms5003.uart_tx_pin);
#endif
  } else {
    /** Share with sensor air s8*/
    if (bsp->SenseAirS8.supported == false) {
      AgLog("Board [%d] PMS5003T_2 not supported", this->_boardDef);
      return false;
    }

    AgLog("Init Serialx");
    this->_serial->begin(9600, SERIAL_8N1, bsp->SenseAirS8.uart_rx_pin,
                         bsp->SenseAirS8.uart_tx_pin);
  }
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
int PMS5003T::getPm01Ae(void) { return pms.getPM0_1(); }

/**
 * @brief Read PM2.5
 *
 * @return int PM2.5 index (atmospheric environment)
 */
int PMS5003T::getPm25Ae(void) { return pms.getPM2_5(); }

/**
 * @brief Read PM10.0
 *
 * @return int PM10.0 index (atmospheric environment)
 */
int PMS5003T::getPm10Ae(void) { return pms.getPM10(); }

/**
 * @brief Read PM1.0
 *
 * @return int PM1.0 index (standard particle)
 */
int PMS5003T::getPm01Sp(void) { return pms.getRaw0_1(); }

/**
 * @brief Read PM2.5
 *
 * @return int PM2.5 index (standard particle)
 */
int PMS5003T::getPm25Sp(void) { return pms.getRaw2_5(); }

/**
 * @brief Read PM10
 *
 * @return int PM10 index (standard particle)
 */
int PMS5003T::getPm10Sp(void) { return pms.getRaw10(); }

/**
 * @brief Read particle 0.3 count
 *
 * @return int particle 0.3 count index
 */
int PMS5003T::getPm03ParticleCount(void) {
  return pms.getCount0_3();
}

/**
 * @brief Read particle 0.5 count
 *
 * @return int particle 0.5 count index
 */
int PMS5003T::getPm05ParticleCount(void) { return pms.getCount0_5(); }

/**
 * @brief Read particle 1.0 count
 *
 * @return int particle 1.0 count index
 */
int PMS5003T::getPm01ParticleCount(void) { return pms.getCount1_0(); }

/**
 * @brief Read particle 2.5 count
 *
 * @return int particle 2.5 count index
 */
int PMS5003T::getPm25ParticleCount(void) { return pms.getCount2_5(); }

/**
 * @brief Convert PM2.5 to US AQI
 *
 * @param pm25 PM2.5 index
 * @return int PM2.5 US AQI
 */
int PMS5003T::convertPm25ToUsAqi(int pm25) { return pms.pm25ToAQI(pm25); }

/**
 * @brief Get temperature, Must call this method after @ref readData() success
 *
 * @return float Degree Celcius
 */
float PMS5003T::getTemperature(void) {
  return pms.getTemp() / 10.0f;
}

/**
 * @brief Get humidity, Must call this method after  @ref readData() success
 *
 * @return float Percent (%)
 */
float PMS5003T::getRelativeHumidity(void) {
  return pms.getHum() / 10.0f;
}

/**
 * @brief Correct PM2.5
 *
 * Reference formula: https://www.airgradient.com/documentation/correction-algorithms/
 *
 * @param pm25 PM2.5 raw value
 * @param humidity Humidity value
 * @return compensated value
 */
float PMS5003T::compensate(float pm25, float humidity) { return pms.compensate(pm25, humidity); }

/**
 * @brief Get module(s) firmware version
 *
 * @return int Version code
 */
int PMS5003T::getFirmwareVersion(void) { return _ver; }

/**
 * @brief Get sensor error code
 *
 * @return uint8_t
 */
uint8_t PMS5003T::getErrorCode(void) { return pms.getErrorCode(); }

/**
 * @brief Is sensor connect to device
 *
 * @return true Connected
 * @return false Removed
 */
bool PMS5003T::connected(void) { return pms.connected(); }

/**
 * @brief Check device initialized or not
 *
 * @return true Initialized
 * @return false No-initialized
 */
bool PMS5003T::isBegin(void) {
  if (this->_isBegin == false) {
    AgLog("Not-initialized");
    return false;
  }
  return true;
}

void PMS5003T::end(void) {
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
void PMS5003T::handle(void) { pms.readPackage(this->_serial); }

void PMS5003T::updateFailCount(void) {
  pms.updateFailCount();
}

void PMS5003T::resetFailCount(void) {
  pms.resetFailCount();
}

/**
 * @brief Get fail count
 *
 * @return int
 */
int PMS5003T::getFailCount(void) { return pms.getFailCount(); }

/**
 * @brief Get fail count max
 *
 * @return int
 */
int PMS5003T::getFailCountMax(void) { return pms.getFailCountMax(); }
