//
// Created by Antoine on 2022-02-04.
//

#include "SGP30Sensor.h"

double RHtoAbsolute(float relHumidity, float tempC) {
    double eSat = 6.11 * pow(10.0, (7.5 * tempC / (237.7 + tempC)));
    double vaporPressure = (relHumidity * eSat) / 100; //millibars
    double absHumidity = 1000 * vaporPressure * 100 / ((tempC + 273) * 461.5); //Ideal gas law with unit conversions
    return absHumidity;
}

uint16_t doubleToFixedPoint(double number) {
    int power = 1 << 8;
    double number2 = number * power;
    uint16_t value = floor(number2 + 0.5);
    return value;
}

bool AirGradient_Internal::SGP30Sensor::begin() {
    _sensor = std::make_unique<SGP30>();
    auto result = _sensor->begin(_wirePort);
    if (result) {
        _sensor->initAirQuality();
    }
    return result;
}

void AirGradient_Internal::SGP30Sensor::getData(AirGradient_Internal::SensorData &data) const {
    //Set humidity for the sensor
    if (data.TMP > 0 && data.HUM > 0) {
        auto absHumidity = RHtoAbsolute(data.HUM, data.TMP);
        auto sensHumidity = doubleToFixedPoint(absHumidity);
        _sensor->setHumidity(sensHumidity);
    }

    auto measurement = getCurrentMeasurement();

    _sensor->measureAirQuality();
    //Check if sensor is supposed to provide the CO2 reading
    if (!(measurement & Measurement::CO2)) {
        data.GAS_DATA.CO2 = _sensor->CO2;
    }

    //Check if sensor is supposed to provide the TVOC reading
    if (!(measurement & Measurement::TVOC)) {
        data.GAS_DATA.TVOC = _sensor->TVOC;
    }

    _sensor->measureRawSignals();
    //Check if sensor is supposed to provide the H2 reading
    if (!(measurement & Measurement::H2)) {
        data.GAS_DATA.H2 = _sensor->H2;
    }

    //Check if sensor is supposed to provide the H2 reading
    if (!(measurement & Measurement::ETHANOL)) {
        data.GAS_DATA.ETHANOL = _sensor->ethanol;
    }
}

