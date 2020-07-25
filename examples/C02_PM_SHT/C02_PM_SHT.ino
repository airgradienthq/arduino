#include <AirGradient.h>
AirGradient ag = AirGradient();

void setup(){
  Serial.begin(9600);
  ag.PMS_Init();
  ag.CO2_Init();
  ag.TMP_RH_Init(0x44); //check for SHT sensor with address 0x44
}

void loop(){
    
Serial.print("PM2: ");
Serial.println(ag.getPM2());


Serial.print("CO2: ");
Serial.println(ag.getCO2());

TMP_RH result = ag.periodicFetchData();
Serial.print("Humidity: ");
Serial.print(result.rh);
Serial.print(" Temperature: ");
Serial.println(result.t);
delay(5000);
}
