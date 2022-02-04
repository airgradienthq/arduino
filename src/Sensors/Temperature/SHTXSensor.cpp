#include <HardwareSerial.h>
#include "SHTXSensor.h"

AirGradient::SensorType AirGradient::SHTXSensor::getType() const {
    return SensorType::Temperature | SensorType::Humidity;
}

bool AirGradient::SHTXSensor::begin() {
    _sensor = std::make_unique<SHTSensor>();
    _sensor->setAccuracy(_accuracy);
    return _sensor->init();
}

void AirGradient::SHTXSensor::getData(AirGradient::SensorData &data) const {
    if (!_sensor->readSample()) {
        Serial.println("Can't read SHT sensor data");

    } else {
        data.TMP = _sensor->getTemperature() + _tempOffset;
        data.HUM = _sensor->getHumidity();
    }
}
