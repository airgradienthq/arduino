#include <Arduino.h>
#include "Sensors/ISensor.h"

namespace AirGradient {
    class BootTimeSensor : public ISensor {
    public:
        SensorType getType() const override;

        void begin() override;

        void updateData(SensorData &data) const override;

        BootTimeSensor(String ntpServer);

    private:
        String _ntpServer;
        time_t _bootTime;
    };
}
