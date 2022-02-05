#include "AQICalculator.h"

#include <utility>

namespace AirGradient {
    Breakpoints AQICalculator::_getPM25Breakpoints(float pm25Avg) const {
        Breakpoints b{};

        if (pm25Avg <= 12) {
            b.iHi = 50;
            b.iLo = 0;
            b.cHi = 12;
            b.cLo = 0;
        } else if (pm25Avg > 12 && pm25Avg <= 35.4) {
            b.iHi = 100;
            b.iLo = 51;
            b.cHi = 35.4;
            b.cLo = 12.1;
        } else if (pm25Avg > 35.4 && pm25Avg <= 55.4) {
            b.iHi = 150;
            b.iLo = 101;
            b.cHi = 55.4;
            b.cLo = 35.5;
        } else if (pm25Avg > 55.4 && pm25Avg <= 150.4) {
            b.iHi = 200;
            b.iLo = 151;
            b.cHi = 150.4;
            b.cLo = 55.5;
        } else if (pm25Avg > 150.4 && pm25Avg <= 250.4) {
            b.iHi = 300;
            b.iLo = 201;
            b.cHi = 250.4;
            b.cLo = 150.5;
        } else if (pm25Avg > 250.4 && pm25Avg <= 350.4) {
            b.iHi = 400;
            b.iLo = 301;
            b.cHi = 350.4;
            b.cLo = 250.5;
        } else if (pm25Avg > 350.4) {
            b.iHi = 500;
            b.iLo = 401;
            b.cHi = 500.4;
            b.cLo = 350.5;
        }

        return b;
    }

    float AQICalculator::_getAQI(Breakpoints breakpoints, float pm25Avg) const{
        return static_cast<float>((breakpoints.iHi - breakpoints.iLo))
               * (pm25Avg - breakpoints.cLo)
               / (breakpoints.cHi - breakpoints.cLo) + static_cast<float>(breakpoints.iLo);
    }

    AQICalculator::AQICalculator(std::shared_ptr<MetricGatherer> metrics) : _metrics(std::move(metrics)) {

    }

    float AQICalculator::getAQI() const {
        auto pm2Avg = _average.getAverage();
        auto breakpoint = _getPM25Breakpoints(pm2Avg);
        return _getAQI(breakpoint, pm2Avg);
    }

    AQICalculator::~AQICalculator() {

    }

    void AQICalculator::_recordMetric() {
        auto pm25 = _metrics->getData().PARTICLE_DATA.PM_2_5;
        _average.addSample(pm25);
        Serial.printf("Record PM2.5 (%d) for AVG: (%f)\n", pm25, _average.getAverage());
    }

    void AQICalculator::begin() {
        //Don't have a particle sensor
        if(!!(_metrics->getMeasurements() & Measurement::Particle)) {
            Serial.println("No particle sensor to use to calculate PM2.5 AQI");
            return;
        }
        _ticker.attach(GATHER_METRIC_EVERY_X_SECS, [this] { _recordMetric(); });
    }
}
