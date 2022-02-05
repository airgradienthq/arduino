#include "MHZ19Sensor.h"

bool AirGradient::MHZ19Sensor::begin() {
    _softwareSerial = std::make_unique<SoftwareSerial>(_rxPin, _txPin);
    _softwareSerial->begin(_baudRate);
    _sensor = std::make_unique<MHZ19>();
    _sensor->begin(*_softwareSerial);
    _sensor->autoCalibration();

    return _sensor->errorCode == RESULT_OK;
}

void AirGradient::MHZ19Sensor::getData(AirGradient::SensorData &data) const {
    data.GAS_DATA.CO2 = _sensor->getCO2();
}
