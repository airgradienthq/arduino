#include <AirGradient.h>
#include <Wire.h>

/**
 * @brief Define test board
 */
#define TEST_DIY_BASIC 1

/**
 * @brief Define test sensor
 */
#define TEST_SENSOR_SenseAirS8 1
// #define S8_BASELINE_CALIB
#define TEST_SENSOR_PMS5003 0
#define TEST_SENSOR_SHT4x 0
#define TEST_SENSOR_SGP4x 0
#define TEST_SWITCH 0
#define TEST_OLED 0

#if TEST_DIY_BASIC
AirGradient ag(DIY_BASIC);
#elif TEST_DIY_PRO_INDOOR_V4_2
AirGradient ag(DIY_PRO_INDOOR_V4_2);
#else
#error "Board test not defined"
#endif

void setup() {
  Serial.begin(115200);

  /** Print All AirGradient board define */
  printBoardDef(&Serial);

#if TEST_SENSOR_SenseAirS8
  if (ag.s8.begin(&Serial) == true) {
    Serial.println("CO2S8 sensor init success");
  } else {
    Serial.println("CO2S8 sensor init failure");
  }

#ifdef S8_BASELINE_CALIB
  if (ag.s8.setBaselineCalibration()) {
    Serial.println("Manual calib success");
  } else {
    Serial.println("Manual calib failure");
  }
#else
  if (ag.s8.setAutoCalib(8)) {
    Serial.println("Set auto calib success");
  } else {
    Serial.println("Set auto calib failure");
  }
#endif
  delay(5000);
#endif

#if TEST_SENSOR_PMS5003
  if (ag.pms5003.begin(&Serial) == true) {
    Serial.println("PMS5003 sensor init success");
  } else {
    Serial.println("PMS5003 sensor init failure");
  }
#endif

#if TEST_SENSOR_SHT4x || TEST_SENSOR_SGP4x || TEST_OLED
  Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin());
#endif

#if TEST_SENSOR_SHT4x
  if (ag.sht4x.begin(Wire, Serial)) {
    Serial.println("SHT init success");
  } else {
    Serial.println("SHT init failed");
  }
#endif

#if TEST_SENSOR_SGP4x
  if (ag.sgp41.begin(Wire, Serial)) {
    Serial.println("SGP init succses");
  } else {
    Serial.println("SGP init failure");
  }
#endif

#if TEST_SWITCH
  ag.button.begin(Serial);
#endif

#if TEST_OLED
  ag.display.begin(Wire, Serial);
  ag.display.setTextSize(1);
  ag.display.setCursor(0, 0);
  ag.display.setTextColor(1);
  ag.display.setText("Hello");
  ag.display.show();
#endif
}

void loop() {
  uint32_t ms;
#if TEST_SENSOR_SenseAirS8
  static uint32_t lastTime = 0;

  /** Wait for sensor ready */
  // if(co2s8.isReady())
  // {
  // Get sensor data each 1sec
  ms = (uint32_t)(millis() - lastTime);
  if (ms >= 1000) {
    lastTime = millis();
    Serial.printf("CO2: %d (PMM)\r\n", ag.s8.getCo2());
  }
  // }
#endif

#if TEST_SENSOR_PMS5003
  static uint32_t pmsTime = 0;
  ms = (uint32_t)(millis() - pmsTime);
  if (ms >= 1000) {
    pmsTime = millis();
    if (ag.pms5003.readData()) {
      Serial.printf("Passive mode PM 1.0  (ug/m3): %d\r\n",
                    ag.pms5003.getPm01Ae());
      Serial.printf("Passive mode PM 2.5  (ug/m3): %d\r\n",
                    ag.pms5003.getPm25Ae());
      Serial.printf("Passive mode PM 10.5 (ug/m3): %d\r\n",
                    ag.pms5003.getPm10Ae());
    }
  }
#endif

#if TEST_SENSOR_SHT4x
  /**
   * @brief Get SHT sensor data each 1sec
   *
   */
  static uint32_t shtTime = 0;
  ms = (uint32_t)(millis() - shtTime);
  if (ms >= 1000) {
    shtTime = millis();
    float temperature, humidity;
    Serial.printf("SHT Temperature: %f, Humidity: %f\r\n",
                  ag.sht4x.getTemperature(), ag.sht4x.getRelativeHumidity());
  }
#endif

#if TEST_SENSOR_SGP4x
  static uint32_t sgpTime;
  ms = (uint32_t)(millis() - sgpTime);

  /***
   * Must call this task on loop and avoid delay on loop over 1000 ms
   */
  ag.sgp41.handle();

  if (ms >= 1000) {
    sgpTime = millis();
    Serial.printf("SGP TVOC: %d, NOx: %d\r\n", ag.sgp41.getTvocIndex(),
                  ag.sgp41.getNoxIndex());
  }
#endif

#if TEST_SWITCH
  static PushButton::State stateOld = PushButton::State::BUTTON_RELEASED;
  PushButton::State state = ag.button.getState();
  if (state != stateOld) {
    stateOld = state;
    Serial.printf("Button state changed: %s\r\n",
                  ag.button.toString(state).c_str());
  }
#endif
}
