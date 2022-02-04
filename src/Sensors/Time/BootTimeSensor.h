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

        BootTimeSensor(String ntpServer);


    private:
        String _ntpServer;
        time_t _bootTime;
    };
}
