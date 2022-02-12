#pragma once

#include <cstdint>
#include <ctime>

namespace AirGradient_Internal {

    struct GasData {
        uint16_t CO2 = 0;
        uint16_t TVOC = 0;
        uint16_t H2 = 0;
        uint16_t ETHANOL = 0;
    };
    struct ParticleData {
        // Atmospheric environment
        uint16_t PM_1_0;
        uint16_t PM_2_5;
        uint16_t PM_10_0;
    };
    struct SensorData {
        GasData GAS_DATA{};
        ParticleData PARTICLE_DATA{};
        float TMP = 0;
        float HUM = 0;
        time_t BOOT_TIME = 0;
    };
}
