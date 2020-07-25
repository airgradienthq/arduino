#include <AirGradient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

AirGradient ag = AirGradient();
#define OLED_RESET 0
Adafruit_SSD1306 display(OLED_RESET);

void setup(){
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); 
  ag.PMS_Init();
  ag.C02_Init();
  ag.TMP_RH_Init(0x44); //check for SHT sensor with address 0x44
  showTextRectangle("Init", String(ESP.getChipId(),HEX),"AirGradient");
  delay(2000);
}

void loop(){
  int PM2 = ag.getPM2();
  int CO2 = ag.getC02();
  TMP_RH result = ag.periodicFetchData();
  showTextRectangle(String(result.t)+"c "+String(result.rh)+"%", "PM2: "+ String(PM2), "CO2: "+String(CO2)+"");
  delay(5000);
}

// DISPLAY
void showTextRectangle(String ln1, String ln2, String ln3) {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(32,8);
  display.println(ln1);
  display.setTextSize(1);
  display.setCursor(32,16);
  display.println(ln2);
  display.setTextSize(1);
  display.setCursor(32,24);
  display.println(ln3);
  display.display();
}
