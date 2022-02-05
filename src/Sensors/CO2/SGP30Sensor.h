#pragma once

#include <memory>
#include <Ticker.h>
#include "Sensors/ISensor.h"
#include "SparkFun_SGP30_Arduino_Library.h"

namespace AirGradient {
    class SGP30Sensor : public ISensor {
    public:
        const char *getName() const override {
            return "Sensirion SGP30";
        }

        SensorType getType() const override {
            return SensorType::TVOC | SensorType::CO2 | SensorType::ETHANOL | SensorType::H2;
        }

        bool begin() override;

        void getData(SensorData &data) const override;

        SGP30Sensor(TwoWire &wirePort) : _wirePort(wirePort) {}

        SGP30Sensor() : SGP30Sensor(Wire) {}

    private:
        TwoWire &_wirePort;
        std::unique_ptr<SGP30> _sensor;
    };
}
