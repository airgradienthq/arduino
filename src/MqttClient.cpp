#include "MqttClient.h"

static void __mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data);

MqttClient::MqttClient(Stream &debugLog) : debugLog(debugLog) {}

MqttClient::~MqttClient() {}

bool MqttClient::begin(String uri) {
  if (isBegin) {
    _printLog("Already begin, calll 'end' and try again");
    return true;
  }
  if (uri.isEmpty()) {
    Serial.println("Mqtt uri is empty");
    return false;
  }

  this->uri = uri;
  _printLog("Init uri: " + uri);

  /** config esp_mqtt client */
  esp_mqtt_client_config_t config = {
      .uri = this->uri.c_str(),
  };

  /** init client */
  client = esp_mqtt_client_init(&config);
  if (client == NULL) {
    _printLog("Init client failed");
    return false;
  }

  /** Register event */
  if (esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, __mqtt_event_handler,
                                     this) != ESP_OK) {
    _printLog("Register event failed");
    return false;
  }

  if (esp_mqtt_client_start(client) != ESP_OK) {
    _printLog("Client start failed");
    return false;
  }

  isBegin = true;
  connectionFailedCount = 0;
  return true;
}

void MqttClient::end(void) {
  if (!isBegin) {
    _printLog("Already end, call 'begin' and try again");
    return;
  }

  esp_mqtt_client_disconnect(client);
  esp_mqtt_client_stop(client);
  esp_mqtt_client_destroy(client);
  client = NULL;
  isBegin = false;

  Serial.println("De-init");
}

void MqttClient::_printLog(String log) {
  debugLog.println("[MqttClient]" + log);
}

void MqttClient::_updateConnected(bool connected) {
  this->connected = connected;
  if (connected) {
    connectionFailedCount = 0;
  } else {
    connectionFailedCount++;
    _printLog("Connection failed count " + String(connectionFailedCount));
  }
}

bool MqttClient::publish(String &topic, String &payload) {
  if (!isBegin) {
    _printLog("Error: No-initialized");
    return false;
  }
  if (!connected) {
    _printLog("Error: Client disconnected");
    return false;
  }

  if (esp_mqtt_client_publish(client, topic.c_str(), payload.c_str(),
                              payload.length(), 0, 0) == ESP_OK) {
    _printLog("Publish topic: " + topic);
    _printLog("Publish payload: " + payload);
    return true;
  }
  _printLog("Error: publish");
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
    mqtt->_printLog("MQTT_EVENT_CONNECTED");
    mqtt->_updateConnected(true);
    break;
  case MQTT_EVENT_DISCONNECTED:
    mqtt->_printLog("MQTT_EVENT_DISCONNECTED");
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
      mqtt->_printLog("Reported from esp-tls: " +
                      String(event->error_handle->esp_tls_last_esp_err));
      mqtt->_printLog("Reported from tls stack: " +
                      String(event->error_handle->esp_tls_stack_err));
      mqtt->_printLog("Captured as transport's socket errno: " +
                      String(event->error_handle->esp_transport_sock_errno));
    }
    break;
  default:
    Serial.printf("Other event id:%d\r\n", event->event_id);
    break;
  }
}
