#include "Sht.h"

#include "../Libraries/arduino-sht/SHTSensor.h"
#include "../Main/utils.h"

/** Cast _sensor to SHTSensor */
#define shtSensor() ((SHTSensor *)(this->_sensor))

/**
 * @brief Check that is sensor initialized
 *
 * @return true Initialized
 * @return false Not-initialized
 */
bool Sht::isBegin(void) {
  if (this->_isBegin) {
    return true;
  }
  AgLog("Sensor not-initialized");
  return false;
}

/**
 * @brief Check board is support I2C to work with this sensor
 *
 * @return true Supported
 * @return false Not supported
 */
bool Sht::boardSupported(void) {
  if (this->_bsp == NULL) {
    this->_bsp = getBoardDef(this->_boardType);
  }

  if ((this->_bsp == NULL) || (this->_bsp->I2C.supported == false)) {
    AgLog("Board not supported");
    return false;
  }
  return true;
}

/**
 * @brief Construct a new Sht:: Sht object
 *
 * @param type
 */
Sht::Sht(BoardType type) : _boardType(type) {}

/**
 * @brief Destroy the Sht:: Sht object
 *
 */
Sht::~Sht() {}

#if defined(ESP8266)
/**
 * @brief Init sensor, Ifthis funciton not call the other funtion call will
 * always return false or value invalid
 *
 * @param wire wire TwoWire instance, Must be initialized
 * @param debugStream Point to debug Serial to print debug log
 * @return true Sucecss
 * @return false Failure
 */
bool Sht::begin(TwoWire &wire, Stream &debugStream) {
  _debugStream = &debugStream;
  return begin(wire);
}
#else
#endif
/**
 * @brief Initialize sensor, should be init sensor before use other API, if not
 * return result always failed
 *
 * @param wire TwoWire instance, Must be initialized
 * @return true Success
 * @return false Failure
 */
bool Sht::begin(TwoWire &wire) {
  if (_isBegin) {
    AgLog("Initialized, call end() then try again");
    return true;
  }

  if (boardSupported() == false) {
    return false;
  }

  /** Create new sensor */
  _sensor = new SHTSensor();
  if (_sensor == nullptr) {
    AgLog("Create SHTSensor failed");
    return false;
  }

  /** Initialize sensor */
  if (shtSensor()->init(wire) == false) {
    AgLog("Initialize SHTSensor failed");
    return false;
  }

  // Only supported by SHT3x
  if (shtSensor()->setAccuracy(SHTSensor::SHT_ACCURACY_MEDIUM) == false) {
    AgLog("Configure sensor failed");
    return false;
  }

  AgLog("Initialize");
  _isBegin = true;
  return true;
}

/**
 * @brief De-initialize sht sensor
 *
 */
void Sht::end(void) {
  if (_isBegin == false) {
    return;
  }
  delete shtSensor();
  _isBegin = false;
#if defined(ESP8266)
  _debugStream = nullptr;
#else
#endif
  _isBegin = false;
  AgLog("De-Initialize");
}

/**
 * @brief Get temprature degree celcius
 *
 * @return float
 */
float Sht::getTemperature(void) {
  return shtSensor()->getTemperature();
}

/**
 * @brief Get humidity
 *
 * @return float
 */
float Sht::getRelativeHumidity(void) {
  return shtSensor()->getHumidity();
}

/**
 * @brief Measure temperature and humidity
 *
 * @return true Success
 * @return false Failure
 */
bool Sht::measure(void) { return shtSensor()->readSample(); }
