/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

For build instructions please visit https://www.airgradient.com/open-airgradient/instructions/

Compatible with the following sensors:
SenseAir S8 (CO2 Sensor)

Please install ESP8266 board manager (tested with version 3.0.0)

If you have any questions please visit our forum at https://forum.airgradient.com/

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/

Kits with all required components are available at https://www.airgradient.com/open-airgradient/shop/

CC BY-SA 4.0 Attribution-ShareAlike 4.0 International License

*/

#include <AirGradient.h>
AirGradient ag = AirGradient();

void setup(){
  Serial.begin(115200);
  ag.CO2_Init();
}

void loop(){

int CO2 = ag.getCO2_Raw();
Serial.print("C02: ");
Serial.println(ag.getCO2());

delay(5000);
}
