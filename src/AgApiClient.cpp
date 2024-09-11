#include "AgApiClient.h"
#include "AgConfigure.h"
#include "AirGradient.h"
#include "Libraries/Arduino_JSON/src/Arduino_JSON.h"

AgApiClient::AgApiClient(Stream &debug, Configuration &config)
    : PrintLog(debug, "ApiClient"), config(config) {}

AgApiClient::~AgApiClient() {}

/**
 * @brief Initialize the API client
 *
 */
void AgApiClient::begin(void) {
  getConfigFailed = false;
  postToServerFailed = false;
  logInfo("Init apiRoot: " + apiRoot);
  logInfo("begin");
}

/**
 * @brief Get configuration from AirGradient cloud
 *
 * @param deviceId Device ID
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::fetchServerConfiguration(void) {
  if (config.getConfigurationControl() ==
          ConfigurationControl::ConfigurationControlLocal ||
      config.isOfflineMode()) {
    logWarning("Ignore fetch server configuration");

    // Clear server configuration failed flag, cause it's ignore but not
    // really failed
    getConfigFailed = false;
    return false;
  }

  String uri = apiRoot + "/sensors/airgradient:" +
               ag->deviceId() + "/one/config";

  /** Init http client */
#ifdef ESP8266
  HTTPClient client;
  WiFiClient wifiClient;
  if (client.begin(wifiClient, uri) == false) {
    getConfigFailed = true;
    return false;
  }
#else
  HTTPClient client;
  _setHttpClientTimeout(&client);
  if (client.begin(uri) == false) {
    getConfigFailed = true;
    return false;
  }
#endif

  /** Get data */
  int retCode = client.GET();

  logInfo(String("GET: ") + uri);
  logInfo(String("Return code: ") + String(retCode));

  if (retCode != 200) {
    client.end();
    getConfigFailed = true;

    /** Return code 400 mean device not setup on cloud. */
    if (retCode == 400) {
      notAvailableOnDashboard = true;
    }
    return false;
  }

  /** clear failed */
  getConfigFailed = false;
  notAvailableOnDashboard = false;

  /** Get response string */
  String respContent = client.getString();
  client.end();

  // logInfo("Get configuration: " + respContent);

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
bool AgApiClient::postToServer(String data) {
  if (config.isPostDataToAirGradient() == false) {
    logWarning("Ignore post data to server");
    return true;
  }

  if (WiFi.isConnected() == false) {
    return false;
  }

  String uri = apiRoot + "/sensors/airgradient:" + ag->deviceId() + "/measures";
  // logInfo("Post uri: " + uri);
  // logInfo("Post data: " + data);

  WiFiClient wifiClient;
  HTTPClient client;
  _setHttpClientTimeout(&client);
  if (client.begin(wifiClient, uri.c_str()) == false) {
    logError("Init client failed");
    return false;
  }
  client.addHeader("content-type", "application/json");
  int retCode = client.POST(data);
  client.end();

  logInfo(String("POST: ") + uri);
  logInfo(String("DATA: ") + data);
  logInfo(String("Return code: ") + String(retCode));

  if ((retCode == 200) || (retCode == 429)) {
    postToServerFailed = false;
    return true;
  } else {
    logError("Post response failed code: " + String(retCode));
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

/**
 * @brief Get status device has available on dashboard or not. should get after
 * fetch configuration return failed
 *
 * @return true
 * @return false
 */
bool AgApiClient::isNotAvailableOnDashboard(void) {
  return notAvailableOnDashboard;
}

void AgApiClient::setAirGradient(AirGradient *ag) { this->ag = ag; }

/**
 * @brief Send the package to check the connection with cloud
 * 
 * @param rssi WiFi RSSI
 * @param bootCount Boot count
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::sendPing(int rssi, int bootCount) { 
  JSONVar root;
  root["wifi"] = rssi;
  root["boot"] = bootCount;
  return postToServer(JSON.stringify(root));
}

String AgApiClient::getApiRoot() const { return apiRoot; }

void AgApiClient::setApiRoot(const String &apiRoot) { this->apiRoot = apiRoot; }

/**
 * @brief Set http request timeout. (Default: 10s)
 *
 * @param timeoutMs
 */
void AgApiClient::setTimeout(uint16_t timeoutMs) {
  this->timeoutMs = timeoutMs;
}

/**
 * @brief Set timeout to both connect to server and tcp connection timeout
 *
 */
void AgApiClient::_setHttpClientTimeout(HTTPClient *httpClient) {
  httpClient->setTimeout(timeoutMs);
  httpClient->setConnectTimeout(timeoutMs);
}