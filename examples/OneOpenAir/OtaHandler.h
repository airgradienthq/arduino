#ifndef _OTA_HANDLER_H_
#define _OTA_HANDLER_H_

#include <esp_ota_ops.h>
#include <esp_http_client.h>
#include <esp_err.h>
#include <Arduino.h>

#define OTA_BUF_SIZE 512
#define URL_BUF_SIZE 256


class OtaHandler {
public:
    void updateFirmwareIfOutdated(String deviceId) {

        String url = "http://hw.airgradient.com/sensors/airgradient:" 
        + deviceId + "/generic/os/firmware.bin";
        url += "?current_firmware=";
        url += GIT_VERSION;
        char urlAsChar[URL_BUF_SIZE];
        url.toCharArray(urlAsChar, URL_BUF_SIZE);
        Serial.printf("checking for new ota @ %s\n", urlAsChar);

        esp_http_client_config_t config = {};
        config.url = urlAsChar;
        esp_err_t ret = attemptToPerformOta(&config);
        Serial.println(ret);
        if (ret == 0) {
            Serial.println("OTA completed");
            esp_restart();
        } else {
            Serial.println("OTA failed, maybe already up to date");
        }   
    }

private:

    int attemptToPerformOta(const esp_http_client_config_t *config) {
        esp_http_client_handle_t client = esp_http_client_init(config);
        if (client == NULL) {
            Serial.println("Failed to initialize HTTP connection");
            return -1;
        }

        esp_err_t err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            esp_http_client_cleanup(client);
            Serial.printf("Failed to open HTTP connection: %s\n", esp_err_to_name(err));
            return -1;
        }
        esp_http_client_fetch_headers(client);

        esp_ota_handle_t update_handle = 0;
        const esp_partition_t *update_partition = NULL;
        Serial.println("Starting OTA ...");
        update_partition = esp_ota_get_next_update_partition(NULL);
        if (update_partition == NULL) {
            Serial.println("Passive OTA partition not found");
            cleanupHttp(client);
            return ESP_FAIL;
        }
        Serial.printf("Writing to partition subtype %d at offset 0x%x\n",
                update_partition->subtype, update_partition->address);

        err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
        if (err != ESP_OK) {
            Serial.printf("esp_ota_begin failed, error=%d\n", err);
            cleanupHttp(client);
            return err;
        }
  
        esp_err_t ota_write_err = ESP_OK;
        char *upgrade_data_buf = (char *)malloc(OTA_BUF_SIZE);
        if (!upgrade_data_buf) {
            Serial.println("Couldn't allocate memory for data buffer");
            return ESP_ERR_NO_MEM;
        }

        int binary_file_len = 0;
        while (1) {
            int data_read = esp_http_client_read(client, upgrade_data_buf, OTA_BUF_SIZE);
            if (data_read == 0) {
                Serial.println("Connection closed, all data received");
                break;
            }
            if (data_read < 0) {
                Serial.println("Data read error");
                break;
            }
            if (data_read > 0) {
                ota_write_err = esp_ota_write( update_handle, (const void *)upgrade_data_buf, data_read);
                if (ota_write_err != ESP_OK) {
                    break;
                }
                binary_file_len += data_read;
                // Serial.printf("Written image length %d\n", binary_file_len);
            }
        }
        free(upgrade_data_buf);
        cleanupHttp(client); 
        Serial.printf("# of bytes written: %d\n", binary_file_len);
        
        esp_err_t ota_end_err = esp_ota_end(update_handle);
        if (ota_write_err != ESP_OK) {
            Serial.printf("Error: esp_ota_write failed! err=0x%d\n", err);
            return ota_write_err;
        } else if (ota_end_err != ESP_OK) {
            Serial.printf("Error: esp_ota_end failed! err=0x%d. Image is invalid", ota_end_err);
            return ota_end_err;
        }

        err = esp_ota_set_boot_partition(update_partition);
        if (err != ESP_OK) {
            Serial.printf("esp_ota_set_boot_partition failed! err=0x%d\n", err);
            return err;
        }
        return 0;
    }

    void cleanupHttp(esp_http_client_handle_t client) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
    }
    
};

#endif
