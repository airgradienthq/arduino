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
  float temp = ag.sht3x.getTemperature();
  if (temp <= -256.0f) {
    Serial.println("Get temperature failed");
  } else {
    Serial.printf("Get temperature: %f\r\n", temp);
  }

  float hum = ag.sht3x.getRelativeHumidity();
  if (hum < 0) {
    Serial.println("Get humidity failed");
  } else {
    Serial.printf("Get humidity: %f\r\n", hum);
  }

  delay(1000);
}

void failedHandler(String msg) {
  while (true) {
    Serial.println(msg);
    delay(1000);
  }
}
