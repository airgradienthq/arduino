#include "BootTimeSensor.h"
#include "NTP/NTPClient.h"

AirGradient::Measurement AirGradient::BootTimeSensor::getAvailableMeasurement() const {
    return Measurement::BootTime;
}

bool AirGradient::BootTimeSensor::begin() {
    //Check if sensor is supposed to provide the Boot reading, if not, return directly
    if (!!(getCurrentMeasurement() & Measurement::BootTime)) {
        Serial.printf("%s can only provide BootTime reading and it's disabled.", getName());
        return false;
    }
    auto ntpClient = std::make_unique<NTPClient>(_ntpServer);
    _bootTime = ntpClient->getUtcUnixEpoch();
    return _bootTime > 0;
}

void AirGradient::BootTimeSensor::getData(AirGradient::SensorData &data) const {
    //No need to update the bootime more than once
    if (data.BOOT_TIME == 0) {
        data.BOOT_TIME = _bootTime;
    }
}
