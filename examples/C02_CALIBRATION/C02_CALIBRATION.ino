/*
 This is the code for forced calibration of the SenseAir S8 sensor. The sensor also has a one-week automatic baseline calibration that should calibrate the sensor latest after one week.0
 However if you need a faster calibration please proceed as following:

 1. Flash this code
 2. Bring the sensor outside into fresh air and leave it there for at least 10 minutes
 3. Power on the sensor
 4. Follow the instructions on the display
 5. After the calibration has been done, flash back the previous code for AQ measurements

 The codes needs the following libraries installed:
  “S8_UART” by Josep Comas tested with version 1.0.1
  “U8g2” by oliver tested with version 2.32.15

Many thanks to Josep Comas of the S8_UART library from which parts of below code are borrowed.

 */

#include <Arduino.h>
#include "s8_uart.h"
#include <U8g2lib.h>

/* BEGIN CONFIGURATION */

// Display
//U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);//for DIY PRO
U8G2_SSD1306_64X48_ER_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE); //for DIY BASIC

#define DEBUG_BAUDRATE 115200

#if (defined USE_SOFTWARE_SERIAL || defined ARDUINO_ARCH_RP2040)
  #define S8_RX_PIN 2
  #define S8_TX_PIN 0
#else
  #define S8_UART_PORT  1
#endif

#define COUNTDOWN (60) //seconds

/* END CONFIGURATION */

#ifdef USE_SOFTWARE_SERIAL
  SoftwareSerial S8_serial(S8_RX_PIN, S8_TX_PIN);
#else
  #if defined(ARDUINO_ARCH_RP2040)
    REDIRECT_STDOUT_TO(Serial)
    UART S8_serial(S8_TX_PIN, S8_RX_PIN, NC, NC);
  #else
    HardwareSerial S8_serial(S8_UART_PORT);
  #endif
#endif


S8_UART *sensor_S8;
S8_sensor sensor;


void setup() {
  Serial.begin(DEBUG_BAUDRATE);
  u8g2.begin();
  int i = 0;
  while (!Serial && i < 50) {
    delay(10);
    i++;
  }
  S8_serial.begin(S8_BAUDRATE);
  sensor_S8 = new S8_UART(S8_serial);
  sensor_S8->get_firmware_version(sensor.firm_version);
  int len = strlen(sensor.firm_version);
  if (len == 0) {
      Serial.println("SenseAir S8 CO2 sensor not found!");
      updateOLED2("SenseAir", "not", "found");
      while (1) { delay(1); };
  }
  Serial.println(">>> SenseAir S8 NDIR CO2 sensor <<<");
  printf("Firmware version: %s\n", sensor.firm_version);
  sensor.sensor_id = sensor_S8->get_sensor_ID();
  Serial.print("Sensor ID: 0x"); printIntToHex(sensor.sensor_id, 4); Serial.println("");
  Serial.println("Now, you put the sensor outside and wait.");
  Serial.println("Countdown begins...");
  unsigned int seconds = COUNTDOWN;
  while (seconds > 0) {
    printf("Time remaining: %d minutes %d seconds\n", seconds / 60, seconds % 60);
    updateOLED2("Wait", "for", String(seconds) + " Sec");
    delay(1000);
    seconds--;
  }
  Serial.println("Time reamining: 0 minutes 0 seconds");
  // Start manual calibration
  Serial.println("Starting manual calibration...");
  updateOLED2("Starting", "Manual", "Calibration");
  delay(2000);
  if (!sensor_S8->manual_calibration()) {
    Serial.println("Error setting manual calibration!");
    updateOLED2("Error", "Manual", "Calibration");
    while (1) { delay(10); }
  }
}

void loop() {
  static unsigned int elapsed = 0;
  delay(2000);
  elapsed += 2;

  // Check if background calibration is finished
  sensor.ack = sensor_S8->get_acknowledgement();
  if (sensor.ack & S8_MASK_CO2_BACKGROUND_CALIBRATION) {
    printf("Manual calibration is finished. Elapsed: %u seconds\n", elapsed);
    updateOLED2("Calibration", "finished", "");
    while (1) { delay(10); }
  } else {
    Serial.println("Doing manual calibration...");
    updateOLED2("Doing", "manual", "calibration");
  }
}

void updateOLED2(String ln1, String ln2, String ln3) {
      char buf[9];
          u8g2.firstPage();
          u8g2.firstPage();
          do {
          u8g2.setFont(u8g2_font_t0_16_tf);
          u8g2.drawStr(1, 10, String(ln1).c_str());
          u8g2.drawStr(1, 28, String(ln2).c_str());
          u8g2.drawStr(1, 46, String(ln3).c_str());
            } while ( u8g2.nextPage() );
}
