#ifdef ESP32

#include "MqttClient.h"

static void __mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                 int32_t event_id, void *event_data);

MqttClient::MqttClient(Stream &debugLog) : PrintLog(debugLog, "MqttClient") {}

MqttClient::~MqttClient() {}

bool MqttClient::begin(String uri) {
  if (isBegin) {
    logInfo("Already begin, calll 'end' and try again");
    return true;
  }
  if (uri.isEmpty()) {
    Serial.println("Mqtt uri is empty");
    return false;
  }

  this->uri = uri;
  logInfo("Init uri: " + uri);

  /** config esp_mqtt client */
  esp_mqtt_client_config_t config = {
      .uri = this->uri.c_str(),
  };

  /** init client */
  client = esp_mqtt_client_init(&config);
  if (client == NULL) {
    logError("Init client failed");
    return false;
  }

  /** Register event */
  if (esp_mqtt_client_register_event(client, MQTT_EVENT_ANY,
                                     __mqtt_event_handler, this) != ESP_OK) {
    logError("Register event failed");
    return false;
  }

  if (esp_mqtt_client_start(client) != ESP_OK) {
    logError("Client start failed");
    return false;
  }

  isBegin = true;
  connectionFailedCount = 0;
  return true;
}

void MqttClient::end(void) {
  if (!isBegin) {
    logWarning("Already end, call 'begin' and try again");
    return;
  }

  esp_mqtt_client_disconnect(client);
  esp_mqtt_client_stop(client);
  esp_mqtt_client_destroy(client);
  client = NULL;
  isBegin = false;

  logInfo("end");
}

void MqttClient::_updateConnected(bool connected) {
  this->connected = connected;
  if (connected) {
    connectionFailedCount = 0;
  } else {
    connectionFailedCount++;
    logWarning("Connection failed count " + String(connectionFailedCount));
  }
}

bool MqttClient::publish(const char *topic, const char *payload, int len) {
  if (!isBegin) {
    logError("No-initialized");
    return false;
  }
  if (!connected) {
    logError("Client disconnected");
    return false;
  }

  if (esp_mqtt_client_publish(client, topic, payload, len, 0, 0) == ESP_OK) {
    logInfo("Publish success");
    return true;
  }
  logError("Publish failed");
  return false;
}

/**
 * @brief Check that URI is same as current initialized  URI
 *
 * @param uri Target URI
 * @return true Same
 * @return false Difference
 */
bool MqttClient::isCurrentUri(String &uri) {
  if (this->uri == uri) {
    return true;
  }
  return false;
}

/**
 * @brief Get MQTT client connected status
 *
 * @return true Connected
 * @return false Disconnected
 */
bool MqttClient::isConnected(void) { return connected; }

/**
 * @brief Get number of connection failed
 *
 * @return int
 */
int MqttClient::getConnectionFailedCount(void) { return connectionFailedCount; }

static void __mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                 int32_t event_id, void *event_data) {
  MqttClient *mqtt = (MqttClient *)handler_args;

  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  esp_mqtt_client_handle_t client = event->client;

  int msg_id;
  switch ((esp_mqtt_event_id_t)event_id) {
  case MQTT_EVENT_CONNECTED:
    mqtt->logInfo("MQTT_EVENT_CONNECTED");
    mqtt->_updateConnected(true);
    break;
  case MQTT_EVENT_DISCONNECTED:
    mqtt->logInfo("MQTT_EVENT_DISCONNECTED");
    mqtt->_updateConnected(false);
    break;
  case MQTT_EVENT_SUBSCRIBED:
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    break;
  case MQTT_EVENT_PUBLISHED:
    break;
  case MQTT_EVENT_DATA:
    break;
  case MQTT_EVENT_ERROR:
    Serial.println("MQTT_EVENT_ERROR");
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
      mqtt->logError("Reported from esp-tls: " +
                     String(event->error_handle->esp_tls_last_esp_err));
      mqtt->logError("Reported from tls stack: " +
                     String(event->error_handle->esp_tls_stack_err));
      mqtt->logError("Captured as transport's socket errno: " +
                     String(event->error_handle->esp_transport_sock_errno));
    }
    break;
  default:
    Serial.printf("Other event id:%d\r\n", event->event_id);
    break;
  }
}

#endif /** ESP32 */
