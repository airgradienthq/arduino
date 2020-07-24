#include <AirGradient.h>
AirGradient ag = AirGradient();

void setup(){
  Serial.begin(9600);
  ag.PMS_Init();
  ag.C02_Init();
  ag.TMP_RH_Init(0x44); //check for SHT sensor with address 0x44
}

void loop(){
    
int PM2 = ag.getPM2();
Serial.print("PM2: ");
Serial.println(PM2);

int CO2 = ag.getC02();
Serial.print("C02: ");
Serial.println(CO2);

TMP_RH result = ag.periodicFetchData();
Serial.print("Humidity: ");
Serial.print(result.rh);
Serial.print(" Temperature: ");
Serial.println(result.t);
delay(5000);
}
