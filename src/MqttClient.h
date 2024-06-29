#ifndef _AG_MQTT_CLIENT_H_
#define _AG_MQTT_CLIENT_H_

#ifdef ESP32
#include "mqtt_client.h"
#else
#include <WiFiClient.h>
#endif /** ESP32 */
#include "Main/PrintLog.h"
#include <Arduino.h>

class MqttClient: public PrintLog {
private:
  bool isBegin = false;
  String uri;
#ifdef ESP32
  esp_mqtt_client_handle_t client;
#else
  WiFiClient __wifiClient;
  void* client;
  String password;
  String user;
  String server;
  uint16_t port;
#endif
  bool connected = false;
  int connectionFailedCount = 0;

public:
  MqttClient(Stream &debugLog);
  ~MqttClient();

  bool begin(String uri);
  void end(void);
  void _updateConnected(bool connected);
  bool publish(const char* topic, const char* payload, int len);
  bool isCurrentUri(String &uri);
  bool isConnected(void);
  int getConnectionFailedCount(void);
#ifdef ESP8266
  bool connect(String id);
  void handle(void);
#endif
};

#endif /** _AG_MQTT_CLIENT_H_ */
