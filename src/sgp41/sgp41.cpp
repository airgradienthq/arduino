#include "sgp41.h"
#include "../library/SensirionSGP41/src/SensirionI2CSgp41.h"
#include "../library/Sensirion_Gas_Index_Algorithm/src/NOxGasIndexAlgorithm.h"
#include "../library/Sensirion_Gas_Index_Algorithm/src/VOCGasIndexAlgorithm.h"

#define sgpSensor() ((SensirionI2CSgp41 *)(this->_sensor))
#define vocAlgorithm() ((VOCGasIndexAlgorithm *)(this->_vocAlgorithm))
#define noxAlgorithm() ((NOxGasIndexAlgorithm *)(this->_noxAlgorithm))

/**
 * @brief Construct a new Sgp 4 1:: Sgp 4 1 object
 *
 * @param type Board type @ref BoardType
 */
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
  /** Ignore next step if initialized */
  if (this->_isBegin) {
    AgLog("Initialized, call end() then try again");
    return true;
  }

  /** Check that board has supported this sensor */
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

  this->_isBegin = true;
  AgLog("Initialize");
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
/**
 * @brief Handle the sensor conditioning and run time udpate value, This method
 * must not call, it's called on private task
 */
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

/**
 * @brief De-Initialize sensor
 */
void Sgp41::end(void) {
  if (this->_isBegin == false) {
    return;
  }

#ifdef ESP32
  vTaskDelete(pollTask);
#else
  _debugStream = nullptr;
#endif
  bsp = NULL;
  this->_isBegin = false;
  delete sgpSensor();
  delete vocAlgorithm();
  delete noxAlgorithm();

  AgLog("De-initialize");
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

/**
 * @brief Check that board has supported sensor
 *
 * @return true Supported
 * @return false Not-supported
 */
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

/**
 * @brief Get raw signal
 *
 * @param raw_voc Raw VOC output
 * @param row_nox Raw NOx output
 * @param defaultRh
 * @param defaultT
 * @return true Success
 * @return false Failure
 */
bool Sgp41::getRawSignal(uint16_t &raw_voc, uint16_t &row_nox,
                         uint16_t defaultRh, uint16_t defaultT) {
  if (this->isBegin() == false) {
    return false;
  }

  if (sgpSensor()->measureRawSignals(defaultRh, defaultT, raw_voc, row_nox) ==
      0) {
    return true;
  }
  return false;
}

/**
 * @brief Check that sensor is initialized
 *
 * @return true Initialized
 * @return false Not-initialized
 */
bool Sgp41::isBegin(void) {
  if (this->_isBegin) {
    return true;
  }
  AgLog("Sensor not-initialized");
  return false;
}

/**
 * @brief Handle nox conditioning process
 *
 * @return true Success
 * @return false Failure
 */
bool Sgp41::_noxConditioning(void) {
  uint16_t err;
  uint16_t srawVoc;
  err = sgpSensor()->executeConditioning(defaultRh, defaultT, srawVoc);
  return (err == 0);
}
