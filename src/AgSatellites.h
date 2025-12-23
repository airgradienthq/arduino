#ifndef _AG_SATELLITES_H_
#define _AG_SATELLITES_H_

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "AgConfigure.h"
#include "AgValue.h"

class AgSatellites {
public:
  struct SatelliteData {
    float temp;
    float rhum;
    uint8_t useCount;
  };

  struct Satellite {
    String id;
    SatelliteData data;
  };

private:
  Satellite _satellites[MAX_SATELLITES];
  Measurements &_measurements;
  Configuration &_config;
  NimBLEScan *_pScan;
  bool _initialized;

  // NimBLE scan callback class
  class ScanCallbacks : public NimBLEScanCallbacks {
  private:
    AgSatellites *_parent;

  public:
    ScanCallbacks(AgSatellites *parent) : _parent(parent) {}

    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override {
      if (_parent) {
        _parent->processAdvertisedDevice(advertisedDevice);
      }
    }
  };

  ScanCallbacks *_scanCallbacks;

  // Helper methods
  bool isSatelliteInList(String macAddress);
  void processAdvertisedDevice(const NimBLEAdvertisedDevice *device);
  bool decodeBTHome(const uint8_t* payload, size_t size, SatelliteData &data);

public:
  AgSatellites(Measurements &measurement, Configuration &config);
  ~AgSatellites();

  bool run();
  Satellite* getSatellites();
};

#endif // _AG_SATELLITES_H_
