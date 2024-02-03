#include "sgp41.h"
#include "../library/SensirionSGP41/src/SensirionI2CSgp41.h"
#include "../library/Sensirion_Gas_Index_Algorithm/src/NOxGasIndexAlgorithm.h"
#include "../library/Sensirion_Gas_Index_Algorithm/src/VOCGasIndexAlgorithm.h"

#define sgpSensor() ((SensirionI2CSgp41 *)(this->_sensor))
#define vocAlgorithm() ((VOCGasIndexAlgorithm *)(this->_vocAlgorithm))
#define noxAlgorithm() ((NOxGasIndexAlgorithm *)(this->_noxAlgorithm))

Sgp41::Sgp41(BoardType type) : _boardType(type) {}

/**
 * @brief Sensor intialize, if initialize fail, other function call always
 * return false or invalid valid
 *
 * @param wire TwoWire instance
 * @return true Success
 * @return false Failure
 */
bool Sgp41::begin(TwoWire &wire) {
  if (this->_isInit) {
    return true;
  }
  if (this->boardSupported() == false) {
    return false;
  }

  /** Init algorithm */
  _vocAlgorithm = new VOCGasIndexAlgorithm();
  _noxAlgorithm = new NOxGasIndexAlgorithm();

  /** Init sensor */
  this->_sensor = new SensirionI2CSgp41();
  sgpSensor()->begin(wire);

  uint16_t testResult;
  if (sgpSensor()->executeSelfTest(testResult) != 0) {
    return false;
  }
  if (testResult != 0xD400) {
    AgLog("Self-test failed");
    return false;
  }

#ifdef ESP32
  /** Create task */
  xTaskCreate(
      [](void *param) {
        Sgp41 *sgp = static_cast<Sgp41 *>(param);
        sgp->_handle();
      },
      "sgp_poll", 2048, this, 5, &pollTask);
#else
  conditioningPeriod = millis();
  conditioningCount = 0;
#endif

  this->_isInit = true;
  AgLog("Init");
  return true;
}

#if defined(ESP8266)
bool Sgp41::begin(TwoWire &wire, Stream &stream) {
  this->_debugStream = &stream;
  return this->begin(wire);
}
void Sgp41::handle(void) {
  uint32_t ms = (uint32_t)(millis() - conditioningPeriod);
  if (ms >= 1000) {
    conditioningPeriod = millis();
    if (onConditioning) {
      if (_noxConditioning() == false) {
        AgLog("SGP on conditioning failed");
      }
      conditioningCount++;
      if (conditioningCount >= 10) {
        onConditioning = false;
      }
    } else {
      uint16_t srawVoc, srawNox;
      if (getRawSignal(srawVoc, srawNox)) {
        nox = noxAlgorithm()->process(srawNox);
        tvoc = vocAlgorithm()->process(srawVoc);
        AgLog("Polling SGP41 success: tvoc: %d, nox: %d", tvoc, nox);
      }
    }
  }
}

#else 
void Sgp41::_handle(void) {
  /** NOx conditionning */
  uint16_t err;
  for (int i = 0; i < 10; i++) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (_noxConditioning() == false) {
      AgLog("SGP on conditioning failed");
      vTaskDelete(NULL);
    }
  }

  onConditioning = false;
  AgLog("Conditioning finish");

  uint16_t srawVoc, srawNox;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (getRawSignal(srawVoc, srawNox)) {
      nox = noxAlgorithm()->process(srawNox);
      tvoc = vocAlgorithm()->process(srawVoc);
      AgLog("Polling SGP41 success: tvoc: %d, nox: %d", tvoc, nox);
    }
  }
}
#endif

void Sgp41::end(void) {
  if (this->_isInit == false) {
    return;
  }

#ifdef ESP32
  vTaskDelete(pollTask);
#endif
  this->_isInit = false;
  delete sgpSensor();

  AgLog("De-Init");
}

/**
 * @brief Get TVOC index
 *
 * @return int -1 if invalid
 */
int Sgp41::getTvocIndex(void) {
  if (onConditioning) {
    return -1;
  }
  return tvoc;
}

/**
 * @brief Get NOX index
 *
 * @return int -1 if invalid
 */
int Sgp41::getNoxIndex(void) {
  if (onConditioning) {
    return -1;
  }
  return nox;
}

bool Sgp41::boardSupported(void) {
  if (this->bsp == nullptr) {
    this->bsp = getBoardDef(this->_boardType);
  }

  if ((this->bsp == nullptr) || (this->bsp->I2C.supported == false)) {
    AgLog("Board not supported");
    return false;
  }
  return true;
}

int Sgp41::sdaPin(void) {
  if (this->bsp) {
    return this->bsp->I2C.sda_pin;
  }
  AgLog("sdaPin(): board not supported I2C");
  return -1;
}

int Sgp41::sclPin(void) {
  if (this->bsp) {
    return this->bsp->I2C.scl_pin;
  }
  AgLog("sdlPin(): board not supported I2C");
  return -1;
}

bool Sgp41::getRawSignal(uint16_t &raw_voc, uint16_t &row_nox,
                         uint16_t defaultRh, uint16_t defaultT) {
  if (this->checkInit() == false) {
    return false;
  }

  if (sgpSensor()->measureRawSignals(defaultRh, defaultT, raw_voc, row_nox) ==
      0) {
    return true;
  }
  return false;
}

/**
 * @brief This command turns the hotplate off and stops the measurement.
 *        Subsequently, the sensor enters the idle mode.
 *
 * @return true Success
 * @return false Failure
 */
bool Sgp41::turnHeaterOff(void) {
  if (this->checkInit() == false) {
    return false;
  }

  if (sgpSensor()->turnHeaterOff() == 0) {
    return true;
  }
  return false;
}

bool Sgp41::getSerialNumber(uint16_t serialNumbers[],
                            uint8_t serialNumberSize) {
  if (this->checkInit() == false) {
    return false;
  }

  if (sgpSensor()->getSerialNumber(serialNumbers) == 0) {
    return true;
  }
  return false;
}

bool Sgp41::checkInit(void) {
  if (this->_isInit) {
    return true;
  }
  AgLog("Sensor no-initialized");
  return false;
}

bool Sgp41::_noxConditioning(void) {
  uint16_t err;
  uint16_t srawVoc;
  err = sgpSensor()->executeConditioning(defaultRh, defaultT, srawVoc);
  return (err == 0);
}
