#include "BootTimeSensor.h"
#include "NTP/NTPClient.h"

AirGradient::SensorType AirGradient::BootTimeSensor::getType() const {
    return BootTime;
}

void AirGradient::BootTimeSensor::begin() {

    auto ntpClient = new NTPClient(_ntpServer);
    _bootTime = ntpClient->getUtcUnixEpoch();
}

void AirGradient::BootTimeSensor::updateData(AirGradient::SensorData &data) const {
    //No need to update the bootime more than once
    if (data.BOOT_TIME == 0) {
        data.BOOT_TIME = _bootTime;
    }
}

AirGradient::BootTimeSensor::BootTimeSensor(String ntpServer) : _ntpServer(std::move(ntpServer)) {}
