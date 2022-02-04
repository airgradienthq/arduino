#pragma once

#include <memory>
#include "Sensors/ISensor.h"
#include "SHTSensor.h"

#define SHT_TMP_OFFSET_DEFAULT -2
namespace AirGradient {
    class SHTXSensor : public ISensor {
    public:
        ~SHTXSensor() override = default;

        SensorType getType() const override;

        bool begin() override;

        void updateData(SensorData &data) const override;

        /**
         *
         * @param accuracy Accuracy of the sensor (also define how fast does it take to do a reading)
         * @param tempOffset Offset that will be added to the temperature reading. Useful when the sensor is next to a source of heat.
         */
        SHTXSensor(SHTSensor::SHTAccuracy accuracy, int8_t tempOffset) : _accuracy(accuracy), _tempOffset(tempOffset) {}

        SHTXSensor() : SHTXSensor(SHTSensor::SHTAccuracy::SHT_ACCURACY_HIGH, SHT_TMP_OFFSET_DEFAULT) {}

    private:
        SHTSensor::SHTAccuracy _accuracy;
        int8_t _tempOffset;

        std::unique_ptr<SHTSensor> _sensor;
    };
}
