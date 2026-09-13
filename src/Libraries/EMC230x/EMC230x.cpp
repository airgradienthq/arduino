/*
 * EMC230x Arduino Library Implementation
 *
 * Supports EMC2301/2/3/5 RPM-Based PWM Fan Controllers
 */

#include "EMC230x.h"

// Constructor
EMC230x::EMC230x(uint8_t address, TwoWire &wire)
    : _wire(&wire), _address(address), _device(EMC2301_DEVICE), _maxFans(1) {}

EMC230x::EMC230x(EMC230xDevice device, uint8_t address, TwoWire &wire)
    : _wire(&wire), _address(address), _device(device), _maxFans(device) {}

// Initialization
bool EMC230x::begin() {
  _wire->begin();
  return beginAfterWireInit();
}

bool EMC230x::begin(uint8_t sda, uint8_t scl) {
  _wire->begin(sda, scl);
  return beginAfterWireInit();
}

bool EMC230x::begin(EMC230xDevice device) {
  _device = device;
  _maxFans = device;
  return begin();
}

bool EMC230x::beginWithoutWireInit() { return beginAfterWireInit(); }

bool EMC230x::beginAfterWireInit() {
  if (!isConnected()) {
    return false;
  }

  // Auto-detect device type
  if (!detectAndSetDevice()) {
    return false;
  }

  // Clear any pending faults
  clearFaults();
  return true;
}

bool EMC230x::isConnected() {
  // Try to read manufacturer ID (should always be 0x5D)
  uint8_t mfgId = getManufacturerID();
  return (mfgId == 0x5D);
}

// Device identification
uint8_t EMC230x::getProductID() { return readRegister(EMC230X_REG_PRODUCT_ID); }

uint8_t EMC230x::getManufacturerID() { return readRegister(EMC230X_REG_MANUFACTURER); }

uint8_t EMC230x::getRevision() { return readRegister(EMC230X_REG_REVISION); }

EMC230xDevice EMC230x::detectDevice() {
  uint8_t pid = getProductID();
  switch (pid) {
  case 0x37:
    return EMC2301_DEVICE;
  case 0x36:
    return EMC2302_DEVICE;
  case 0x35:
    return EMC2303_DEVICE;
  case 0x34:
    return EMC2305_DEVICE;
  default:
    return EMC230X_UNKNOWN_DEVICE;
  }
}

// Fan control - Direct PWM mode
bool EMC230x::setFanSpeed(uint8_t fan, uint8_t speed) {
  if (!isValidFan(fan))
    return false;

  // Ensure we're in direct mode (not RPM control)
  uint8_t configReg = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG1;
  uint8_t config = readRegister(configReg);

  if (config & 0x80) { // ENAG bit is set
    // Need to disable RPM control first
    config &= ~0x80;
    if (!writeRegister(configReg, config)) {
      return false;
    }
  }

  uint8_t reg = getFanRegisterBase(fan) + EMC230X_FAN_SETTING;
  return writeRegister(reg, speed);
}

bool EMC230x::setFanSpeedPercent(uint8_t fan, float percent) {
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;

  uint8_t speed = (uint8_t)(percent * 255.0 / 100.0);
  return setFanSpeed(fan, speed);
}

uint8_t EMC230x::getFanSpeed(uint8_t fan) {
  if (!isValidFan(fan))
    return 0;

  uint8_t reg = getFanRegisterBase(fan) + EMC230X_FAN_SETTING;
  return readRegister(reg);
}

float EMC230x::getFanSpeedPercent(uint8_t fan) {
  uint8_t speed = getFanSpeed(fan);
  return (float)speed * 100.0 / 255.0;
}

// Updated getActualRPM to use the improved conversion
uint16_t EMC230x::getActualRPM(uint8_t fan) {
  if (!isValidFan(fan))
    return 0;

  uint8_t regLow = getFanRegisterBase(fan) + EMC230X_FAN_TACH_READ_LOW;
  uint8_t regHigh = getFanRegisterBase(fan) + EMC230X_FAN_TACH_READ_HIGH;

  uint16_t tach = readRegister16(regHigh, regLow);

  // Use the fan-aware conversion function
  return convertTachToRPM(tach, fan);
}

// Updated getTargetRPM to use the improved conversion
uint16_t EMC230x::getTargetRPM(uint8_t fan) {
  if (!isValidFan(fan))
    return 0;

  uint8_t regLow = getFanRegisterBase(fan) + EMC230X_FAN_TACH_TARGET_LOW;
  uint8_t regHigh = getFanRegisterBase(fan) + EMC230X_FAN_TACH_TARGET_HIGH;

  uint16_t tach = readRegister16(regHigh, regLow);

  // Use the fan-aware conversion function
  return convertTachToRPM(tach, fan);
}

// Updated setTargetRPM to use the improved conversion
bool EMC230x::setTargetRPM(uint8_t fan, uint16_t rpm) {
  if (!isValidFan(fan))
    return false;

  // Convert RPM to TACH count using fan-aware function
  uint16_t tach = convertRPMtoTach(rpm, fan);

  // Write to target registers
  uint8_t regLow = getFanRegisterBase(fan) + EMC230X_FAN_TACH_TARGET_LOW;
  uint8_t regHigh = getFanRegisterBase(fan) + EMC230X_FAN_TACH_TARGET_HIGH;

  return writeRegister16(regHigh, regLow, tach);
}

// Helper function to get current fan poles setting
uint8_t EMC230x::getFanPoles(uint8_t fan) {
  if (!isValidFan(fan))
    return 2; // Default to 2-pole

  uint8_t configReg = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG1;
  uint8_t config = readRegister(configReg);
  uint8_t edges = ((config >> 3) & 0x03);

  // Map edges setting to pole count
  // This assumes the edges are configured to match the fan poles
  switch (edges) {
  case 0:
    return 1; // 3 edges = 1 pole
  case 1:
    return 2; // 5 edges = 2 poles (most common)
  case 2:
    return 3; // 7 edges = 3 poles
  case 3:
    return 4; // 9 edges = 4 poles
  default:
    return 2;
  }
}

bool EMC230x::enableRPMControl(uint8_t fan, bool enable) {
  if (!isValidFan(fan))
    return false;

  uint8_t reg = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG1;
  uint8_t config = readRegister(reg);

  if (enable) {
    config |= 0x80; // Set ENAG bit
  } else {
    config &= ~0x80; // Clear ENAG bit
  }

  return writeRegister(reg, config);
}

bool EMC230x::isRPMControlEnabled(uint8_t fan) {
  if (!isValidFan(fan))
    return false;

  uint8_t reg = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG1;
  uint8_t config = readRegister(reg);

  return (config & 0x80) != 0;
}

// Tachometer functions
uint16_t EMC230x::getTachCount(uint8_t fan) {
  if (!isValidFan(fan))
    return 0;

  uint8_t regLow = getFanRegisterBase(fan) + EMC230X_FAN_TACH_READ_LOW;
  uint8_t regHigh = getFanRegisterBase(fan) + EMC230X_FAN_TACH_READ_HIGH;

  return readRegister16(regHigh, regLow);
}

uint16_t EMC230x::convertRPMtoTach(uint16_t rpm, uint8_t fan) {
  if (rpm == 0)
    return 0xFFFF; // Special value for stopped fan
  if (!isValidFan(fan))
    return 0;

  // Get fan configuration to determine edges and range
  uint8_t configReg = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG1;
  uint8_t config = readRegister(configReg);

  uint8_t edges = ((config >> 3) & 0x03);
  uint8_t range = ((config >> 5) & 0x03);

  // Calculate actual edges measured based on EDGES bits
  uint8_t edgeCount = (edges * 2) + 3;

  // Calculate multiplier from RANGE bits
  uint8_t multiplier = 1 << range;

  // TACH = (f_tach * 60 * multiplier * pulses) / (RPM * (n-1))
  // The Taiju fan provides 4 tach pulses per revolution.
  uint32_t numerator = 32768UL * 60 * multiplier * 4;
  uint32_t denominator = (uint32_t)rpm * (edgeCount - 1);

  // Prevent division by zero
  if (denominator == 0)
    return 0xFFFF;

  uint32_t tach = numerator / denominator;

  // Clamp to max 13-bit value
  if (tach > 0x1FFF)
    tach = 0x1FFF;

  return (uint16_t)tach;
}

uint16_t EMC230x::convertTachToRPM(uint16_t tach, uint8_t fan) {
  if (tach == 0 || tach >= 0x1FFF)
    return 0;

  if (!isValidFan(fan))
    return 0;

  uint8_t configReg = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG1;

  uint8_t config = readRegister(configReg);

  uint8_t edges = (config >> 3) & 0x03;
  uint8_t range = (config >> 5) & 0x03;

  uint8_t edgeCount = (edges * 2) + 3;
  uint8_t multiplier = 1 << range;

  const uint8_t pulsesPerRevolution = 4;

  uint32_t numerator = 32768UL * 60UL * multiplier * (edgeCount - 1);

  uint32_t denominator = (uint32_t)tach * pulsesPerRevolution;

  if (denominator == 0)
    return 0;

  uint32_t rpm = numerator / denominator;

  if (rpm > 16000)
    rpm = 16000;

  return (uint16_t)rpm;
}

// PWM configuration
bool EMC230x::setPWMFrequency(uint8_t fan, PWMFrequency freq) {
  if (!isValidFan(fan))
    return false;

  uint8_t reg;
  uint8_t mask;
  uint8_t shift;

  // Determine which register and bit positions to use
  if (fan <= 3) {
    reg = EMC230X_REG_PWM_BASE_123;
    shift = (fan - 1) * 2;
  } else {
    reg = EMC230X_REG_PWM_BASE_45;
    shift = (fan - 4) * 2;
  }

  mask = 0x03 << shift;

  uint8_t value = readRegister(reg);
  value = (value & ~mask) | ((freq & 0x03) << shift);

  return writeRegister(reg, value);
}

bool EMC230x::setPWMDivider(uint8_t fan, uint8_t divider) {
  if (!isValidFan(fan))
    return false;
  if (divider == 0)
    divider = 1; // 0 is interpreted as 1

  uint8_t reg = getFanRegisterBase(fan) + EMC230X_FAN_PWM_DIVIDE;
  return writeRegister(reg, divider);
}

bool EMC230x::setPWMPolarity(uint8_t fan, bool inverted) {
  if (!isValidFan(fan))
    return false;

  uint8_t reg = EMC230X_REG_PWM_POLARITY;
  uint8_t value = readRegister(reg);
  uint8_t mask = 1 << (fan - 1);

  if (inverted) {
    value |= mask;
  } else {
    value &= ~mask;
  }

  return writeRegister(reg, value);
}

bool EMC230x::setPWMOutputType(uint8_t fan, bool pushPull) {
  if (!isValidFan(fan))
    return false;

  uint8_t reg = EMC230X_REG_PWM_OUTPUT;
  uint8_t value = readRegister(reg);
  uint8_t mask = 1 << (fan - 1);

  if (pushPull) {
    value |= mask;
  } else {
    value &= ~mask;
  }

  return writeRegister(reg, value);
}

// Fan configuration
bool EMC230x::setFanConfig(uint8_t fan, const FanConfig &config) {
  if (!isValidFan(fan))
    return false;

  // Configure Fan Config 1 register
  uint8_t config1 = 0;
  if (config.enableClosedLoop)
    config1 |= 0x80;
  config1 |= (config.minRPM & 0x03) << 5;
  config1 |= (config.edges & 0x03) << 3;
  config1 |= (config.updateTime & 0x07);

  uint8_t reg1 = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG1;
  if (!writeRegister(reg1, config1))
    return false;

  // Configure Fan Config 2 register
  uint8_t config2 = 0;
  if (config.enableRampRate)
    config2 |= 0x40;
  if (config.enableGlitchFilter)
    config2 |= 0x20;
  config2 |= (config.errorWindow & 0x03) << 1;

  uint8_t reg2 = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG2;
  return writeRegister(reg2, config2);
}

bool EMC230x::getFanConfig(uint8_t fan, FanConfig &config) {
  if (!isValidFan(fan))
    return false;

  uint8_t reg1 = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG1;
  uint8_t config1 = readRegister(reg1);

  uint8_t reg2 = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG2;
  uint8_t config2 = readRegister(reg2);

  config.enableClosedLoop = (config1 & 0x80) != 0;
  config.minRPM = (config1 >> 5) & 0x03;
  config.edges = (config1 >> 3) & 0x03;
  config.updateTime = config1 & 0x07;

  config.enableRampRate = (config2 & 0x40) != 0;
  config.enableGlitchFilter = (config2 & 0x20) != 0;
  config.errorWindow = (config2 >> 1) & 0x03;

  return true;
}

bool EMC230x::setSpinUpConfig(uint8_t fan, const SpinUpConfig &config) {
  if (!isValidFan(fan))
    return false;

  uint8_t value = 0;
  value |= (config.driveFailCount & 0x03) << 6;
  if (config.noKick)
    value |= 0x20;
  value |= (config.spinLevel & 0x07) << 2;
  value |= (config.spinTime & 0x03);

  uint8_t reg = getFanRegisterBase(fan) + EMC230X_FAN_SPIN_UP;
  return writeRegister(reg, value);
}

bool EMC230x::getSpinUpConfig(uint8_t fan, SpinUpConfig &config) {
  if (!isValidFan(fan))
    return false;

  uint8_t reg = getFanRegisterBase(fan) + EMC230X_FAN_SPIN_UP;
  uint8_t value = readRegister(reg);

  config.driveFailCount = (value >> 6) & 0x03;
  config.noKick = (value & 0x20) != 0;
  config.spinLevel = (value >> 2) & 0x07;
  config.spinTime = value & 0x03;

  return true;
}

// PID control
bool EMC230x::setPIDGains(uint8_t fan, const PIDGains &gains) {
  if (!isValidFan(fan))
    return false;

  uint8_t value = 0;
  value |= (gains.kd & 0x03) << 4;
  value |= (gains.ki & 0x03) << 2;
  value |= (gains.kp & 0x03);

  uint8_t gainReg = getFanRegisterBase(fan) + EMC230X_FAN_GAIN;
  if (!writeRegister(gainReg, value))
    return false;

  // Set derivative option in Config2 register
  uint8_t config2Reg = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG2;
  uint8_t config2 = readRegister(config2Reg);
  config2 = (config2 & ~0x18) | ((gains.derivativeOption & 0x03) << 3);

  return writeRegister(config2Reg, config2);
}

bool EMC230x::getPIDGains(uint8_t fan, PIDGains &gains) {
  if (!isValidFan(fan))
    return false;

  uint8_t gainReg = getFanRegisterBase(fan) + EMC230X_FAN_GAIN;
  uint8_t value = readRegister(gainReg);

  gains.kd = (value >> 4) & 0x03;
  gains.ki = (value >> 2) & 0x03;
  gains.kp = value & 0x03;

  uint8_t config2Reg = getFanRegisterBase(fan) + EMC230X_FAN_CONFIG2;
  uint8_t config2 = readRegister(config2Reg);
  gains.derivativeOption = (config2 >> 3) & 0x03;

  return true;
}

// Advanced settings
bool EMC230x::setMinimumDrive(uint8_t fan, uint8_t minDrive) {
  if (!isValidFan(fan))
    return false;

  uint8_t reg = getFanRegisterBase(fan) + EMC230X_FAN_MIN_DRIVE;
  return writeRegister(reg, minDrive);
}

bool EMC230x::setMaximumStep(uint8_t fan, uint8_t maxStep) {
  if (!isValidFan(fan))
    return false;

  if (maxStep > 0x3F)
    maxStep = 0x3F; // 6-bit value

  uint8_t reg = getFanRegisterBase(fan) + EMC230X_FAN_MAX_STEP;
  return writeRegister(reg, maxStep);
}

bool EMC230x::setValidTachCount(uint8_t fan, uint8_t count) {
  if (!isValidFan(fan))
    return false;

  uint8_t reg = getFanRegisterBase(fan) + EMC230X_FAN_VALID_TACH;
  return writeRegister(reg, count);
}

// Status and fault detection
bool EMC230x::isFanStalled(uint8_t fan) {
  if (!isValidFan(fan))
    return false;

  uint8_t status = readRegister(EMC230X_REG_FAN_STALL);
  return (status & (1 << (fan - 1))) != 0;
}

bool EMC230x::isFanSpinUpFailed(uint8_t fan) {
  if (!isValidFan(fan))
    return false;

  uint8_t status = readRegister(EMC230X_REG_FAN_SPIN);
  return (status & (1 << (fan - 1))) != 0;
}

bool EMC230x::isDriveFailed(uint8_t fan) {
  if (!isValidFan(fan))
    return false;

  uint8_t status = readRegister(EMC230X_REG_DRIVE_FAIL);
  return (status & (1 << (fan - 1))) != 0;
}

uint8_t EMC230x::getFanStatus() { return readRegister(EMC230X_REG_FAN_STATUS); }

bool EMC230x::clearFaults() {
  // Read status registers to clear them
  readRegister(EMC230X_REG_FAN_STATUS);
  readRegister(EMC230X_REG_FAN_STALL);
  readRegister(EMC230X_REG_FAN_SPIN);
  readRegister(EMC230X_REG_DRIVE_FAIL);
  return true;
}

// Interrupt control
bool EMC230x::enableInterrupt(uint8_t fan, bool enable) {
  if (!isValidFan(fan))
    return false;

  uint8_t reg = EMC230X_REG_FAN_INT_EN;
  uint8_t value = readRegister(reg);
  uint8_t mask = 1 << (fan - 1);

  if (enable) {
    value |= mask;
  } else {
    value &= ~mask;
  }

  return writeRegister(reg, value);
}

bool EMC230x::isInterruptEnabled(uint8_t fan) {
  if (!isValidFan(fan))
    return false;

  uint8_t value = readRegister(EMC230X_REG_FAN_INT_EN);
  return (value & (1 << (fan - 1))) != 0;
}

// Watchdog timer
bool EMC230x::enableWatchdog(bool continuous) {
  uint8_t config = readRegister(EMC230X_REG_CONFIG);

  if (continuous) {
    config |= EMC230X_CONFIG_WD_EN;
  } else {
    config &= ~EMC230X_CONFIG_WD_EN;
  }

  return writeRegister(EMC230X_REG_CONFIG, config);
}

bool EMC230x::disableWatchdog() {
  // Write to any fan setting register to disable watchdog
  return writeRegister(EMC230X_REG_FAN1_BASE + EMC230X_FAN_SETTING,
                       readRegister(EMC230X_REG_FAN1_BASE + EMC230X_FAN_SETTING));
}

bool EMC230x::isWatchdogTriggered() {
  uint8_t status = readRegister(EMC230X_REG_FAN_STATUS);
  return (status & 0x80) != 0;
}

// Clock configuration
bool EMC230x::useExternalClock(bool enable) {
  uint8_t config = readRegister(EMC230X_REG_CONFIG);

  if (enable) {
    config |= EMC230X_CONFIG_USECK;
    config &= ~EMC230X_CONFIG_DRECK; // Cannot drive clock if using external
  } else {
    config &= ~EMC230X_CONFIG_USECK;
  }

  return writeRegister(EMC230X_REG_CONFIG, config);
}

bool EMC230x::enableClockOutput(bool enable) {
  uint8_t config = readRegister(EMC230X_REG_CONFIG);

  if (enable) {
    config |= EMC230X_CONFIG_DRECK;
    config &= ~EMC230X_CONFIG_USECK; // Cannot use external if driving clock
  } else {
    config &= ~EMC230X_CONFIG_DRECK;
  }

  return writeRegister(EMC230X_REG_CONFIG, config);
}

// Software lock
bool EMC230x::lockConfiguration() { return writeRegister(EMC230X_REG_SOFTWARE_LOCK, 0x01); }

bool EMC230x::isConfigurationLocked() {
  uint8_t lock = readRegister(EMC230X_REG_SOFTWARE_LOCK);
  return (lock & 0x01) != 0;
}

// Low-level register access
uint8_t EMC230x::readRegister(uint8_t reg) {
  _wire->beginTransmission(_address);
  _wire->write(reg);
  if (_wire->endTransmission(false) != 0) {
    return 0;
  }

  if (_wire->requestFrom(_address, (uint8_t)1) != 1) {
    return 0;
  }

  return _wire->read();
}

bool EMC230x::writeRegister(uint8_t reg, uint8_t value) {
  _wire->beginTransmission(_address);
  _wire->write(reg);
  _wire->write(value);
  return (_wire->endTransmission() == 0);
}

uint16_t EMC230x::readRegister16(uint8_t regHigh, uint8_t regLow) {
  uint8_t high = readRegister(regHigh);
  uint8_t low = readRegister(regLow);

  // Combine with only the valid bits from low register (bits 7:3)
  return ((uint16_t)high << 5) | ((low >> 3) & 0x1F);
}

bool EMC230x::writeRegister16(uint8_t regHigh, uint8_t regLow, uint16_t value) {
  uint8_t high = (value >> 5) & 0xFF;
  uint8_t low = (value & 0x1F) << 3;

  // Write low byte first (it's not applied until high byte is written)
  if (!writeRegister(regLow, low))
    return false;
  return writeRegister(regHigh, high);
}

// Utility functions
bool EMC230x::isValidFan(uint8_t fan) { return (fan >= 1 && fan <= _maxFans); }

uint8_t EMC230x::getMaxFans() { return _maxFans; }

uint8_t EMC230x::getFanRegisterBase(uint8_t fan) {
  switch (fan) {
  case 1:
    return EMC230X_REG_FAN1_BASE;
  case 2:
    return EMC230X_REG_FAN2_BASE;
  case 3:
    return EMC230X_REG_FAN3_BASE;
  case 4:
    return EMC230X_REG_FAN4_BASE;
  case 5:
    return EMC230X_REG_FAN5_BASE;
  default:
    return EMC230X_REG_FAN1_BASE;
  }
}

bool EMC230x::detectAndSetDevice() {
  EMC230xDevice detectedDevice = detectDevice();
  if (detectedDevice == EMC230X_UNKNOWN_DEVICE) {
    return false;
  }

  _device = detectedDevice;
  _maxFans = detectedDevice;
  return true;
}

// EMC230xAddressDecoder implementation
uint8_t EMC230xAddressDecoder::getAddressFromResistor(float resistorValue) {
  // Based on datasheet Table 5-1
  if (resistorValue >= 4230 && resistorValue <= 4935)
    return 0x2E; // 4.7k ?5%
  if (resistorValue >= 6120 && resistorValue <= 7140)
    return 0x2F; // 6.8k ?5%
  if (resistorValue >= 9000 && resistorValue <= 10500)
    return 0x2C; // 10k ?5%
  if (resistorValue >= 13500 && resistorValue <= 15750)
    return 0x2D; // 15k ?5%
  if (resistorValue >= 19800 && resistorValue <= 23100)
    return 0x4C; // 22k ?5%
  if (resistorValue >= 29700 && resistorValue <= 34650)
    return 0x4D; // 33k ?5%

  return 0x2F; // Default
}

bool EMC230xAddressDecoder::isClockPinAvailable(float resistorValue) {
  // Clock pin is only available with resistor values up to 22k
  return (resistorValue < 29700);
}

uint8_t EMC230xAddressDecoder::getDefaultFanSpeed(float clkResistorValue) {
  // Based on datasheet Table 4-9
  if (clkResistorValue >= 4230 && clkResistorValue <= 4935)
    return 0; // 0%
  if (clkResistorValue >= 6120 && clkResistorValue <= 7140)
    return 77; // 30%
  if (clkResistorValue >= 9000 && clkResistorValue <= 10500)
    return 128; // 50%
  if (clkResistorValue >= 13500 && clkResistorValue <= 15750)
    return 191; // 75%
  if (clkResistorValue >= 19800 && clkResistorValue <= 23100)
    return 255; // 100%
  if (clkResistorValue >= 29700 && clkResistorValue <= 34650)
    return 0; // 0%

  return 0; // Default
}
