#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H
#ifndef ESP8266 // Only for esp32 based mcu

#include <Arduino.h>
#include <esp_err.h>
#include <esp_http_client.h>
#include <esp_ota_ops.h>

#define OTA_BUF_SIZE 1024
#define URL_BUF_SIZE 256

class OtaHandler {
public:
  enum OtaState {
    OTA_STATE_BEGIN,
    OTA_STATE_FAIL,
    OTA_STATE_SKIP,
    OTA_STATE_UP_TO_DATE,
    OTA_STATE_PROCESSING,
    OTA_STATE_SUCCESS
  };

  typedef void (*OtaHandlerCallback_t)(OtaState state, String message);
  void setHandlerCallback(OtaHandlerCallback_t callback);
  void updateFirmwareIfOutdated(String deviceId);

private:
  OtaHandlerCallback_t _callback;

  enum OtaUpdateOutcome {
    UPDATE_PERFORMED = 0,
    ALREADY_UP_TO_DATE,
    UPDATE_FAILED,
    UPDATE_SKIPPED
  }; // Internal use

  OtaUpdateOutcome attemptToPerformOta(const esp_http_client_config_t *config);
  void cleanupHttp(esp_http_client_handle_t client);
};

#endif // ESP8266
#endif //  OTA_HANDLER_H