#include "BootTimeSensor.h"
#include "NTP/NTPClient.h"

AirGradient::SensorType AirGradient::BootTimeSensor::getType() const {
    return SensorType::BootTime;
}

bool AirGradient::BootTimeSensor::begin() {
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
