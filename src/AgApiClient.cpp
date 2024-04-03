#include "AgApiClient.h"
#include <HTTPClient.h>

void AgApiClient::printLog(String log) {
  debugLog.printf("[AgApiClient] %s\r\n", log.c_str());
  // Serial.printf("[AgApiClient] %s\r\n", log.c_str());
}

AgApiClient::AgApiClient(Stream &debug, AgConfigure &config)
    : debugLog(debug), config(config) {}

AgApiClient::~AgApiClient() {}

/**
 * @brief Initialize the API client
 *
 */
void AgApiClient::begin(void) {
  getConfigFailed = false;
  postToServerFailed = false;
  printLog("Begin");
}

/**
 * @brief Get configuration from AirGradient cloud
 *
 * @param deviceId Device ID
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::fetchServerConfiguration(String deviceId) {
  if (config.getConfigurationControl() ==
      ConfigurationControl::ConfigurationControlLocal) {
    printLog("Ignore fetch server configuratoin");

    // Clear server configuration failed flag, cause it's ignore but not
    // really failed
    getConfigFailed = false;
    return false;
  }

  String uri = "http://hw.airgradient.com/sensors/airgradient:" + deviceId +
               "/one/config";

  /** Init http client */
  HTTPClient client;
  if (client.begin(uri) == false) {
    getConfigFailed = true;
    return false;
  }

  /** Get data */
  int retCode = client.GET();
  if (retCode != 200) {
    client.end();
    getConfigFailed = true;
    return false;
  }

  /** clear failed */
  getConfigFailed = false;

  /** Get response string */
  String respContent = client.getString();
  client.end();

  printLog("Server configuration: " + respContent);

  /** Parse configuration and return result */
  return config.parse(respContent, false);
}

/**
 * @brief Post data to AirGradient cloud
 *
 * @param deviceId Device Id
 * @param data String JSON
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::postToServer(String deviceId, String data) {
  if (config.isPostDataToAirGradient() == false) {
    printLog("Ignore post data to server");
    return true;
  }

  if (WiFi.isConnected() == false) {
    return false;
  }

  String uri =
      "http://hw.airgradient.com/sensors/airgradient:" + deviceId + "/measures";
  printLog("Post uri: " + uri);
  printLog("Post data: " + data);

  WiFiClient wifiClient;
  HTTPClient client;
  if (client.begin(wifiClient, uri.c_str()) == false) {
    return false;
  }
  client.addHeader("content-type", "application/json");
  int retCode = client.POST(data);
  client.end();

  if ((retCode == 200) || (retCode == 429)) {
    postToServerFailed = false;
    return true;
  } else {
    printLog("Post response failed code: " + String(retCode));
  }
  postToServerFailed = true;
  return false;
}

/**
 * @brief Get failed status when get configuration from AirGradient cloud
 *
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::isFetchConfigureFailed(void) { return getConfigFailed; }

/**
 * @brief Get failed status when post data to AirGradient cloud
 *
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::isPostToServerFailed(void) { return postToServerFailed; }
