#pragma once

#include <SoftwareSerial.h>
#include <Ticker.h>
#include "ISensor.h"
#include "PMS.h"

#define PMS_DEFAULT_BAUDRATE 9600
#define PMS_DEFAULT_WAKE_SECS 120
#define PMS_DELAY_BEFORE_READING_SECS 30

namespace AirGradient {
    class PMSXSensor : public ISensor {


    public:

        /**
         * Constructor for the sensor
         * @param rxPin receiving pin
         * @param txPin transmitting pin
         * @param baudRate baudrate
         * @param wakupXSecs the sensor will only wakeup every X seconds to take a reading before going back to sleep to save on the lifespan.
         */
        PMSXSensor(uint8_t rxPin, uint8_t txPin, uint16_t baudRate, uint16_t wakupXSecs);

        PMSXSensor();

        void begin() override;

        SensorType getType() const override {
            return Particle;
        }

        void updateData(SensorData &data) const override;

        ~PMSXSensor() override = default;

    private:
        std::unique_ptr<SoftwareSerial> _softwareSerial;
        uint8_t _rxPin;
        uint8_t _txPin;
        uint16_t _baudRate;
        uint16_t _wakupXSecs;

        ParticleData _data{0};

        Ticker _wakerTicker;
        Ticker _readSleepTicker;
        std::unique_ptr<PMS> _sensor;

        void _wakeUpPm2();

        void _getPm2DataSleep();


    };
}

