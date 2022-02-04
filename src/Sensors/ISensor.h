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
         * Name of the sensor, useful for debugging message.
         * @return
         */
        inline virtual const char* getName() const = 0;

        /**
         * Type of measurement returned by the sensor
         * @return
         */
        virtual SensorType getType() const = 0;

        /**
         * To initialize the sensor
         */
        virtual bool begin() = 0;

        /**
         * To update the data structure containing the data of all sensors
         * @param data
         */
        virtual void updateData(SensorData &data) const = 0;
    };

}