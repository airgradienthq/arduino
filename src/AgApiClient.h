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
#include "AirGradient.h"
#include "Main/PrintLog.h"
#include <Arduino.h>

class AgApiClient : public PrintLog {
private:
  Configuration &config;
  AirGradient *ag;
#ifdef ESP8266
  String apiRoot = "http://hw.airgradient.com";
#else
  String apiRoot = "https://hw.airgradient.com";
  const char *agCA = "-----BEGIN CERTIFICATE-----\n"
                     "MIIF4jCCA8oCCQD7MgvcaVWxkTANBgkqhkiG9w0BAQsFADCBsjELMAkGA1UEBhMC\n"
                     "VEgxEzARBgNVBAgMCkNoaWFuZyBNYWkxEDAOBgNVBAcMB01hZSBSaW0xGTAXBgNV\n"
                     "BAoMEEFpckdyYWRpZW50IEx0ZC4xFDASBgNVBAsMC1NlbnNvciBMYWJzMSgwJgYD\n"
                     "VQQDDB9BaXJHcmFkaWVudCBTZW5zb3IgTGFicyBSb290IENBMSEwHwYJKoZIhvcN\n"
                     "AQkBFhJjYUBhaXJncmFkaWVudC5jb20wHhcNMjEwOTE3MTE0NDE3WhcNNDEwOTEy\n"
                     "MTE0NDE3WjCBsjELMAkGA1UEBhMCVEgxEzARBgNVBAgMCkNoaWFuZyBNYWkxEDAO\n"
                     "BgNVBAcMB01hZSBSaW0xGTAXBgNVBAoMEEFpckdyYWRpZW50IEx0ZC4xFDASBgNV\n"
                     "BAsMC1NlbnNvciBMYWJzMSgwJgYDVQQDDB9BaXJHcmFkaWVudCBTZW5zb3IgTGFi\n"
                     "cyBSb290IENBMSEwHwYJKoZIhvcNAQkBFhJjYUBhaXJncmFkaWVudC5jb20wggIi\n"
                     "MA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQC6XkVQ4O9d5GcUjPYRgF/uaY6O\n"
                     "5ry1xCGvotxkEeKkBk99lB1oNUUfNsP5bwuDci4XKfY9Ro6/jmkfHSVcPAwUnjAt\n"
                     "BcHqZtA/cMXykaynf9yXPxPQN7XLu/Rk32RIfb90sIGS318xgNziCYvzWZmlxpxc\n"
                     "3gUcAgGtamlgZ6wD3yOHVo8B9aFNvmP16QwkUm8fKDHunJG+iX2Bxa4ka5FJovhG\n"
                     "TnUwtso6Vrn0JaWF9qWcPZE0JZMjFW8PYRriyJmHwr/nAXfPPKphD1oRO+oA7/jq\n"
                     "dYkrJw6+OHfFXnPB1xkeh4OPBzcCZHT5XWNfwBYazYpjcJa9ngGFSmg8lX1ac23C\n"
                     "zea1XJmSrPwbZbWxoQznnf7Y78mRjruYKgSP8rf74KYvBe/HGPL5NQyXQ3l6kwmu\n"
                     "CCUqfcC0wCWEtWESxwSdFE2qQii8CZ12kQExzvR2PrOIyKQYSdkGx9/RBZtAVPXP\n"
                     "hmLuRBQYHrF5Cxf1oIbBK8OMoNVgBm6ftt15t9Sq9dH5Aup2YR6WEJkVaYkYzZzK\n"
                     "X7M+SQcdbXp+hAO8PFpABJxkaDAO2kiB5Ov7pDYPAcmNFqnJT48AY0TZJeVeCa5W\n"
                     "sIv3lPvB/XcFjP0+aZxxNSEEwpGPUYgvKUYUUmb0NammlYQwZHKaShPEmZ3UZ0bp\n"
                     "VNt4p6374nzO376sSwIDAQABMA0GCSqGSIb3DQEBCwUAA4ICAQB/LfBPgTx7xKQB\n"
                     "JNMUhah17AFAn050NiviGJOHdPQely6u3DmJGg+ijEVlPWO1FEW3it+LOuNP5zOu\n"
                     "bhq8paTYIxPxtALIxw5ksykX9woDuX3H6FF9mPdQIbL7ft+3ZtZ4FWPui9dUtaPe\n"
                     "ZBmDFDi4U29nhWZK68JSp5QkWjfaYLV/vtag7120eVyGEPFZ0UAuTUNqpw+stOt9\n"
                     "gJ2ZxNx13xJ8ZnLK7qz1crPe8/8IVAdxbVLoY7JaWPLc//+VF+ceKicy8+4gV7zN\n"
                     "Gnq2IyM+CHFz8VYMLbW+3eVp4iJjTa72vae116kozboEIUVN9rgLqIKyVqQXiuoN\n"
                     "g3xY+yfncPB2+H/+lfyy6mepPIfgksd3+KeNxFADSc5EVY2JKEdorRodnAh7a8K6\n"
                     "WjTYgq+GjWXU2uQW2SyPt6Tu33OT8nBnu3NB80eT8WXgdVCkgsuyCuLvNRf1Xmze\n"
                     "igvurpU6JmQ1GlLgLJo8omJHTh1zIbkR9injPYne2v9ciHCoP6+LDEqe+rOsvPCB\n"
                     "C/o/iZ4svmYX4fWGuU7GgqZE8hhrC3+GdOTf2ADC752cYCZxBidXGtkrGNoHQKmQ\n"
                     "KCOMFBxZIvWteB3tUo3BKYz1D2CvKWz1wV4moc5JHkOgS+jqxhvOkQ/vfQBQ1pUY\n"
                     "TMui9BSwU7B1G2XjdLbfF3Dc67zaSg==\n"
                     "-----END CERTIFICATE-----\n";
#endif

  bool apiRootChanged = false; // Indicate if setApiRoot() is called
  bool getConfigFailed;
  bool postToServerFailed;
  bool notAvailableOnDashboard = false; // Device not setup on Airgradient cloud dashboard.
  uint16_t timeoutMs = 10000;           // Default set to 10s

public:
  AgApiClient(Stream &stream, Configuration &config);
  ~AgApiClient();

  void begin(void);
  bool fetchServerConfiguration(void);
  bool postToServer(String data);
  bool isFetchConfigureFailed(void);
  bool isPostToServerFailed(void);
  bool isNotAvailableOnDashboard(void);
  void setAirGradient(AirGradient *ag);
  bool sendPing(int rssi, int bootCount);
  String getApiRoot() const;
  void setApiRoot(const String &apiRoot);
  void setTimeout(uint16_t timeoutMs);
};

#endif /** _AG_API_CLIENT_H_ */
