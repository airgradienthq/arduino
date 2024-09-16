#include "AgApiClient.h"
#include "AgConfigure.h"
#include "AirGradient.h"
#include "Libraries/Arduino_JSON/src/Arduino_JSON.h"
#ifdef ESP8266
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#else
#include <HTTPClient.h>
#endif

AgApiClient::AgApiClient(Stream &debug, Configuration &config)
    : PrintLog(debug, "ApiClient"), config(config) {}

AgApiClient::~AgApiClient() {}

/**
 * @brief Initialize the API client
 *
 */
void AgApiClient::begin(void) {
  postResult.value = 0x00;
  getResult.value = 0x00;

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
    getResult.value = 0;
    return false;
  }

  String uri = apiRoot + "/sensors/airgradient:" +
               ag->deviceId() + "/one/config";

  int retCode = 200;
  bool result = true;
  /** Init http client */
  HTTPClient client;
#ifdef ESP8266
  WiFiClient wifiClient;
  if (client.begin(wifiClient, uri) == false) {
#else
  if (client.begin(uri) == false) {
#endif
    result = false;
  }

  /** Get data */
  if (result) {
    retCode = client.GET();
    logInfo(String("GET: ") + uri);
    logInfo(String("Return code: ") + String(retCode));

    if (retCode != 200) {
      client.end();
      result = false;
    }
  }

  if (result) {
    /** clear failed */
    notAvailableOnDashboard = false;
    getResult.value = 0x00;
  } else {
    /** Set retry flag */
    getResult.bits.retry = 0b1;

    /** Return code 400 mean device not setup on cloud. */
    if (retCode == 400) {
      notAvailableOnDashboard = true;
      getResult.bits.failed = 0b1;
      getResult.value = 0x00;
    } else {
      if (getResult.bits.count < postGetRetryCountMax) {
        getResult.bits.count = getResult.bits.count + 1;
      }
    }

    /** Set failed flag */
    if (getResult.bits.count >= postGetRetryCountMax) {
      getResult.bits.failed = 0b1;
    }
    if (getResult.bits.count > postGetRetryCountMax) {
      getResult.bits.retry = 0b0;
    }

    return false;
  }

  /** Get response string */
  String respContent = client.getString();
  client.end();

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

  String uri =
      "http://hw.airgradient.com/sensors/airgradient:" + ag->deviceId() +
      "/measures";

  /** Reset failed */
  postResult.bits.failed = 0b0;

  WiFiClient wifiClient;
  HTTPClient client;
  bool result = true;  /** Process result */
  int retCode = 200;   /** POST return code */
  if (client.begin(wifiClient, uri.c_str()) == false) {
    logError("Init client failed");
    result = false;
  }

  if (result) {
    /** Send data */
    client.addHeader("content-type", "application/json");
    retCode = client.POST(data);
    client.end();

    logInfo(String("POST: ") + uri);
    logInfo(String("DATA: ") + data);
    logInfo(String("Return code: ") + String(retCode));

    if ((retCode == 200) || (retCode == 429)) {
      postResult.value = 0;   /** Clear failed and retry flag. */
      return true;
    } else {
      logError("Post response failed code: " + String(retCode));
      result = false;
    }
  }

  if (result) {
    /** clear failed */
    postResult.value = 0x00;
  } else {
    /** Set retry flag */
    postResult.bits.retry = 0b1;

    if (retCode == 400) {
      /** Ignore retry if return code 400 */
      postResult.value = 0;
      postResult.bits.failed = 0b1;
    } else {
      /** Update retry count */
      if (postResult.bits.count < postGetRetryCountMax) {
        postResult.bits.count = postResult.bits.count + 1;
      }
    }

    /** Set failed flag */
    if (postResult.bits.count >= postGetRetryCountMax) {
      postResult.bits.failed = 0b1;
    }
    if (postResult.bits.count > postGetRetryCountMax) {
      postResult.bits.retry = 0b0;
    }
  }

  return result;
}

/**
 * @brief Get failed status when get configuration from AirGradient cloud
 *
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::isFetchConfigureFailed(void) {
  return (getResult.bits.failed == 0b1);
}

/**
 * @brief Get failed status when post data to AirGradient cloud
 *
 * @return true Success
 * @return false Failure
 */
bool AgApiClient::isPostToServerFailed(void) {
  return (postResult.bits.failed == 0b1);
}

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

  bool result = postToServer(JSON.stringify(root));

  /** Clear retry */
  postResult.bits.count = 0;
  postResult.bits.retry = 0;

  return result;
}

String AgApiClient::getApiRoot() const { return apiRoot; }

void AgApiClient::setApiRoot(const String &apiRoot) { this->apiRoot = apiRoot; }

/**
 * @brief Is retry post to server. It's valid if postToServer return false
 * 
 * @return true 
 * @return false 
 */
bool AgApiClient::postToServerRetry(void) {
  if (postResult.bits.retry) {
    logWarning("postToServer retry " + String(postResult.bits.count));
    return true;
  }
  return false;
}

/**
 * @brief Is retry fetch the configuration. It's value if fetchConfigure return
 * false
 *
 * @return true
 * @return false
 */
bool AgApiClient::fetchConfigureRetry(void) {
  if (getResult.bits.retry) {
    logWarning("fetchConfiguration retry " + String(postResult.bits.count));
    return true;
  }
  return false;
}

/**
 * @brief Get POST/GET retry period milliseconds
 * 
 * @return uint16_t 
 */
uint16_t AgApiClient::getRetryPeriod(void) { return retryPeriodMs; }
