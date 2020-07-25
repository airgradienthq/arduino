#include <AirGradient.h>
AirGradient ag = AirGradient();

void setup(){
  Serial.begin(9600);
  ag.PMS_Init();
}

void loop(){
    
int PM2 = ag.getPM2();
Serial.print("PM2: ");
Serial.println(PM2);

delay(5000);
}