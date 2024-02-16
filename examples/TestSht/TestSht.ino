#include <AirGradient.h>

#if defined(ESP8266)
AirGradient ag(DIY_BASIC);
#else
AirGradient ag(ONE_INDOOR);
#endif

void failedHandler(String msg);

void setup() {
  Serial.begin(115200);
  Serial.println("Hello");
  Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin());

  if (ag.sht.begin(Wire) == false) {
    failedHandler("SHT3x init failed");
  }
}

void loop() {
  if (ag.sht.measure()) {
    float hum = ag.sht.getRelativeHumidity();
    float temp = ag.sht.getTemperature();
    Serial.printf("Get temperature: %f\r\n", temp);
    Serial.printf("   Get humidity: %f\r\n", hum);
  } else {
    Serial.println("Measure failed");
  }
  delay(1000);
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}
