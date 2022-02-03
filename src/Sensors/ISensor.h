#pragma once

#include "Data/SensorData.h"
#include "Sensors/Data/Type.h"

namespace AirGradient {
    class ISensor {
    protected:
        ISensor();

    public:
        virtual ~ISensor() = default;

        /**
         * Type of measurement returned by the sensor
         * @return
         */
        virtual SensorType getType() const = 0;

        /**
         * To initialize the sensor
         */
        virtual void begin() = 0;

        /**
         * To update the data structure containing the data of all sensors
         * @param data
         */
        virtual void updateData(SensorData &data) const = 0;
    };

}