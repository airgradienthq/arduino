#pragma once

#include "Arduino.h"
#include "AQI/MovingAverage.h"
#include "Metrics/MetricGatherer.h"

#define GATHER_METRIC_EVERY_X_SECS 900
#define GATHER_PERIOD_SECS 86400
#define NUM_OF_METRICS GATHER_PERIOD_SECS/GATHER_METRIC_EVERY_X_SECS


namespace AirGradient {
    struct Breakpoints {
        uint16_t iHi;
        uint16_t iLo;
        float cHi;
        float cLo;
    };

    class Calculator {
    public:
        Calculator(std::shared_ptr<MetricGatherer> metrics);

        /**
         * Check if we have enough reading to give a proper AQI.
         * (since we need a 24h avg of PM2.5 to do AQI calculation)
         * @return
         */
        bool isAQIAvailable() const { return _average.hasReachCapacity(); }

        /**
         * Get the current AQI, doesn't check if we have enough readings.
         * @return
         */
        float getAQI() const;

        virtual ~Calculator();

    private:
        std::shared_ptr<MetricGatherer> _metrics;
        Ticker _ticker;
        MovingAverage<uint16_t, uint16_t, NUM_OF_METRICS> _average;

        Breakpoints _getPM25Breakpoints(float pm25Avg) const;

        float _getAQI(Breakpoints breakpoints, float pm25Avg) const;

        void _recordMetric();
    };
}



