#include <HardwareSerial.h>
#include "MetricGatherer.h"

AirGradient::MetricGatherer &AirGradient::MetricGatherer::addSensor(std::unique_ptr<ISensor> sensor) {
    for (auto const &currentSensor: _sensors) {
        auto type = currentSensor->getType() & sensor->getType();
        if (type == sensor->getType() || type == currentSensor->getType()) {
            Serial.printf("[Conflict %s & %s] Already have a sensor with the type: %hhu\n",
                          currentSensor->getName(), sensor->getName(), sensor->getType());
            return *this;
        }
    }
    _sensorTypes |= sensor->getType();
    _sensors.push_back(std::move(sensor));
    return *this;
}

void AirGradient::MetricGatherer::begin() {
    for (auto const &sensor: _sensors) {
        if (!sensor->begin()) {
            Serial.printf("Can't init sensor: %s\nStopping initialization.", sensor->getName());
            return;
        }
    }

    _initialized = true;
    _getMetricsTicker.attach_scheduled(_gatherMetricEverySecs, [this] { _gatherMetrics(); });
    _gatherMetrics();

}

void AirGradient::MetricGatherer::_gatherMetrics() {
    if (!_initialized) {
        Serial.println("Gather not initialized properly.");
    }
    for (auto const &sensor: _sensors) {
        sensor->updateData(_data);
    }
}
