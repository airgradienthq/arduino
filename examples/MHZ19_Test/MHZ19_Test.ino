#include <AirGradient.h>
AirGradient ag = AirGradient();

void setup(){
  Serial.begin(9600);
  ag.MHZ19_Init(MHZ19B);
}

void loop(){
    
int MHZ19_C02 = ag.readMHZ19();
Serial.print("C02: ");
Serial.println(MHZ19_C02);

delay(5000);
}
