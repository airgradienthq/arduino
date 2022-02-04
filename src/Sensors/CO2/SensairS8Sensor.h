#include "s8_uart.h"
#include "Sensors/ISensor.h"

#define SENSAIR_S8_DEFAULT_BAUDRATE 9600

namespace AirGradient {
    class SensairS8Sensor : public ISensor {

    public:
        ~SensairS8Sensor() override = default;

        SensorType getType() const override;

        bool begin() override;

        void getData(SensorData &data) const override;

        inline const char *getName() const override {
            return "Sensair S8";
        }

        SensairS8Sensor(uint8_t rxPin, uint8_t txPin, uint16_t baudRate) : _rxPin(rxPin),
                                                                           _txPin(txPin),
                                                                           _baudRate(baudRate) {}

        SensairS8Sensor() : SensairS8Sensor(D4, D3, SENSAIR_S8_DEFAULT_BAUDRATE) {}

    private:
        std::unique_ptr<SoftwareSerial> _softwareSerial;
        std::unique_ptr<S8_UART> _sensor;
        uint8_t _rxPin;
        uint8_t _txPin;
        uint16_t _baudRate;
    };
}
