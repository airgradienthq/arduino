#ifndef _AIR_GRADIENT_SPS30_H_
#define _AIR_GRADIENT_SPS30_H_

#include "../Main/BoardDef.h"

#if defined(ESP8266)

/**
 * @brief Stub class for ESP8266 — SPS30 UART is not supported on this platform.
 * Provides the type so AirGradient.h compiles, but begin() always returns false.
 */
class SPS30 {
public:
  explicit SPS30(BoardType def) {}
  bool begin(Stream &serial) { return false; }
  void end() {}
  bool readValues() { return false; }
  bool connected() { return false; }
  int getPm01Ae() { return 0; }
  int getPm25Ae() { return 0; }
  int getPm10Ae() { return 0; }
  int getPm01Sp() { return 0; }
  int getPm25Sp() { return 0; }
  int getPm10Sp() { return 0; }
  int getPm05ParticleCount() { return 0; }
  int getPm01ParticleCount() { return 0; }
  int getPm25ParticleCount() { return 0; }
  int getPm10ParticleCount() { return 0; }
};

#else // ESP32

#include <SensirionUartSps30.h>

/**
 * @brief Wrapper class for Sensirion SPS30 particulate matter sensor over UART.
 *
 * Follows the same interface pattern as PMS5003 so it can be used as a
 * drop-in alternative PM sensor on the ONE_INDOOR board.
 *
 * Key differences from PMS5003:
 *   - SHDLC protocol at 115200 baud (vs PMS5003 streaming at 9600)
 *   - No continuous handle() needed; values are polled via readValues()
 *   - No PM0.3 or PM5.0 particle count bins
 *   - No distinction between atmospheric-environment and standard-particle
 *     mass concentrations (same values mapped to both)
 */
class SPS30 {
public:
  explicit SPS30(BoardType def);

  /**
   * @brief Initialize sensor on the given serial port.
   *
   * Configures the serial port to 115200 baud, performs sensor detection
   * (reads serial number), and starts continuous measurement in float mode.
   *
   * @param serial HardwareSerial instance (e.g. Serial0)
   * @return true  Sensor detected and measurement started
   * @return false Sensor not found or initialization failed
   */
  bool begin(HardwareSerial &serial);

  /**
   * @brief Stop measurement and release the serial port.
   */
  void end();

  /**
   * @brief Poll the sensor for the latest measurement.
   *
   * Should be called periodically (e.g. every 2 s via pmsSchedule).
   * Internally calls readMeasurementValuesFloat() over SHDLC.
   *
   * @return true  New data read successfully
   * @return false Read failed (sensor may be disconnected)
   */
  bool readValues();

  /**
   * @brief Check whether the sensor is connected / responding.
   *
   * Based on the result of the most recent readValues() call.
   */
  bool connected();

  // -- Mass concentration (µg/m³), truncated to int like PMS5003 ----------

  int getPm01Ae();
  int getPm25Ae();
  int getPm10Ae();

  // SPS30 has no Ae/SP distinction — return the same values
  int getPm01Sp();
  int getPm25Sp();
  int getPm10Sp();

  // -- Number concentration (converted from #/cm³ to #/0.1 L) -------------

  int getPm05ParticleCount();
  int getPm01ParticleCount();
  int getPm25ParticleCount();
  int getPm10ParticleCount();

private:
  bool _isBegin = false;
  bool _connected = false;
  int _consecutiveErrors = 0;
  BoardType _boardDef;
  SensirionUartSps30 _driver;
  HardwareSerial *_serial = nullptr;

  /// Maximum consecutive read errors before reporting disconnected
  static constexpr int kMaxConsecutiveErrors = 3;

  // Cached raw values from last successful read
  float _mc1p0 = 0.0f;
  float _mc2p5 = 0.0f;
  float _mc4p0 = 0.0f;
  float _mc10p0 = 0.0f;
  float _nc0p5 = 0.0f;
  float _nc1p0 = 0.0f;
  float _nc2p5 = 0.0f;
  float _nc4p0 = 0.0f;
  float _nc10p0 = 0.0f;
  float _typicalParticleSize = 0.0f;
};

#endif // ESP8266 / ESP32

#endif // _AIR_GRADIENT_SPS30_H_
