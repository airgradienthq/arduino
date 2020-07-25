#include <AirGradient.h>
AirGradient ag = AirGradient();

void setup(){
  Serial.begin(9600);
  ag.TMP_RH_Init(0x44); //check for SHT sensor with address 0x44
}

void loop(){
  TMP_RH result = ag.periodicFetchData();
  Serial.print("Humidity: ");
  Serial.print(result.rh_char);
  Serial.print(" Temperature: ");
  Serial.println(result.t_char);
  delay(5000);
}
