#include "sht3x.h"
#include "../library/arduino-i2c-sht3x/src/SensirionI2cSht3x.h"

#define sht3x() ((SensirionI2cSht3x *)(this->_sensor))

/**
 * @brief Check that sensor has initialized
 *
 * @return true Initialized
 * @return false Not-initialized
 */
bool Sht3x::isBegin(void) {
  if (_isBegin) {
    return true;
  }
  AgLog("Sensor not-initialized");
  return false;
}

/**
 * @brief Check sensor has supported by board
 *
 * @return true Supported
 * @return false Not-supported
 */
bool Sht3x::boardSupported(void) {
  if (_bsp == NULL) {
    _bsp = getBoardDef(_boarType);
  }

  if ((_bsp == NULL) || (_bsp->I2C.supported == false)) {
    AgLog("Board not supported");
    return false;
  }
  return true;
}

/**
 * @brief Get temperature and humidity data
 *
 * @param temp Tempreature read out
 * @param hum Humidity read out
 * @return true Success
 * @return false Failure
 */
bool Sht3x::measure(float &temp, float &hum) {
  if (isBegin() == false) {
    return false;
  }

  if (sht3x()->measureSingleShot(REPEATABILITY_MEDIUM, false, temp, hum) ==
      NO_ERROR) {
    return true;
  }
  return false;
}

/**
 * @brief Construct a new Sht 3x:: Sht 3x object
 *
 * @param type
 */
Sht3x::Sht3x(BoardType type) : _boarType(type) {}

/**
 * @brief Destroy the Sht 3x:: Sht 3x object
 *
 */
Sht3x::~Sht3x() { end(); }

#ifdef ESP8266
/**
 * @brief Initialized sensor
 *
 * @param wire TwoWire instance, must be initialized
 * @param debugStream Point to debug Serial to print debug log
 * @return true Success
 * @return false Failure
 */
bool Sht3x::begin(TwoWire &wire, Stream &debugStream) {
  _debugStream = &debugStream;
  return begin(wire);
}
#else
#endif

/**
 * @brief Init sensor, should init before use sensor, if not call other method
 * always return invalid
 *
 * @param wire TwoWire instance, must be initialized
 * @return true Success
 * @return false Failure
 */
bool Sht3x::begin(TwoWire &wire) {
  if (_isBegin) {
    AgLog("Initialized, call end() then try again");
    return true;
  }

  /** Check sensor has supported on board */
  if (boardSupported() == false) {
    return false;
  }

  /** Create sensor and init */
  _sensor = new SensirionI2cSht3x();
  sht3x()->begin(wire, SHT30_I2C_ADDR_44);
  if (sht3x()->softReset() != NO_ERROR) {
    AgLog("Reset sensor fail, look like sensor is not on I2C bus");
    return false;
  }

  _isBegin = true;
  AgLog("Initialize");
  return true;
}

/**
 * @brief De-initialize sensor
 *
 */
void Sht3x::end(void) {
  if (_isBegin == false) {
    return;
  }
  _isBegin = false;
  _bsp = NULL;
  delete sht3x();
#ifdef ESP8266
  _debugStream = nullptr;
#endif
  AgLog("De-initialize");
}

/**
 * @brief Get temperature degree celsius
 *
 * @return float value <= 256.0f is invalid, that mean sensor has issue or
 * communication to sensor not worked as well
 */
float Sht3x::getTemperature(void) {
  float temp;
  float hum;
  if (measure(temp, hum)) {
    return temp;
  }
  return -256.0f;
}

/**
 * @brief Get humidity
 *
 * @return float Percent(0 - 100), value < 0 is invalid.
 */
float Sht3x::getRelativeHumidity(void) {
  float temp;
  float hum;
  if (measure(temp, hum)) {
    return hum;
  }
  return -1.0f;
}
