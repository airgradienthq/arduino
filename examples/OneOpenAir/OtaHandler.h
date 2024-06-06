#ifndef _OTA_HANDLER_H_
#define _OTA_HANDLER_H_
#include <Arduino.h>
#include <esp_err.h>
#include <esp_http_client.h>
#include <esp_ota_ops.h>

#define OTA_BUF_SIZE 1024
#define URL_BUF_SIZE 256

enum OtaUpdateOutcome {
  UPDATE_PERFORMED,
  ALREADY_UP_TO_DATE,
  UPDATE_FAILED,
  UDPATE_SKIPPED
};

enum OtaState {
  OTA_STATE_BEGIN,
  OTA_STATE_FAIL,
  OTA_STATE_SKIP,
  OTA_STATE_UP_TO_DATE,
  OTA_STATE_PROCESSING,
  OTA_STATE_SUCCESS
};

typedef void(*OtaHandlerCallback_t)(OtaState state,
                                   String message);

class OtaHandler {
public:
  void updateFirmwareIfOutdated(String deviceId) {
    String url = "http://hw.airgradient.com/sensors/airgradient:" + deviceId +
                 "/generic/os/firmware.bin";
    url += "?current_firmware=";
    url += GIT_VERSION;
    char urlAsChar[URL_BUF_SIZE];
    url.toCharArray(urlAsChar, URL_BUF_SIZE);
    Serial.printf("checking for new OTA update @ %s\n", urlAsChar);

    esp_http_client_config_t config = {};
    config.url = urlAsChar;
    OtaUpdateOutcome ret = attemptToPerformOta(&config);
    Serial.println(ret);
    if (this->callback) {
      switch (ret) {
      case OtaUpdateOutcome::UPDATE_PERFORMED:
        this->callback(OtaState::OTA_STATE_SUCCESS, "");
        break;
      case OtaUpdateOutcome::UDPATE_SKIPPED:
        this->callback(OtaState::OTA_STATE_SKIP, "");
        break;
      case OtaUpdateOutcome::ALREADY_UP_TO_DATE:
        this->callback(OtaState::OTA_STATE_UP_TO_DATE, "");
        break;
      case OtaUpdateOutcome::UPDATE_FAILED:
        this->callback(OtaState::OTA_STATE_FAIL, "");
        break;
      default:
        break;
      }
    }
  }

  void setHandlerCallback(OtaHandlerCallback_t callback) {
    this->callback = callback;
  }

private:
  OtaHandlerCallback_t callback;

  OtaUpdateOutcome attemptToPerformOta(const esp_http_client_config_t *config) {
    esp_http_client_handle_t client = esp_http_client_init(config);
    if (client == NULL) {
      Serial.println("Failed to initialize HTTP connection");
      return OtaUpdateOutcome::UPDATE_FAILED;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
      esp_http_client_cleanup(client);
      Serial.printf("Failed to open HTTP connection: %s\n",
                    esp_err_to_name(err));
      return OtaUpdateOutcome::UPDATE_FAILED;
    }
    esp_http_client_fetch_headers(client);

    int httpStatusCode = esp_http_client_get_status_code(client);
    if (httpStatusCode == 304) {
      Serial.println("Firmware is already up to date");
      cleanupHttp(client);
      return OtaUpdateOutcome::ALREADY_UP_TO_DATE;
    } else if (httpStatusCode != 200) {
      Serial.printf("Firmware update skipped, the server returned %d\n",
                    httpStatusCode);
      cleanupHttp(client);
      return OtaUpdateOutcome::UDPATE_SKIPPED;
    }

    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;
    Serial.println("Starting OTA update ...");
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
      Serial.println("Passive OTA partition not found");
      cleanupHttp(client);
      return OtaUpdateOutcome::UPDATE_FAILED;
    }
    Serial.printf("Writing to partition subtype %d at offset 0x%x\n",
                  update_partition->subtype, update_partition->address);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
      Serial.printf("esp_ota_begin failed, error=%d\n", err);
      cleanupHttp(client);
      return OtaUpdateOutcome::UPDATE_FAILED;
    }

    esp_err_t ota_write_err = ESP_OK;
    char *upgrade_data_buf = (char *)malloc(OTA_BUF_SIZE);
    if (!upgrade_data_buf) {
      Serial.println("Couldn't allocate memory for data buffer");
      return OtaUpdateOutcome::UPDATE_FAILED;
    }

    int binary_file_len = 0;
    int totalSize = esp_http_client_get_content_length(client);
    Serial.println("File size: " + String(totalSize) + String(" bytes"));

    // Show display start update new firmware.
    if (this->callback) {
      this->callback(OtaState::OTA_STATE_BEGIN, "");
    }

    // Download file and write new firmware to OTA partition
    uint32_t lastUpdate = millis();
    while (1) {
      int data_read =
          esp_http_client_read(client, upgrade_data_buf, OTA_BUF_SIZE);
      if (data_read == 0) {
        if (this->callback) {
          this->callback(OtaState::OTA_STATE_PROCESSING, String(100));
        }
        Serial.println("Connection closed, all data received");
        break;
      }
      if (data_read < 0) {
        Serial.println("Data read error");
        if (this->callback) {
          this->callback(OtaState::OTA_STATE_FAIL, "");
        }
        break;
      }
      if (data_read > 0) {
        ota_write_err = esp_ota_write(
            update_handle, (const void *)upgrade_data_buf, data_read);
        if (ota_write_err != ESP_OK) {
          if (this->callback) {
            this->callback(OtaState::OTA_STATE_FAIL, "");
          }
          break;
        }
        binary_file_len += data_read;

        int percent = (binary_file_len * 100) / totalSize;
        uint32_t ms = (uint32_t)(millis() - lastUpdate);
        if (ms >= 250) {
          // sm.executeOTA(StateMachine::OtaState::OTA_STATE_PROCESSING, "",
          //               percent);
          if (this->callback) {
            this->callback(OtaState::OTA_STATE_PROCESSING,
                           String(percent));
          }
          lastUpdate = millis();
        }
      }
    }
    free(upgrade_data_buf);
    cleanupHttp(client);
    Serial.printf("# of bytes written: %d\n", binary_file_len);

    esp_err_t ota_end_err = esp_ota_end(update_handle);
    if (ota_write_err != ESP_OK) {
      Serial.printf("Error: esp_ota_write failed! err=0x%d\n", err);
      return OtaUpdateOutcome::UPDATE_FAILED;
    } else if (ota_end_err != ESP_OK) {
      Serial.printf("Error: esp_ota_end failed! err=0x%d. Image is invalid",
                    ota_end_err);
      return OtaUpdateOutcome::UPDATE_FAILED;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
      Serial.printf("esp_ota_set_boot_partition failed! err=0x%d\n", err);
      return OtaUpdateOutcome::UPDATE_FAILED;
    }
    return OtaUpdateOutcome::UPDATE_PERFORMED;
  }

  void cleanupHttp(esp_http_client_handle_t client) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
  }
};

#endif
