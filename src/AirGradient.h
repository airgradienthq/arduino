#pragma once

#include "Metrics/MetricGatherer.h"
#include "Prometheus/PrometheusServer.h"
#include "Sensors/CO2/SensairS8Sensor.h"
#include "Sensors/CO2/MHZ19Sensor.h"
#include "Sensors/Particle/PMSXSensor.h"
#include "Sensors/Temperature/SHTXSensor.h"
#include "Sensors/Time/BootTimeSensor.h"
#include "Sensors/TVOC/SGP30Sensor.h"
#include "AQI/AQICalculator.h"

using namespace AirGradient_Internal;

/**
 * Please don't use, only here for backward compatibility. Use the MetricGather instead.
 */
class AirGradient {

private:
    MetricGatherer _metrics;
public:
    struct TMP_RH {
        float t;
        float rh;
    };

    AirGradient() = default;

    /**
     * Add the PMS Sensor
     */
    void PMS_Init();

    /**
     * Add the Sensair S8
     */
    void CO2_Init();

    /**
     * Add the Temp sensor SHT3x
     */
    void TMP_RH_Init();

    /**
     * Add the Temp sensor MHZ19
     * @param rxPin
     * @param txPin
     */
    void MHZ19_Init();

    /**
     * Start gathering the sensor data
     */
    void begin() { _metrics.begin(); }

    /**
     * Get the sensor data
     * @return
     */
    inline SensorData getData() const { return _metrics.getData(); }

    /**
     * Get PM2.5 particle
     * @return
     */
    uint16_t getPM2_Raw() const { return getData().PARTICLE_DATA.PM_2_5; }

    /**
     * Get CO2 gaz
     * @return
     */
    uint16_t getCO2_Raw() const { return getData().GAS_DATA.CO2; }

    /**
     * Get the Temperature and humidity
     * @return
     */
    TMP_RH periodicFetchData() const {
        const SensorData data = getData();
        return TMP_RH{
                data.TMP,
                data.HUM
        };
    }
};
