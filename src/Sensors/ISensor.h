#pragma once

#include "Data/SensorData.h"
#include "Sensors/Data/Type.h"

namespace AirGradient {
    class ISensor {
    protected:
        ISensor() = default;

    public:
        virtual ~ISensor() = default;

        /**
         * Name of the sensor, useful for debugging message.
         * @return
         */
        inline virtual const char *getName() const = 0;

        /**
         * Type of measurement returned by the sensor
         * @return
         */
        virtual Measurement getAvailableMeasurement() const = 0;

        /**
         * To initialize the sensor
         */
        virtual bool begin() = 0;

        /**
         * To update the data structure containing the data of all sensors
         * @param data
         */
        virtual void getData(SensorData &data) const = 0;

        /***
         * Set what measurement are excluded
         * @param excludedMeasurement
         */
        void setExcludedMeasurement(Measurement excludedMeasurement) {
            _excludedMeasurement = excludedMeasurement;
        }

        /**
         * Get measurement to that is currently required from the sensor
         * @return
         */
        inline Measurement getCurrentMeasurement() const {
            return getAvailableMeasurement() & ~_excludedMeasurement;
        }

    private:
        Measurement _excludedMeasurement{Measurement::None};
    };

}