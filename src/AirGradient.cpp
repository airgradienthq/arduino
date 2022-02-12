#include "AirGradient.h"

void AirGradient::PMS_Init() {
    _metrics.addSensor(std::make_unique<PMSXSensor>());
}

void AirGradient::CO2_Init() {
    _metrics.addSensor(std::make_unique<SensairS8Sensor>());
}

void AirGradient::TMP_RH_Init() {
    _metrics.addSensor(std::make_unique<SHTXSensor>());
}

void AirGradient::MHZ19_Init() {
    _metrics.addSensor(std::make_unique<MHZ19Sensor>());
}
