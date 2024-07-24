#include "S8.h"
#include "mb_crc.h"
#include "../Main/utils.h"
#if defined(ESP8266)
#include <SoftwareSerial.h>
#else
#endif

/**
 * @brief Construct a new Sense Air S8:: Sense Air S8 object
 *
 * @param def
 */
S8::S8(BoardType def) : _boardDef(def) {}

#if defined(ESP8266)
/**
 * @brief Init sensor
 * @return true = success, otherwise is failure
 */
bool S8::begin(void) {
  if (this->_isBegin) {
    AgLog("Initialized, Call end() then try again");
    return true;
  }

  return this->_begin();
}

/**
 * @brief Init S8 sensor, this methos should be call before other, if not it's
 * always return the failure status
 *
 * @param _debugStream Serial print debug log, NULL if don't use
 * @return true = success, otherwise is failure
 */
bool S8::begin(Stream *_debugStream) {
  this->_debugStream = _debugStream;
  return this->begin();
}
#else
/**
 * @brief Init sensor
 *
 * @param serial Target Serial use for communication with sensor
 * @return true Success
 * @return false Failure
 */
bool S8::begin(HardwareSerial &serial) {
  this->_serial = &serial;
  return this->_begin();
}
#endif

/**
 * @brief De-Initialize sensor and release peripheral resource
 *
 */
void S8::end(void) {
  if (this->_isBegin == false) {
    return;
  }

  // Deinit
  AgLog("De-Inititlize");
}

/**
 * @brief Get sensor firmware version
 *
 * @param firmver String buffer, len = 10 char
 */
void S8::getFirmwareVersion(char firmver[]) {
  if (this->isBegin() == false) {
    return;
  }

  if (firmver == NULL) {
    return;
  }

  strcpy(firmver, "");

  // Ask software version
  sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR29, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {
    snprintf(firmver, S8_LEN_FIRMVER, "%0u.%0u", buf_msg[3], buf_msg[4]);
    AgLog("Firmware version: %s", firmver);
  } else {
    AgLog("Firmware version not available!");
  }
}

/**
 * @brief Get sensor type ID
 *
 * @return int32_t Return ID
 */
int32_t S8::getSensorTypeId(void) {
  if (this->isBegin() == false) {
    return utils::getInvalidCO2();
  }

  int32_t sensorType = 0;

  // Ask sensor type ID (high)
  sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR26, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {

    // Save sensor type ID (high)
    sensorType = (((int32_t)buf_msg[4] << 16) & 0x00FF0000);

    // Ask sensor type ID (low)
    sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR27, 0x0001);

    // Wait response
    memset(buf_msg, 0, S8_LEN_BUF_MSG);
    nb = uartReadBytes(7, S8_TIMEOUT);

    // Check response and get data
    if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {
      sensorType |=
          ((buf_msg[3] << 8) & 0x0000FF00) | (buf_msg[4] & 0x000000FF);
    } else {
      AgLog("Error getting sensor type ID (low)!");
    }
  } else {
    AgLog("Error getting sensor type ID (high)!");
  }

  return sensorType;
}

/**
 * @brief Get sensor ID
 *
 * @return int32_t ID
 */
int32_t S8::getSensorId(void) {
  if (this->isBegin() == false) {
    return -1;
  }

  int32_t sensorID = 0;

  // Ask sensor ID (high)
  sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR30, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {

    // Save sensor ID (high)
    sensorID = (((int32_t)buf_msg[3] << 24) & 0xFF000000) |
               (((int32_t)buf_msg[4] << 16) & 0x00FF0000);

    // Ask sensor ID (low)
    sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR31, 0x0001);

    // Wait response
    memset(buf_msg, 0, S8_LEN_BUF_MSG);
    nb = uartReadBytes(7, S8_TIMEOUT);

    // Check response and get data
    if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {
      sensorID |= ((buf_msg[3] << 8) & 0x0000FF00) | (buf_msg[4] & 0x000000FF);
    } else {
      AgLog("Error getting sensor ID (low)!");
    }
  } else {
    AgLog("Error getting sensor ID (high)!");
  }

  return sensorID;
}

/**
 * @brief Get memory map version
 *
 * @return int16_t
 */
int16_t S8::getMemoryMapVersion(void) {
  if (this->isBegin() == false) {
    return -1;
  }

  int16_t mmVersion = 0;

  // Ask memory map version
  sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR28, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {
    mmVersion = ((buf_msg[3] << 8) & 0xFF00) | (buf_msg[4] & 0x00FF);
    AgLog("Memory map version = %d", mmVersion);
  } else {
    AgLog("Error getting memory map version!");
  }

  return mmVersion;
}

/**
 * @brief Get CO2
 *
 * @return int16_t (PPM), -1 if invalid.
 */
int16_t S8::getCo2(void) {
  if (this->isBegin() == false) {
    return -1;
  }

  int16_t co2 = -1;

  // Ask CO2 value
  sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR4, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {
    co2 = ((buf_msg[3] << 8) & 0xFF00) | (buf_msg[4] & 0x00FF);
    AgLog("CO2 value = %d ppm", co2);
  } else {
    AgLog("Error getting CO2 value!");
  }

  return co2;
}

/**
 * @brief Calibration CO2 value to baseline 400(PPM)
 *
 * @return true Success
 * @return false Failure
 */

bool S8::setBaselineCalibration(void) {
  if (this->manualCalib()) {
    isCalib = true;
    return true;
  }
  return false;
}

/**
 * @brief Wait for background calibration done
 *
 * @return true Done
 * @return false On calib
 */
bool S8::isBaseLineCalibrationDone(void) {
  if (isCalib == false) {
    return true;
  }
  if (getAcknowledgement() & S8_MASK_CO2_BACKGROUND_CALIBRATION) {
    return true;
  }
  return false;
}

/**
 * @brief Get output PWM value
 *
 * @return int16_t PWM
 */
int16_t S8::getOutputPWM(void) {
  if (this->isBegin() == false) {
    return -1;
  }

  int16_t pwm = 0;

  // Ask PWM output
  sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR22, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {
    pwm = ((buf_msg[3] << 8) & 0xFF00) | (buf_msg[4] & 0x00FF);
    AgLog("PWM output (raw) = %d", pwm);
    AgLog("PWM output (to ppm, normal version) = %d PPM",
          (pwm / 16383.0) * 2000.0);
  } else {
    AgLog("Error getting PWM output!");
  }

  return pwm;
}

/**
 * @brief Get ABC calibration period
 *
 * @return int16_t Hour
 */
int16_t S8::getCalibPeriodABC(void) {
  if (this->isBegin() == false) {
    return -1;
  }

  int16_t period = 0;

  // Ask ABC period
  sendCommand(MODBUS_FUNC_READ_HOLDING_REGISTERS, MODBUS_HR32, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_HOLDING_REGISTERS, nb, 7)) {
    period = ((buf_msg[3] << 8) & 0xFF00) | (buf_msg[4] & 0x00FF);
    AgLog("ABC period: %d hour", period);
  } else {
    AgLog("Error getting ABC period!");
  }

  return period;
}

/**
 * @brief Set ABC calibration period
 *
 * @param period Hour, 4 - 4800 hour, 0 to disable
 * @return true Success
 * @return false Failure
 */
bool S8::setCalibPeriodABC(int16_t period) {
  if (this->isBegin() == false) {
    return false;
  }

  uint8_t buf_msg_sent[8];
  bool result = false;

  if (period >= 0 && period <= 4800) { // 0 = disable ABC algorithm

    // Ask set ABC period
    sendCommand(MODBUS_FUNC_WRITE_SINGLE_REGISTER, MODBUS_HR32, period);

    // Save bytes sent
    memcpy(buf_msg_sent, buf_msg, 8);

    // Wait response
    memset(buf_msg, 0, S8_LEN_BUF_MSG);
    uartReadBytes(8, S8_TIMEOUT);

    // Check response
    if (memcmp(buf_msg_sent, buf_msg, 8) == 0) {
      result = true;
      AgLog("Successful setting of ABC period");
    } else {
      AgLog("Error in setting of ABC period!");
    }
  } else {
    AgLog("Invalid ABC period!");
  }

  return result;
}

/**
 * @brief Manual calibration sensor, must be place sensor at clean environment
 * for 5 minute before calib
 *
 * @return true Success
 * @return false Failure
 */
bool S8::manualCalib(void) {
  if (this->isBegin() == false) {
    return false;
  }

  bool result = clearAcknowledgement();

  if (result) {
    result = sendSpecialCommand(S8_CO2_BACKGROUND_CALIBRATION);

    if (result) {
      AgLog("Manual calibration in background has started");
    } else {
      AgLog("Error starting manual calibration!");
    }
  }

  return result;
}

/**
 * @brief Get sensor acknowlegement flags
 *
 * @return int16_t Flags
 */
int16_t S8::getAcknowledgement(void) {
  if (this->isBegin() == false) {
    return -1;
  }

  int16_t flags = 0;

  // Ask acknowledgement flags
  sendCommand(MODBUS_FUNC_READ_HOLDING_REGISTERS, MODBUS_HR1, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_HOLDING_REGISTERS, nb, 7)) {
    flags = ((buf_msg[3] << 8) & 0xFF00) | (buf_msg[4] & 0x00FF);
  } else {
    AgLog("Error getting acknowledgement flags!");
  }

  return flags;
}

/**
 * @brief Clea acknowlegement flags
 *
 * @return true Success
 * @return false Failure
 */
bool S8::clearAcknowledgement(void) {
  if (this->isBegin() == false) {
    return false;
  }

  uint8_t buf_msg_sent[8];
  bool result = false;

  // Ask clear acknowledgement flags
  sendCommand(MODBUS_FUNC_WRITE_SINGLE_REGISTER, MODBUS_HR1, 0x0000);

  // Save bytes sent
  memcpy(buf_msg_sent, buf_msg, 8);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uartReadBytes(8, S8_TIMEOUT);

  // Check response
  if (memcmp(buf_msg_sent, buf_msg, 8) == 0) {
    result = true;
    AgLog("Successful clearing acknowledgement flags");
  } else {
    AgLog("Error clearing acknowledgement flags!");
  }

  return result;
}

/**
 * @brief Get sensor alarm status
 *
 * @return int16_t Alarm status
 */
int16_t S8::getAlarmStatus(void) {
  if (this->isBegin() == false) {
    return -1;
  }

  int16_t status = 0;

  // Ask alarm status
  sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR2, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {
    status = ((buf_msg[3] << 8) & 0xFF00) | (buf_msg[4] & 0x00FF);
  } else {
    AgLog("Error getting alarm status!");
  }

  return status;
}

/**
 * @brief Get sensor status
 *
 * @return S8::Status Sensor status
 */
S8::Status S8::getStatus(void) {
  if (this->isBegin() == false) {
    return (Status)0;
  }

  int16_t status = 0;

  // Ask meter status
  sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR1, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {
    status = ((buf_msg[3] << 8) & 0xFF00) | (buf_msg[4] & 0x00FF);
  } else {
    AgLog("Error getting meter status!");
  }

  return (Status)status;
}

/**
 * @brief Get sensor output status
 *
 * @return int16_t Output status
 */
int16_t S8::getOutputStatus(void) {
  if (this->isBegin() == false) {
    return -1;
  }

  int16_t status = 0;

  // Ask output status
  sendCommand(MODBUS_FUNC_READ_INPUT_REGISTERS, MODBUS_IR3, 0x0001);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uint8_t nb = uartReadBytes(7, S8_TIMEOUT);

  // Check response and get data
  if (validResponseLenght(MODBUS_FUNC_READ_INPUT_REGISTERS, nb, 7)) {
    status = ((buf_msg[3] << 8) & 0xFF00) | (buf_msg[4] & 0x00FF);
  } else {
    AgLog("Error getting output status!");
  }

  return status;
}

/**
 * @brief Send special command to sensor, example manual calibration
 *
 * @param command Command
 * @return true Success
 * @return false Failure
 */
bool S8::sendSpecialCommand(CalibrationSpecialComamnd command) {
  if (this->isBegin() == false) {
    return false;
  }

  uint8_t buf_msg_sent[8];
  bool result = false;

  // Ask set user special command
  sendCommand(MODBUS_FUNC_WRITE_SINGLE_REGISTER, MODBUS_HR2, command);

  // Save bytes sent
  memcpy(buf_msg_sent, buf_msg, 8);

  // Wait response
  memset(buf_msg, 0, S8_LEN_BUF_MSG);
  uartReadBytes(8, S8_TIMEOUT);

  // Check response
  if (memcmp(buf_msg_sent, buf_msg, 8) == 0) {
    result = true;
    AgLog("Successful setting user special command");
  } else {
    AgLog("Error in setting user special command!");
  }

  return result;
}

/**
 * @brief Init sensor with BSP
 *
 * @param bsp AirGradient BSP @ref BoardDef
 * @return true
 * @return false
 */
bool S8::init(const BoardDef *bsp) {
  return this->init(bsp->SenseAirS8.uart_tx_pin, bsp->SenseAirS8.uart_rx_pin);
}

/**
 * @brief Init sensor
 *
 * @param txPin UART TX pin
 * @param rxPin UART RX pin
 * @return true = success, otherwise failure
 */
bool S8::init(int txPin, int rxPin) { return this->init(txPin, rxPin, 9600); }

bool S8::_begin(void) {
  const BoardDef *bsp = getBoardDef(this->_boardDef);
  if (bsp == NULL) {
    AgLog("Board Type not supported");
    return false;
  }

  if (bsp->SenseAirS8.supported == false) {
    AgLog("Board is not support this sensor");
    return false;
  }

  // Init sensor
  if (this->init(bsp) == false) {
    return false;
  }
  return true;
}

/**
 * @brief Init sensor
 *
 * @param txPin UART TX pin
 * @param rxPin UART RX pin
 * @param baud UART speed
 * @return true = success, otherwise failure
 */
bool S8::init(int txPin, int rxPin, uint32_t baud) {
#if defined(ESP8266)
  SoftwareSerial *uart = new SoftwareSerial(txPin, rxPin);
  uart->begin(baud);
  this->_uartStream = uart;
#else
#if ARDUINO_USB_CDC_ON_BOOT
  /** The 'Serial0' can ont configure tx, rx pin, only use as default */
  if (_serial == &Serial0) {
    AgLog("Init on 'Serial0'");
    _serial->begin(baud, SERIAL_8N1);
  } else {
    AgLog("Init on 'Serialx'");
    this->_serial->begin(baud, SERIAL_8N1, rxPin, txPin);
  }
  this->_uartStream = this->_serial;
#else
  AgLog("Init on 'Serialx'");
  this->_serial->begin(baud, SERIAL_8N1, rxPin, txPin);
  this->_uartStream = this->_serial;
#endif
#endif

  /** Check communication by get firmware version */
  delay(100);
  char fwVers[11];
  this->_isBegin = true;
  this->getFirmwareVersion(fwVers);
  if (strlen(fwVers) == 0) {
    this->_isBegin = false;
    return false;
  }
  AgLog("Firmware version: %s", fwVers);

  AgLog("Sensor successfully initialized. Heating up for 10s");
  this->_isBegin = true;
  this->_lastInitTime = millis();
  return true;
}

/**
 * @brief Check that sensor is initialized
 *
 * @return true Initialized
 * @return false No-Initialized
 */
bool S8::isBegin(void) {
  if (this->_isBegin) {
    return true;
  }
  AgLog("Sensor not-initialized");
  return false;
}

/**
 * @brief UART write
 *
 * @param size Number of bytes
 */
void S8::uartWriteBytes(uint8_t size) {
  this->_uartStream->write(buf_msg, size);
  this->_uartStream->flush();
}

uint8_t S8::uartReadBytes(uint8_t max_bytes, uint32_t timeout_ms) {
  uint8_t nb = 0;
  uint32_t stime = millis();
  while (1) {
    if (this->_uartStream->available()) {
      buf_msg[nb] = this->_uartStream->read();
      nb++;
      if (nb >= max_bytes) {
        break;
      }
    }

    uint32_t ms = (uint32_t)(millis() - stime);
    if (ms >= timeout_ms) {
      break;
    }

#if defined(ESP32)
    // Relax 5ms to avoid watchdog reset
    vTaskDelay(pdMS_TO_TICKS(1));
#endif
  }
  return nb;
}

/**
 * @brief Check reponse
 *
 * @param func Modbus function code
 * @param nb Number of byte
 * @return true
 * @return false
 */
bool S8::validResponse(uint8_t func, uint8_t nb) {
  uint16_t crc16;
  bool result = false;

  if (nb >= 7) {
    crc16 = AgMb16Crc(buf_msg, nb - 2);
    if ((buf_msg[nb - 2] == (crc16 & 0x00FF)) &&
        (buf_msg[nb - 1] == ((crc16 >> 8) & 0x00FF))) {

      if (buf_msg[0] == MODBUS_ANY_ADDRESS &&
          (buf_msg[1] == MODBUS_FUNC_READ_HOLDING_REGISTERS ||
           buf_msg[1] == MODBUS_FUNC_READ_INPUT_REGISTERS) &&
          buf_msg[2] == nb - 5) {
        AgLog("Valid response");
        result = true;
      } else {
        AgLog("Err: Unexpected response!");
      }
    } else {
      AgLog("Err: Checksum/length is invalid!");
    }
  } else {
    AgLog("Err: Invalid length!");
  }

  return result;
}

/**
 * @brief Check response length
 *
 * @param func Modbus function code
 * @param nb Number of bytes
 * @param len Length
 * @return true Success
 * @return false Failure
 */
bool S8::validResponseLenght(uint8_t func, uint8_t nb, uint8_t len) {
  bool result = false;
  if (nb == len) {
    result = validResponse(func, nb);
  } else {
    AgLog("Err: Unexpected length!");
  }

  return result;
}

void S8::sendCommand(uint8_t func, uint16_t reg, uint16_t value) {
  uint16_t crc16;

  if (((func == MODBUS_FUNC_READ_HOLDING_REGISTERS ||
        func == MODBUS_FUNC_READ_INPUT_REGISTERS) &&
       value >= 1) ||
      (func == MODBUS_FUNC_WRITE_SINGLE_REGISTER)) {
    buf_msg[0] = MODBUS_ANY_ADDRESS;    // Address
    buf_msg[1] = func;                  // Function
    buf_msg[2] = (reg >> 8) & 0x00FF;   // High-input register
    buf_msg[3] = reg & 0x00FF;          // Low-input register
    buf_msg[4] = (value >> 8) & 0x00FF; // High-word to read or setup
    buf_msg[5] = value & 0x00FF;        // Low-word to read or setup
    crc16 = AgMb16Crc(buf_msg, 6);
    buf_msg[6] = crc16 & 0x00FF;
    buf_msg[7] = (crc16 >> 8) & 0x00FF;
    uartWriteBytes(8);
  }
}

/**
 * @brief Set Auto calib baseline period
 *
 * @param hours Number of hour will trigger auto calib: [0, 4800], 0: disable
 * @return true Success
 * @return false Failure
 */
bool S8::setAbcPeriod(int hours) {
  if (isBegin() == false) {
    return false;
  }

  int curCalib = getCalibPeriodABC();
  if (curCalib == hours) {
    return true;
  }

  return setCalibPeriodABC(hours);
}

/**
 * @brief Get current 'ABC' calib period
 *
 * @return int Hour
 */
int S8::getAbcPeriod(void) { return getCalibPeriodABC(); }
