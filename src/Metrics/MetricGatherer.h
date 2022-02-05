#pragma once

#include <memory>
#include <vector>
#include <Ticker.h>
#include "Sensors/ISensor.h"
#include "Sensors/Data/Measurement.h"

#define METRIC_GATHERING_EVERY_X_SECS 3
namespace AirGradient {
    class MetricGatherer {
    public:
        /**
         * Add a sensor to gather the metric from. Check if we don't have already a sensor for that metric.
         * @param sensor sensor to add
         * @param excludedMeasurement measurement that shouldn't be recorded by the sensor. Default: None
         * @return the same instance of the MetricGatherer to easily add new sensor
         */
        AirGradient::MetricGatherer &addSensor(
                std::unique_ptr<ISensor> sensor,
                Measurement excludedMeasurement = Measurement::None
        );

        /**
         * To setup all the different sensor and the gathering of data.<br />
         * <b>Only call this AFTER registering all the sensors with addSensor</b>
         */
        void begin();

        /**
         * Get the latest sensor data
         * @return
         */
        inline SensorData getData() const { return _data; }

        /**
         * What type of sensor are we gathering metric for.
         * @return
         */
        inline Measurement getMeasurements() const { return _measurements; }

        MetricGatherer(uint8_t gatherMetricEverySecs, float temperatureOffset) :
                _gatherMetricEverySecs(gatherMetricEverySecs),
                _temperatureOffset(temperatureOffset) {}

        MetricGatherer(float temperatureOffset) : MetricGatherer(METRIC_GATHERING_EVERY_X_SECS, temperatureOffset) {}

        MetricGatherer() : MetricGatherer(METRIC_GATHERING_EVERY_X_SECS, 0) {}

    private:
        std::vector<std::unique_ptr<ISensor>> _sensors;
        Measurement _measurements;
        SensorData _data;
        bool _initialized{false};
        uint8_t _gatherMetricEverySecs;
        Ticker _getMetricsTicker;
        float _temperatureOffset{0};

        void _gatherMetrics();
    };
}
