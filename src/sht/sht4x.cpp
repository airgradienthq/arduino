#include "sht4x.h"
#include "../library/SensirionSHT4x/src/SensirionI2CSht4x.h"

#define shtSensor() ((SensirionI2CSht4x *)(this->_sensor))

#if defined(ESP8266)
bool Sht::begin(TwoWire &wire, Stream &debugStream) {
  this->_debugStream = &debugStream;
  return this->begin(wire);
}
#else

#endif

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
  if (this->_isInit) {
    return true;
  }

  if (this->boardSupported() == false) {
    return false;
  }
  this->_sensor = new SensirionI2CSht4x();
  shtSensor()->begin(wire, SHT40_I2C_ADDR_44);
  if (shtSensor()->softReset() != 0) {
    AgLog("Reset sensor fail, look like sensor is not on I2C bus");
    return false;
  }

  delay(10);

  this->_isInit = true;
  AgLog("Init");
  return true;
}

void Sht::end(void) {
  if (this->_isInit == false) {
    return;
  }

  this->_isInit = false;
  delete shtSensor();
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

int Sht::sdaPin(void) {
  if (this->_bsp) {
    return this->_bsp->I2C.sda_pin;
  }
  AgLog("sdaPin(): board not supported I2C");
  return -1;
}

int Sht::sclPin(void) {
  if (this->_bsp) {
    return this->_bsp->I2C.scl_pin;
  }
  AgLog("sdlPin(): board not supported I2C");
  return -1;
}

bool Sht::checkInit(void) {
  if (this->_isInit) {
    return true;
  }
  AgLog("Sensor no-initialized");
  return false;
}

bool Sht::measureHighPrecision(float &temperature, float &humidity) {
  if (this->checkInit() == false) {
    return false;
  }

  if (shtSensor()->measureHighPrecision(temperature, humidity) == 0) {
    return true;
  }
  return false;
}

bool Sht::measureMediumPrecision(float &temperature, float &humidity) {
  if (this->checkInit() == false) {
    return false;
  }

  if (shtSensor()->measureMediumPrecision(temperature, humidity) == 0) {
    return true;
  }
  return false;
}

bool Sht::measureLowestPrecision(float &temperature, float &humidity) {
  if (this->checkInit() == false) {
    return false;
  }

  if (shtSensor()->measureLowestPrecision(temperature, humidity) == 0) {
    return true;
  }
  return false;
}

bool Sht::activateHighestHeaterPowerShort(float &temperature, float &humidity) {
  if (this->checkInit() == false) {
    return false;
  }

  if (shtSensor()->activateHighestHeaterPowerShort(temperature, humidity) ==
      0) {
    return true;
  }
  return false;
}

bool Sht::activateMediumHeaterPowerLong(float &temperature, float &humidity) {
  if (this->checkInit() == false) {
    return false;
  }

  if (shtSensor()->activateMediumHeaterPowerLong(temperature, humidity) == 0) {
    return true;
  }
  return false;
}

bool Sht::activateLowestHeaterPowerLong(float &temperature, float &humidity) {
  if (this->checkInit() == false) {
    return false;
  }

  if (shtSensor()->activateLowestHeaterPowerLong(temperature, humidity) == 0) {
    return true;
  }
  return false;
}

bool Sht::getSerialNumber(uint32_t &serialNumber) {
  if (this->checkInit() == false) {
    return false;
  }

  if (shtSensor()->serialNumber(serialNumber) == 0) {
    return true;
  }
  return false;
}

bool Sht::softReset(void) {
  if (this->checkInit() == false) {
    return false;
  }

  if (shtSensor()->softReset() == 0) {
    return true;
  }
  return false;
}
