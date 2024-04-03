/**
 * @file AgApiClient.h
 * @brief HTTP client connect post data to Aigradient cloud.
 * @version 0.1
 * @date 2024-Apr-02
 *
 * @copyright Copyright (c) 2024
 *
 */

#ifndef _AG_API_CLIENT_H_
#define _AG_API_CLIENT_H_

#include "AgConfigure.h"
#include <Arduino.h>

class AgApiClient {
private:
  Stream &debugLog;
  AgConfigure &config;

  bool getConfigFailed;;
  bool postToServerFailed;

  void printLog(String log);

public:
  AgApiClient(Stream &stream, AgConfigure &config);
  ~AgApiClient();

  void begin(void);
  bool fetchServerConfiguration(String deviceId);
  bool postToServer(String deviceId, String data);
  bool isFetchConfigureFailed(void);
  bool isPostToServerFailed(void);
};

#endif /** _AG_API_CLIENT_H_ */
