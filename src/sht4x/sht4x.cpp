#include "sht4x.h"
#include "../library/SensirionSHT4x/src/SensirionI2CSht4x.h"

/** Cast _sensor to SensirionI2CSht4x */
#define shtSensor() ((SensirionI2CSht4x *)(this->_sensor))

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
  this->_debugStream = &debugStream;
  return this->begin(wire);
}
#else
#endif

/**
 * @brief Construct a new Sht:: Sht object
 * 
 * @param type Board type @ref BoardType
 */
Sht::Sht(BoardType type) : _boardType(type) {}

/**
 * @brief Init sensor, Ifthis funciton not call the other funtion call will
 * always return false or value invalid
 *
 * @param wire TwoWire instance, Must be initialized
 * @return true Success
 * @return false Failure
 */
bool Sht::begin(TwoWire &wire) {
  /** Ignore next step if sensor has intiialized */
  if (this->_isBegin) {
    AgLog("Initialized, call end() then try again");
    return true;
  }

  /** Check sensor has supported on board */
  if (this->boardSupported() == false) {
    return false;
  }

  /** Create new SensirionI2CSht4x and init */
  this->_sensor = new SensirionI2CSht4x();
  shtSensor()->begin(wire, SHT40_I2C_ADDR_44);
  if (shtSensor()->softReset() != 0) {
    AgLog("Reset sensor fail, look like sensor is not on I2C bus");
    return false;
  }

  delay(10);

  this->_isBegin = true;
  AgLog("Initialize");
  return true;
}

/**
 * @brief De-initialize SHT41 sensor
 * 
 */
void Sht::end(void) {
  if (this->_isBegin == false) {
    return;
  }

  this->_isBegin = false;
  _bsp = NULL;
  delete shtSensor();
#if defined(ESP8266)
  _debugStream = nullptr;
#endif
AgLog("De-initialize");
}

/**
 * @brief Get temperature degrees celsius
 *
 * @return float value <= 256.0f is invalid, That mean sensor has issue or
 * communication to sensor not worked as well.
 */
float Sht::getTemperature(void) {
  float temperature;
  float humidity;
  if (this->measureMediumPrecision(temperature, humidity)) {
    return temperature;
  }

  return -256.0f;
}

/**
 * @brief Get humidity
 *
 * @return float Percent(0 - 100), value < 0 is invalid.
 */
float Sht::getRelativeHumidity(void) {
  float temperature;
  float humidity;
  if (this->measureMediumPrecision(temperature, humidity)) {
    return humidity;
  }

  return -1.0f;
}

/**
 * @brief Check sensor has supported by board
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
 * @brief Check that sensor has initialized
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
 * @brief Ge SHT41 temperature and humidity value with medium meaure precision
 * 
 * @param temperature Read out temperarure
 * @param humidity Read humidity
 * @return true Success
 * @return false Failure
 */
bool Sht::measureMediumPrecision(float &temperature, float &humidity) {
  if (this->isBegin() == false) {
    return false;
  }

  if (shtSensor()->measureMediumPrecision(temperature, humidity) == 0) {
    return true;
  }
  return false;
}

