#ifndef _AG_MQTT_CLIENT_H_
#define _AG_MQTT_CLIENT_H_

#include "mqtt_client.h"
#include "Main/PrintLog.h"
#include <Arduino.h>

class MqttClient: public PrintLog {
private:
  bool isBegin = false;
  String uri;
  esp_mqtt_client_handle_t client;
  bool connected = false;
  int connectionFailedCount = 0;

public:
  MqttClient(Stream &debugLog);
  ~MqttClient();

  bool begin(String uri);
  void end(void);
  void _updateConnected(bool connected);
  bool publish(String &topic, String &payload);
  bool isCurrentUri(String &uri);
  bool isConnected(void);
  int getConnectionFailedCount(void);
};

#endif /** _AG_MQTT_CLIENT_H_ */
