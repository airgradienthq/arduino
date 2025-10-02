#ifndef _S8_H_
#define _S8_H_

#include "../Main/BoardDef.h"
#include "Arduino.h"

/**
 * @brief The class define how to handle the senseair S8 sensor (CO2 sensor)
 */
class S8 {
public:
  const int S8_BAUDRATE =
      9600; // Device to S8 Serial baudrate (should not be changed)
  const int S8_TIMEOUT = 5000ul; // Timeout for communication in milliseconds
  const int S8_LEN_BUF_MSG =
      20; // Max length of buffer for communication with the sensor
  const int S8_LEN_FIRMVER = 10; // Length of software version

  enum ModbusAddr {
    MODBUS_ANY_ADDRESS = 0XFE,                 // S8 uses any address
    MODBUS_FUNC_READ_HOLDING_REGISTERS = 0X03, // Read holding registers (HR)
    MODBUS_FUNC_READ_INPUT_REGISTERS = 0x04,   // Read input registers (IR)
    MODBUS_FUNC_WRITE_SINGLE_REGISTER = 0x06,  // Write single register (SR)
  };
  enum ModbusRegInput {
    MODBUS_IR1 = 0x0000,  // MeterStatus
    MODBUS_IR2 = 0x0001,  // AlarmStatus
    MODBUS_IR3 = 0x0002,  // OutputStatus
    MODBUS_IR4 = 0x0003,  // Space CO2
    MODBUS_IR22 = 0x0015, // PWM Output
    MODBUS_IR26 = 0x0019, // Sensor Type ID High
    MODBUS_IR27 = 0x001A, // Sensor Type ID Low
    MODBUS_IR28 = 0x001B, // Memory Map version
    MODBUS_IR29 = 0x001C, // FW version Main.Sub
    MODBUS_IR30 = 0x001D, // Sensor ID High
    MODBUS_IR31 = 0x001E, // Sensor ID Low
  };
  enum ModbusRegHolding {
    MODBUS_HR1 = 0x0000,  // Acknowledgement Register
    MODBUS_HR2 = 0x0001,  // Special Command Register
    MODBUS_HR32 = 0x001F, // ABC Period
  };
  enum Status {
    S8_MASK_METER_FATAL_ERROR = 0x0001,             // Fatal error
    S8_MASK_METER_OFFSET_REGULATION_ERROR = 0x0002, // Offset regulation error
    S8_MASK_METER_ALGORITHM_ERROR = 0x0004,         // Algorithm error
    S8_MASK_METER_OUTPUT_ERROR = 0x0008,            // Output error
    S8_MASK_METER_SELF_DIAG_ERROR = 0x0010,         // Self diagnostics error
    S8_MASK_METER_OUT_OF_RANGE = 0x0020,            // Out of range
    S8_MASK_METER_MEMORY_ERROR = 0x0040,            // Memory error
    S8_MASK_METER_ANY_ERROR = 0x007F, // Mask to detect the previous errors
                                      // (fatal error ... memory error)
  };
  enum OutputStatus {
    S8_MASK_OUTPUT_ALARM =
        0x0001, // Alarm output status (inverted due to Open Collector)
    S8_MASK_OUTPUT_PWM = 0x0002, // PWM output status (=1 -> full output)
  };
  enum AcknowledgementFLags {
    S8_MASK_CO2_BACKGROUND_CALIBRATION =
        0x0020, // CO2 Background calibration performed = 1
    S8_MASK_CO2_NITROGEN_CALIBRATION =
        0x0040, // CO2 Nitrogen calibration performed = 1
  };
  enum CalibrationSpecialComamnd {
    S8_CO2_BACKGROUND_CALIBRATION = 0x7C06, // CO2 Background calibration
    S8_CO2_ZERO_CALIBRATION = 0x7C07,       // CO2 Zero calibration
  };

  S8(BoardType def);
#if defined(ESP8266)
  bool begin(void);
  bool begin(Stream *_serialDebug);
#else
  bool begin(HardwareSerial &serial);
#endif
  void end(void);
  int16_t getCo2(void);
  bool setBaselineCalibration(void);
  bool isBaseLineCalibrationDone(void);
  bool setAbcPeriod(int hours);
  int getAbcPeriod(void);
  void printInformation(void);

private:
  /** Variables */
  const char *TAG = "S8";
  uint8_t buf_msg[20];
  Stream *_debugStream;
  BoardType _boardDef;
  Stream *_uartStream;
#if defined(ESP32)
  HardwareSerial *_serial;
#endif
  bool _isBegin = false;
  uint32_t _lastInitTime;
  bool isCalib = false;

  /** Functions */
  bool _begin(void);
  bool init(const BoardDef *bsp);
  bool init(int txPin, int rxPin);
  bool init(int txPin, int rxPin, uint32_t baud);
  bool isBegin(void);

  void uartWriteBytes(uint8_t size); // Send bytes to sensor
  uint8_t
  uartReadBytes(uint8_t max_bytes,
                uint32_t timeout_seconds); // Read received bytes from sensor
  bool validResponse(
      uint8_t func,
      uint8_t nb); // Check if response is valid according to sent command
  bool validResponseLenght(
      uint8_t func, uint8_t nb,
      uint8_t len); // Check if response is valid according to sent command and
                    // checking expected total length
  void sendCommand(uint8_t func, uint16_t reg, uint16_t value); // Send command

  void getFirmwareVersion(char firmwver[]);
  int32_t getSensorTypeId(void);
  int32_t getSensorId(void);
  int16_t getMemoryMapVersion(void);
  int16_t getOutputPWM(void);
  int16_t getCalibPeriodABC(void);
  bool setCalibPeriodABC(int16_t period);
  bool manualCalib(void);
  int16_t getAcknowledgement(void);
  bool clearAcknowledgement(void);
  Status getStatus(void);
  int16_t getAlarmStatus(void);
  int16_t getOutputStatus(void);
  bool sendSpecialCommand(CalibrationSpecialComamnd command);
};

#endif
