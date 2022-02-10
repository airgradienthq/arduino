/*
This is the code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

For build instructions please visit https://www.airgradient.com/diy/

Compatible with the following sensors:
Plantower PMS5003 (Fine Particle Sensor)

Please install ESP8266 board manager (tested with version 3.0.0)

If you have any questions please visit our forum at https://forum.airgradient.com/

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/schools/

Kits with all required components are available at https://www.airgradient.com/diyshop/

MIT License
*/

#include <AirGradient.h>

auto metrics = std::make_shared<MetricGatherer>();

void setup() {
    Serial.begin(9600);
    metrics->addSensor(std::make_unique<PMSXSensor>())
    metrics->begin();
}

void loop() {
    auto data = metrics->getData();
    auto pm25 = data.PARTICLE_DATA.PM_2_5;
    Serial.print("PM 2.5: ");
    Serial.println(pm25);

    delay(5000);
}
