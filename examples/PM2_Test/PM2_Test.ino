#include <AirGradient.h>
AirGradient ag = AirGradient();

void setup(){
  Serial.begin(9600);
  ag.PMS_Init();
}

void loop(){
    
int PM2 = ag.getPM2_Raw();
Serial.print("PM2: ");
Serial.println(ag.getPM2());

delay(5000);
}
