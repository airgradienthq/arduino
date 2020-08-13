#include <AirGradient.h>
#include <Wire.h>
#include "SSD1306Wire.h"

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

void setup(){
  Serial.begin(9600);
  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(),HEX));

  ag.PMS_Init();
  ag.CO2_Init();
  ag.TMP_RH_Init(0x44);

  delay(2000);
}

void loop(){
  int PM2 = ag.getPM2_Raw();
  int CO2 = ag.getCO2_Raw();
  TMP_RH result = ag.periodicFetchData();
  showTextRectangle(String(result.t),String(result.rh)+"%");
  delay(2000);
  showTextRectangle("PM2",String(PM2));
  delay(2000);
  showTextRectangle("CO2",String(CO2));
  delay(2000);
}

// DISPLAY
void showTextRectangle(String ln1, String ln2) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(32, 12, ln1);
  display.drawString(32, 36, ln2);
  display.display();
}
