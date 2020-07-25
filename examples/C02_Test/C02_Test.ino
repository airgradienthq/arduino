#include <AirGradient.h>
AirGradient ag = AirGradient();

void setup(){
  Serial.begin(9600);
  ag.CO2_Init();
}

void loop(){

int CO2 = ag.getCO2_Raw();
Serial.print("C02: ");
Serial.println(ag.getCO2());

delay(5000);
}
