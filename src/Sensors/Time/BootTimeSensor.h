#include <Arduino.h>
#include "Sensors/ISensor.h"

namespace AirGradient {
    class BootTimeSensor : public ISensor {
    public:
        SensorType getType() const override;

        bool begin() override;

        void getData(SensorData &data) const override;


        inline const char *getName() const override {
            return "NTP Client";
        }

        BootTimeSensor(const char *ntpServer) : _ntpServer(ntpServer) {}

    private:
        const char* _ntpServer;
        time_t _bootTime;
    };
}
