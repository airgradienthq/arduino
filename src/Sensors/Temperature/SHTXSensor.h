#pragma once

#include <memory>
#include "Sensors/ISensor.h"
#include "SHTSensor.h"

#define SHT_TMP_OFFSET_DEFAULT -2
namespace AirGradient {
    class SHTXSensor : public ISensor {
    public:
        ~SHTXSensor() override = default;


        inline const char *getName() const override {
            return "Sensirion SHT";
        }

        bool begin() override;

        void getData(SensorData &data) const override;

        /**
         *
         * @param accuracy Accuracy of the sensor (also define how fast does it take to do a reading)
         */
        SHTXSensor(SHTSensor::SHTAccuracy accuracy) : _accuracy(accuracy) {}

        SHTXSensor() : SHTXSensor(SHTSensor::SHTAccuracy::SHT_ACCURACY_HIGH) {}

    protected:
        Measurement getAvailableMeasurement() const override;

    private:
        SHTSensor::SHTAccuracy _accuracy;
        std::unique_ptr<SHTSensor> _sensor;
    };
}
