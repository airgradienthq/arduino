#include "SPS30.h"

// The Sensirion library defines NO_ERROR which can conflict with ESP-IDF.
// Re-define locally if needed.
#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

SPS30::SPS30(BoardType def) : _boardDef(def) {}

bool SPS30::begin(HardwareSerial &serial) {
  if (_isBegin) {
    AgLog("Already initialized, call end() then try again");
    return true;
  }

  _serial = &serial;

  // SPS30 UART uses 115200 baud (SHDLC protocol)
  _serial->begin(115200);
  _driver.begin(serial);

  // Ensure sensor is in idle state before detection
  _driver.stopMeasurement();

  // Try reading serial number as a detection mechanism
  int8_t serialNumber[32] = {0};
  int16_t error = _driver.readSerialNumber(serialNumber, sizeof(serialNumber));
  if (error != NO_ERROR) {
    AgLog("SPS30 not detected (readSerialNumber error: %d)", error);
    _serial = nullptr;
    return false;
  }
  AgLog("SPS30 detected, serial: %s", reinterpret_cast<char *>(serialNumber));

  // Start continuous measurement in float output format
  error = _driver.startMeasurement(SPS30_OUTPUT_FORMAT_OUTPUT_FORMAT_FLOAT);
  if (error != NO_ERROR) {
    AgLog("SPS30 startMeasurement failed (error: %d)", error);
    _serial = nullptr;
    return false;
  }

  _isBegin = true;
  _connected = true;
  _consecutiveErrors = 0;
  return true;
}

void SPS30::end() {
  if (!_isBegin) {
    return;
  }

  _driver.stopMeasurement();
  _isBegin = false;
  _connected = false;
  _serial = nullptr;
  AgLog("De-initialized");
}

bool SPS30::readValues() {
  if (!_isBegin) {
    return false;
  }

  float mc1p0, mc2p5, mc4p0, mc10p0;
  float nc0p5, nc1p0, nc2p5, nc4p0, nc10p0;
  float typicalParticleSize;

  int16_t error = _driver.readMeasurementValuesFloat(
      mc1p0, mc2p5, mc4p0, mc10p0, nc0p5, nc1p0, nc2p5, nc4p0, nc10p0,
      typicalParticleSize);

  if (error != NO_ERROR) {
    _consecutiveErrors++;
    if (_consecutiveErrors >= kMaxConsecutiveErrors) {
      _connected = false;
    }
    return false;
  }

  // Cache the successfully read values
  _mc1p0 = mc1p0;
  _mc2p5 = mc2p5;
  _mc4p0 = mc4p0;
  _mc10p0 = mc10p0;
  _nc0p5 = nc0p5;
  _nc1p0 = nc1p0;
  _nc2p5 = nc2p5;
  _nc4p0 = nc4p0;
  _nc10p0 = nc10p0;
  _typicalParticleSize = typicalParticleSize;

  _connected = true;
  _consecutiveErrors = 0;
  return true;
}

bool SPS30::connected() { return _connected; }

// -- Mass concentration (µg/m³) → int, matching PMS5003 interface -----------

int SPS30::getPm01Ae() { return static_cast<int>(_mc1p0); }
int SPS30::getPm25Ae() { return static_cast<int>(_mc2p5); }
int SPS30::getPm10Ae() { return static_cast<int>(_mc10p0); }

// SPS30 has no Ae/SP distinction — return the same values
int SPS30::getPm01Sp() { return static_cast<int>(_mc1p0); }
int SPS30::getPm25Sp() { return static_cast<int>(_mc2p5); }
int SPS30::getPm10Sp() { return static_cast<int>(_mc10p0); }

// -- Number concentration: convert #/cm³ → #/0.1L (×100) -------------------

int SPS30::getPm05ParticleCount() { return static_cast<int>(_nc0p5 * 100.0f); }
int SPS30::getPm01ParticleCount() { return static_cast<int>(_nc1p0 * 100.0f); }
int SPS30::getPm25ParticleCount() { return static_cast<int>(_nc2p5 * 100.0f); }
int SPS30::getPm10ParticleCount() { return static_cast<int>(_nc10p0 * 100.0f); }
