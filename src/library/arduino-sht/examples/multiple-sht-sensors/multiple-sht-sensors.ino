#include <Wire.h>
#include "SHTSensor.h"

// Note that all i2c devices sharing one bus must have distinct addresses. Thus
// this example only works with sensors that have distinct i2c addresses (e.g.
// SHT3x and SHT3x_alt, or SHT3x and SHTC3).
// Make sure not to use auto-detection as it will only pick up one sensor.

// Sensor with normal i2c address
// Sensor 1 with address pin pulled to GND
SHTSensor sht1(SHTSensor::SHT3X);

// Sensor with alternative i2c address
// Sensor 2 with address pin pulled to Vdd
SHTSensor sht2(SHTSensor::SHT3X_ALT);

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  delay(1000); // let serial console settle

  // init on a specific sensor type (i.e. without auto detecting),
  // does not check if the sensor is responding and will thus always succeed.

  // initialize sensor with normal i2c-address
  sht1.init();

  // initialize sensor with alternative i2c-address
  sht2.init();
}

void loop() {
  // put your main code here, to run repeatedly:
  // read from first sensor
  if (sht1.readSample()) {
    Serial.print("SHT1 :\n");
    Serial.print("  RH: ");
    Serial.print(sht1.getHumidity(), 2);
    Serial.print("\n");
    Serial.print("  T:  ");
    Serial.print(sht1.getTemperature(), 2);
    Serial.print("\n");
  } else {
    Serial.print("Sensor 1: Error in readSample()\n");
  }

  // read from second sensor
  if (sht2.readSample()) {
    Serial.print("SHT2:\n");
    Serial.print("  RH: ");
    Serial.print(sht2.getHumidity(), 2);
    Serial.print("\n");
    Serial.print("  T:  ");
    Serial.print(sht2.getTemperature(), 2);
    Serial.print("\n");
  } else {
    Serial.print("Sensor 2: Error in readSample()\n");
  }

  delay(1000);
}
