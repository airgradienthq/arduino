#include <AirGradient.h>

AirGradient ag(DIY_BASIC);

void failedHandler(String msg);

void setup() {
  Serial.begin(115200);
  Wire.begin(ag.getI2cSdaPin(), ag.getI2cSclPin());

  if (ag.sht3x.begin(Wire, Serial) == false) {
    failedHandler("SHT3x init failed");
  }
}

void loop() {
  if (ag.sht3x.measure()) {
    float hum = ag.sht3x.getRelativeHumidity();
    float temp = ag.sht3x.getTemperature();

    Serial.printf("Get temperature: %f\r\n", temp);
    Serial.printf("   Get humidity: %f\r\n", hum);
  } else {
    Serial.println("Measure sht3x failed");
  }
  delay(5000);
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}
