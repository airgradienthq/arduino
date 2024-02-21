#include <Arduino.h>
#include <Wire.h>

#include "SHTSensor.h"

SHT3xAnalogSensor sht3xAnalog(A0, A1);

void setup() {
  // put your setup code here, to run once:

  Wire.begin();
  Serial.begin(9600);

  delay(1000); // let serial console settle
}

void loop() {
  Serial.print("SHT3x Analog:\n");
  Serial.print("  RH: ");
  Serial.print(sht3xAnalog.readHumidity(), 2);
  Serial.print("\n");
  Serial.print("  T:  ");
  Serial.print(sht3xAnalog.readTemperature(), 2);
  Serial.print("\n");

  delay(1000);
}
