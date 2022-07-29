/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

For build instructions please visit https://www.airgradient.com/open-airgradient/instructions/

Compatible with the following sensors:
SHT30/31 (Temperature/Humidity Sensor)

Please install ESP8266 board manager (tested with version 3.0.0)

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/

Kits with all required components are available at https://www.airgradient.com/open-airgradient/shop/

If you have any questions please visit our forum at https://forum.airgradient.com/

MIT License
*/

#include <AirGradient.h>
AirGradient ag = AirGradient();

void setup(){
  Serial.begin(9600);
  ag.TMP_RH_Init(0x44); //check for SHT sensor with address 0x44
}

void loop(){

  TMP_RH result = ag.periodicFetchData();

  Serial.print("Relative Humidity in %: ");
  Serial.println(result.rh);

  Serial.print(" Temperature in Celcius: ");
  Serial.println(result.t);

  Serial.print(" Temperature in Fahrenheit: ");
  Serial.println((result.t * 9 / 5) + 32);

  delay(5000);
}
