#include <HardwareSerial.h>
#include "SHTXSensor.h"

AirGradient::Measurement AirGradient::SHTXSensor::getAvailableMeasurement() const {
    return Measurement::Temperature | Measurement::Humidity;
}

bool AirGradient::SHTXSensor::begin() {
    _sensor = std::make_unique<SHTSensor>();
    _sensor->setAccuracy(_accuracy);
    return _sensor->init();
}

void AirGradient::SHTXSensor::getData(AirGradient::SensorData &data) const {
    if (!_sensor->readSample()) {
        Serial.println("Can't read SHT sensor data");
        return;
    }
    auto measurement = getCurrentMeasurement();

    //Check if sensor is supposed to provide the Temperature reading
    if (!(measurement & Measurement::Temperature)) {
        data.TMP = _sensor->getTemperature();
    }

    //Check if sensor is supposed to provide the humidity reading
    if (!(measurement & Measurement::Humidity)) {
        data.HUM = _sensor->getHumidity();
    }

}
