/*
 * EMC230x Arduino Library for RP2040
 *
 * Supports EMC2301/2/3/5 RPM-Based PWM Fan Controllers
 *
 * Author: Arduino Library for EMC230x
 * License: MIT
 */

#ifndef EMC230X_H
#define EMC230X_H

#include <Arduino.h>
#include <Wire.h>

// Device variant enumeration
enum EMC230xDevice {
  EMC230X_UNKNOWN_DEVICE = 0,
  EMC2301_DEVICE = 1,
  EMC2302_DEVICE = 2,
  EMC2303_DEVICE = 3,
  EMC2305_DEVICE = 5
};

// Register addresses
#define EMC230X_REG_CONFIG 0x20
#define EMC230X_REG_FAN_STATUS 0x24
#define EMC230X_REG_FAN_STALL 0x25
#define EMC230X_REG_FAN_SPIN 0x26
#define EMC230X_REG_DRIVE_FAIL 0x27
#define EMC230X_REG_FAN_INT_EN 0x29
#define EMC230X_REG_PWM_POLARITY 0x2A
#define EMC230X_REG_PWM_OUTPUT 0x2B
#define EMC230X_REG_PWM_BASE_45 0x2C
#define EMC230X_REG_PWM_BASE_123 0x2D
#define EMC230X_REG_SOFTWARE_LOCK 0xEF
#define EMC230X_REG_PRODUCT_FEAT 0xFC
#define EMC230X_REG_PRODUCT_ID 0xFD
#define EMC230X_REG_MANUFACTURER 0xFE
#define EMC230X_REG_REVISION 0xFF

// Fan register base addresses
#define EMC230X_REG_FAN1_BASE 0x30
#define EMC230X_REG_FAN2_BASE 0x40
#define EMC230X_REG_FAN3_BASE 0x50
#define EMC230X_REG_FAN4_BASE 0x60
#define EMC230X_REG_FAN5_BASE 0x70

// Fan register offsets from base
#define EMC230X_FAN_SETTING 0x00
#define EMC230X_FAN_PWM_DIVIDE 0x01
#define EMC230X_FAN_CONFIG1 0x02
#define EMC230X_FAN_CONFIG2 0x03
#define EMC230X_FAN_GAIN 0x05
#define EMC230X_FAN_SPIN_UP 0x06
#define EMC230X_FAN_MAX_STEP 0x07
#define EMC230X_FAN_MIN_DRIVE 0x08
#define EMC230X_FAN_VALID_TACH 0x09
#define EMC230X_FAN_DRIVE_FAIL_LOW 0x0A
#define EMC230X_FAN_DRIVE_FAIL_HIGH 0x0B
#define EMC230X_FAN_TACH_TARGET_LOW 0x0C
#define EMC230X_FAN_TACH_TARGET_HIGH 0x0D
#define EMC230X_FAN_TACH_READ_HIGH 0x0E
#define EMC230X_FAN_TACH_READ_LOW 0x0F

// Configuration bits
#define EMC230X_CONFIG_MASK 0x80
#define EMC230X_CONFIG_DIS_TO 0x40
#define EMC230X_CONFIG_WD_EN 0x20
#define EMC230X_CONFIG_DRECK 0x02
#define EMC230X_CONFIG_USECK 0x01

// PWM frequency options
enum PWMFrequency {
  PWM_FREQ_26000_HZ = 0,
  PWM_FREQ_19531_HZ = 1,
  PWM_FREQ_4882_HZ = 2,
  PWM_FREQ_2441_HZ = 3
};

// Fan configuration structures
struct FanConfig {
  bool enableClosedLoop;   // Enable RPM-based control
  uint8_t minRPM;          // 0=500RPM, 1=1000RPM, 2=2000RPM, 3=4000RPM
  uint8_t edges;           // 0=3 edges, 1=5 edges, 2=7 edges, 3=9 edges
  uint8_t updateTime;      // 0=100ms to 7=1600ms
  bool enableRampRate;     // Enable ramp rate control
  bool enableGlitchFilter; // Enable tach glitch filter
  uint8_t errorWindow;     // 0=0RPM, 1=50RPM, 2=100RPM, 3=200RPM
};

struct SpinUpConfig {
  uint8_t spinTime;       // 0=250ms, 1=500ms, 2=1s, 3=2s
  uint8_t spinLevel;      // 0=30% to 7=65%
  bool noKick;            // Disable initial 100% kick
  uint8_t driveFailCount; // 0=disabled, 1=16, 2=32, 3=64 update periods
};

struct PIDGains {
  uint8_t kp;               // Proportional gain: 0=1x, 1=2x, 2=4x, 3=8x
  uint8_t ki;               // Integral gain: 0=1x, 1=2x, 2=4x, 3=8x
  uint8_t kd;               // Derivative gain: 0=1x, 1=2x, 2=4x, 3=8x
  uint8_t derivativeOption; // 0=none, 1=basic, 2=step, 3=both
};

class EMC230x {
public:
  // Constructors
  EMC230x(uint8_t address = 0x2F, TwoWire &wire = Wire);
  EMC230x(EMC230xDevice device, uint8_t address = 0x2F, TwoWire &wire = Wire);

  // Initialization
  bool begin();
  bool begin(uint8_t sda, uint8_t scl);
  bool begin(EMC230xDevice device);
  bool beginWithoutWireInit();
  bool isConnected();

  // Device identification
  uint8_t getProductID();
  uint8_t getManufacturerID();
  uint8_t getRevision();
  EMC230xDevice detectDevice();

  // Fan control - Direct PWM mode
  bool setFanSpeed(uint8_t fan, uint8_t speed);        // 0-255 PWM value
  bool setFanSpeedPercent(uint8_t fan, float percent); // 0-100%
  uint8_t getFanSpeed(uint8_t fan);
  float getFanSpeedPercent(uint8_t fan);

  // Fan control - RPM mode
  bool setTargetRPM(uint8_t fan, uint16_t rpm);
  uint16_t getTargetRPM(uint8_t fan);
  uint16_t getActualRPM(uint8_t fan);
  bool enableRPMControl(uint8_t fan, bool enable = true);
  bool isRPMControlEnabled(uint8_t fan);

  // Tachometer readings
  uint16_t getTachCount(uint8_t fan);
  uint16_t convertRPMtoTach(uint16_t rpm, uint8_t fan);
  uint16_t convertTachToRPM(uint16_t tach, uint8_t fan);

  // PWM configuration
  bool setPWMFrequency(uint8_t fan, PWMFrequency freq);
  bool setPWMDivider(uint8_t fan, uint8_t divider);
  bool setPWMPolarity(uint8_t fan, bool inverted);
  bool setPWMOutputType(uint8_t fan, bool pushPull);

  // Fan configuration
  bool setFanConfig(uint8_t fan, const FanConfig &config);
  bool getFanConfig(uint8_t fan, FanConfig &config);
  bool setSpinUpConfig(uint8_t fan, const SpinUpConfig &config);
  bool getSpinUpConfig(uint8_t fan, SpinUpConfig &config);

  // PID control
  bool setPIDGains(uint8_t fan, const PIDGains &gains);
  bool getPIDGains(uint8_t fan, PIDGains &gains);

  // Advanced settings
  bool setMinimumDrive(uint8_t fan, uint8_t minDrive);
  bool setMaximumStep(uint8_t fan, uint8_t maxStep);
  bool setValidTachCount(uint8_t fan, uint8_t count);

  // Status and fault detection
  bool isFanStalled(uint8_t fan);
  bool isFanSpinUpFailed(uint8_t fan);
  bool isDriveFailed(uint8_t fan);
  uint8_t getFanStatus();
  bool clearFaults();

  // Interrupt control
  bool enableInterrupt(uint8_t fan, bool enable = true);
  bool isInterruptEnabled(uint8_t fan);

  // Watchdog timer
  bool enableWatchdog(bool continuous = false);
  bool disableWatchdog();
  bool isWatchdogTriggered();

  // Clock configuration
  bool useExternalClock(bool enable = true);
  bool enableClockOutput(bool enable = true);

  // Software lock
  bool lockConfiguration();
  bool isConfigurationLocked();

  // Low-level register access
  uint8_t readRegister(uint8_t reg);
  bool writeRegister(uint8_t reg, uint8_t value);
  uint16_t readRegister16(uint8_t regHigh, uint8_t regLow);
  bool writeRegister16(uint8_t regHigh, uint8_t regLow, uint16_t value);

  // Utility functions
  bool isValidFan(uint8_t fan);
  uint8_t getMaxFans();

  uint8_t getFanPoles(uint8_t fan);

private:
  TwoWire *_wire;
  uint8_t _address;
  EMC230xDevice _device;
  uint8_t _maxFans;

  uint8_t getFanRegisterBase(uint8_t fan);
  bool beginAfterWireInit();
  bool detectAndSetDevice();
};

// Helper class for EMC2303/5 address selection
class EMC230xAddressDecoder {
public:
  static uint8_t getAddressFromResistor(float resistorValue);
  static bool isClockPinAvailable(float resistorValue);
  static uint8_t getDefaultFanSpeed(float clkResistorValue);
};

#endif // EMC230X_H
