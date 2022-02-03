#pragma once

#include <cstdint>
#include <ctime>

namespace AirGradient {
    struct ParticleData {
        // Atmospheric environment
        uint16_t PM_1_0;
        uint16_t PM_2_5;
        uint16_t PM_10_0;
    };
    struct SensorData {
        uint16_t CO2 = 0;
        ParticleData PARTICLE_DATA{};
        float TMP = 0;
        float HUM = 0;
        time_t BOOT_TIME = 0;
    };
}
