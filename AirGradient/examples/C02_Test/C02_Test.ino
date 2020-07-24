#include <AirGradient.h>
AirGradient ag = AirGradient();

void setup(){
  Serial.begin(9600);
  ag.C02_Init();
}

void loop(){

int CO2 = ag.getC02();
Serial.print("C02: ");
Serial.println(CO2);

delay(5000);
}