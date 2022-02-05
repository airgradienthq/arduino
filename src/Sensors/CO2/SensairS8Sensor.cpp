#include "SensairS8Sensor.h"

AirGradient::Measurement AirGradient::SensairS8Sensor::getAvailableMeasurement() const {
    return Measurement::CO2;
}

bool AirGradient::SensairS8Sensor::begin() {
    _softwareSerial = std::make_unique<SoftwareSerial>(_rxPin, _txPin);
    _softwareSerial->begin(_baudRate);
    _sensor = std::make_unique<S8_UART>(*_softwareSerial);
    S8_sensor sensorData{};
    _sensor->get_firmware_version(sensorData.firm_version);
    Serial.println(">>> SenseAir S8 NDIR CO2 sensor <<<");
    Serial.printf("Firmware version: %s\n", sensorData.firm_version);
    sensorData.sensor_type_id = _sensor->get_sensor_type_ID();
    Serial.print("Sensor type: 0x");
    printIntToHex(sensorData.sensor_type_id, 3);
    Serial.println("");
    sensorData.abc_period = _sensor->get_ABC_period();
    Serial.printf("ABC period: %d\n", sensorData.abc_period);
    return sensorData.abc_period > 0;
}

void AirGradient::SensairS8Sensor::getData(AirGradient::SensorData &data) const {
    auto previousReading = data.GAS_DATA.CO2;
    auto co2 = _sensor->get_co2();

    uint8_t timeout = 0;

    while (co2 <= 0 || (previousReading != 0 && abs(co2 - previousReading) >= 500)) {
        delay(200);
        co2 = _sensor->get_co2();
        if (++timeout > 3) {
            Serial.println("Couldn't get proper CO2 reading");
            return;
        }
    }

    data.GAS_DATA.CO2 = co2;
}