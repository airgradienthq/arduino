#pragma once

#include <SoftwareSerial.h>
#include "Sensors/ISensor.h"
#include "MHZ19.h"

#define WINSEN_MH_Z19_DEFAULT_BAUDRATE 9600


namespace AirGradient_Internal {
    class MHZ19Sensor : public ISensor {
    public:
        const char *getName() const override {
            return "Winsen MH-Z19";
        }

        MHZ19Sensor(uint8_t rxPin, uint8_t txPin, uint16_t baudRate) : _rxPin(rxPin),
                                                                       _txPin(txPin),
                                                                       _baudRate(baudRate) {}

        MHZ19Sensor() : MHZ19Sensor(D4, D3, WINSEN_MH_Z19_DEFAULT_BAUDRATE) {}

        bool begin() override;

        void getData(SensorData &data) const override;

    protected:
        Measurement getAvailableMeasurement() const override {
            return Measurement::CO2;
        }

    private:
        std::unique_ptr<SoftwareSerial> _softwareSerial;
        std::unique_ptr<MHZ19> _sensor;
        uint8_t _rxPin;
        uint8_t _txPin;
        uint16_t _baudRate;
    };
}

