## OTA Updates

From [firmware version 3.1.1](https://github.com/airgradienthq/arduino/tree/3.1.1) onwards, the AirGradient ONE and Open Air monitors support over the air (OTA) updates.

#### Mechanism

Upon compilation of an official release the git tag (GIT_VERSION) is compiled into the binary.

The device attempts to update to the latest version on startup and in regular intervals using URL

http://hw.airgradient.com/sensors/{deviceId}/generic/os/firmware.bin?current_firmware={GIT_VERSION}

If does pass the version it is currently running on along to the server through URL parameter 'current_firmware'.
This allows the server to identify if the device is already running on the latest version or should update.

The following scenarios are possible

1. The device is already on the latest firmware. Then the server returns a 304 with a short explanation text in the body saying this.
2. The device reports a firmware unknown to the server. A 400 with an empty payload is returned in this case and the update is not performed. This case is relevant for local changes. The GIT_VERSION then defaults to "snapshot" which is unknown to the server.
3. There is an update available. A 200 along with the binary data of the new version is returned and the update is performed.

More information about the implementation details are available here: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html
