#include <HardwareSerial.h>
#include "MetricGatherer.h"

AirGradient::MetricGatherer &AirGradient::MetricGatherer::addSensor(std::unique_ptr<ISensor> sensor,
                                                                    Measurement excludedMeasurement) {
    sensor->setExcludedMeasurement(excludedMeasurement);
    auto measurement = sensor->getCurrentMeasurement();
    //If because of the exclusion, the sensor won't provide any _measurements, don't add it.
    if (measurement == Measurement::None) {
        Serial.printf("%s will not provide any _measurements. Not adding sensor.\n", sensor->getName());
        return *this;
    }

    for (auto const &currentSensor: _sensors) {
        //If there is already a sensor providing this data, we need to tell the developer
        if (!(currentSensor->getCurrentMeasurement() & measurement)) {
            Serial.printf("[Conflict %s & %s] Already have a sensor with the type: %d\n",
                          currentSensor->getName(), sensor->getName(), enumAsInt(measurement));
            return *this;
        }
    }
    _measurements |= measurement;
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
        sensor->getData(_data);
    }

    // Only apply the offset if there is a temperature _measurements configured
    if (!(_measurements & Measurement::Temperature)) {
        _data.TMP = _data.TMP + _temperatureOffset;
    }
}
