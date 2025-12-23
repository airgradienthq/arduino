#include "AgSatellites.h"

AgSatellites::AgSatellites(Measurements &measurement, Configuration &config)
    : _measurements(measurement), _config(config), _pScan(nullptr),
      _initialized(false), _scanCallbacks(nullptr) {
  // Initialize satellite array
  for (int i = 0; i < MAX_SATELLITES; i++) {
    _satellites[i].id = "";
    _satellites[i].data.temp = -1000.0f;
    _satellites[i].data.rhum = -1.0f;
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
    _pScan->setScanCallbacks(_scanCallbacks, false);

    // Configure scan parameters
    _pScan->setInterval(100);  // Scan interval in ms
    _pScan->setWindow(99);     // Scan window in ms
    _pScan->setActiveScan(true); // Active scan for scan response data

    // Start continuous scanning (0 = scan forever)
    _pScan->start(0, false);

    _initialized = true;
  }

  // Check if scan is still running, restart if needed
  if (_pScan && !_pScan->isScanning()) {
    _pScan->start(0, false);
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

  // Check if this device is in our satellite list
  if (!isSatelliteInList(macAddress)) {
    return;
  }

  // TODO: Parse advertising data for temperature and humidity
  // For now, just store the MAC address to verify scanning works

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
    // Only set ID if it's a new entry
    if (isNewEntry) {
      _satellites[index].id = macAddress;
    }

    // TODO: Extract temperature and humidity from advertising data
    // This depends on the format your satellite devices use
    // Example placeholder values:
    // _satellites[index].data.temp = extractedTemp;
    // _satellites[index].data.rhum = extractedHumidity;

    // Reset use count when new data is received
    _satellites[index].data.useCount = 0;
  }
}

AgSatellites::Satellite* AgSatellites::getSatellites() {
  return _satellites;
}
