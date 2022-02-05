#pragma once

#include <memory>
#include <vector>
#include <Ticker.h>
#include "Sensors/ISensor.h"

#define METRIC_GATHERING_EVERY_X_SECS 3
namespace AirGradient {
    class MetricGatherer {
    public:
        /**
         * Add a sensor to gather the metric from. Check if we don't have already a sensor for that metric.
         * @param sensor
         * @return the same instance of the MetricGatherer to easily add new sensor
         */
        AirGradient::MetricGatherer &addSensor(std::unique_ptr<ISensor> sensor, Measurement excludedMeasurement = Measurement::None);

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
        inline Measurement getSensorTypes() const { return _sensorTypes; }

        MetricGatherer(uint8_t gatherMetricEverySecs) : _gatherMetricEverySecs(gatherMetricEverySecs) {}

        MetricGatherer() : MetricGatherer(METRIC_GATHERING_EVERY_X_SECS) {}

    private:
        std::vector<std::unique_ptr<ISensor>> _sensors;
        Measurement _sensorTypes;
        SensorData _data;
        bool _initialized{false};
        uint8_t _gatherMetricEverySecs;
        Ticker _getMetricsTicker;

        void _gatherMetrics();
    };
}
