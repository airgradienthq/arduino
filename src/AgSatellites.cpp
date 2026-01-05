#ifdef ESP32

#include "AgSatellites.h"

AgSatellites::AgSatellites(Measurements &measurement, Configuration &config)
    : _measurements(measurement), _config(config), _pScan(nullptr), _initialized(false),
      _scanCallbacks(nullptr) {
  // Initialize satellite array
  for (int i = 0; i < MAX_SATELLITES; i++) {
    _satellites[i].id = "";
    _satellites[i].data.temp = -1000.0f;
    _satellites[i].data.rhum = -1.0f;
    _satellites[i].data.rssi = -1000;
    _satellites[i].data.useCount = 0;
  }
}

AgSatellites::~AgSatellites() {
  // Stop scan if running
  if (_pScan && _initialized) {
    _pScan->stop();
  }

  // Cleanup callbacks
  if (_scanCallbacks) {
    delete _scanCallbacks;
    _scanCallbacks = nullptr;
  }
}

bool AgSatellites::run() {
  // Check if satellites are enabled in config
  if (!_config.isSatellitesEnabled()) {
    return false;
  }

  // Lazy initialization on first run
  if (!_initialized) {
    // Initialize NimBLE
    NimBLEDevice::init("AgSatelliteScanner");

    // Get scan instance
    _pScan = NimBLEDevice::getScan();
    if (!_pScan) {
      return false;
    }

    // Create and set scan callbacks
    _scanCallbacks = new ScanCallbacks(this);
    _pScan->setScanCallbacks(_scanCallbacks, true);

    // Do not store scan results, use callbacks only. Prevents memory leak.
    _pScan->setMaxResults(0);

    // Configure scan parameters
    // TODO: Might need to adjust for reliablity coex wifi
    _pScan->setInterval(100);    // Scan interval in ms
    _pScan->setWindow(99);       // Scan window in ms
    _pScan->setActiveScan(true); // Active scan for scan response data

    // Start scanning for 30 seconds
    _pScan->start(30, false);

    _initialized = true;
  }

  // Check if scan is still running, restart if needed
  if (_pScan && !_pScan->isScanning()) {
    _pScan->start(30, false);
  }

  return true;
}

bool AgSatellites::isSatelliteInList(String macAddress) {
  const String *satellites = _config.getSatellites();
  if (!satellites) {
    return false;
  }

  // Check if MAC address is in the configured satellite list
  for (int i = 0; i < MAX_SATELLITES; i++) {
    if (satellites[i].length() > 0 && satellites[i].equalsIgnoreCase(macAddress)) {
      return true;
    }
  }

  return false;
}

void AgSatellites::processAdvertisedDevice(const NimBLEAdvertisedDevice *device) {
  if (!device) {
    return;
  }

  // Get MAC address
  String macAddress = device->getAddress().toString().c_str();
  macAddress.replace(":", "");

  // Check if this device is in our satellite list

  if (!isSatelliteInList(macAddress)) {
    //Serial.printf("Data from satellite %s discarded\n", macAddress.c_str());
    return;
  }

  int rssi = device->getRSSI();
  Serial.printf("Got data from satellite %s (rssi: %d)\n", macAddress.c_str(), rssi);

  // Get advertising payload
  const std::vector<uint8_t> &payload = device->getPayload();
  if (payload.empty()) {
    Serial.printf("Empty payload from satellite %s\n", macAddress.c_str());
    return;
  }

  // Find or create entry in satellites array
  int index = -1;
  bool isNewEntry = false;

  for (int i = 0; i < MAX_SATELLITES; i++) {
    if (_satellites[i].id == macAddress) {
      index = i;
      break;
    } else if (_satellites[i].id.length() == 0 && index == -1) {
      // Found empty slot
      index = i;
      isNewEntry = true;
    }
  }

  if (index >= 0) {
    // TODO: What happen if old satellites is removed?
    // Only set ID if it's a new entry
    if (isNewEntry) {
      _satellites[index].id = macAddress;
    }

    // Parse BTHome advertising data
    SatelliteData newData;
    if (decodeBTHome(payload.data(), payload.size(), newData)) {
      // Successfully parsed - reset use count when new data is received
      _satellites[index].data.temp = newData.temp;
      _satellites[index].data.rhum = newData.rhum;
      _satellites[index].data.rssi = rssi;
      _satellites[index].data.useCount = 0;
      Serial.printf("Satellite %s reported temp %.1f, hum %.1f, rssi %d\n", macAddress.c_str(),
                    _satellites[index].data.temp, _satellites[index].data.rhum,
                    _satellites[index].data.rssi);
    } else {
      Serial.printf("Failed to parse data from satellite %s\n", macAddress.c_str());
    }
  }
}

bool AgSatellites::decodeBTHome(const uint8_t *payload, size_t size, SatelliteData &data) {
  // Initialize with invalid values
  data.temp = -1000.0f;
  data.rhum = -1.0f;

  // Walk through BLE AD structures: [len][type][data...]
  for (size_t i = 0; i + 1 < size;) {
    uint8_t len = payload[i];
    if (len == 0) {
      break; // No more AD structures
    }

    // Check if we have enough data
    if (i + 1 + len > size) {
      break; // Malformed
    }

    uint8_t type = payload[i + 1];

    // Service Data - 16-bit UUID (type 0x16)
    if (type == 0x16 && len >= 3) {
      // UUID is at i+2 (little-endian)
      uint16_t uuid = payload[i + 2] | (payload[i + 3] << 8);

      // Check for BTHome UUID (0xFCD2)
      if (uuid == 0xFCD2) {
        // BTHome payload begins after UUID
        size_t p = i + 4;
        size_t end = i + 1 + len; // End of this AD structure

        if (p >= end) {
          return false;
        }

        uint8_t device_info = payload[p++];

        // Check if encrypted
        bool encrypted = (device_info & 0x01);
        if (encrypted) {
          // Not handling encryption
          return false;
        }

        bool found_data = false;

        // Parse BTHome v2 objects
        while (p < end) {
          uint8_t obj_id = payload[p++];

          switch (obj_id) {
          case 0x02: // Temperature (sint16, ×0.01 °C)
            if (p + 2 > end)
              return false;
            {
              int16_t raw = (int16_t)(payload[p] | (payload[p + 1] << 8));
              data.temp = raw * 0.01f;
              p += 2;
              found_data = true;
            }
            break;

          case 0x03: // Humidity (uint16, ×0.01 %)
            if (p + 2 > end)
              return false;
            {
              uint16_t raw = (uint16_t)(payload[p] | (payload[p + 1] << 8));
              data.rhum = raw * 0.01f;
              p += 2;
              found_data = true;
            }
            break;

          case 0x00: // Packet ID (uint8) - skip
            if (p + 1 > end)
              return false;
            p += 1;
            break;

          case 0x01: // Battery (uint8) - skip
            if (p + 1 > end)
              return false;
            p += 1;
            break;

          case 0x0C: // Voltage (uint16) - skip
            if (p + 2 > end)
              return false;
            p += 2;
            break;

          default:
            // Unknown object ID - stop parsing but return what we have
            return found_data;
          }
        }

        return found_data;
      }
    }

    // Go to next AD structure
    i += 1 + len;
  }

  return false; // No BTHome data found
}

AgSatellites::Satellite *AgSatellites::getSatellites() { return _satellites; }

#endif // ESP32
