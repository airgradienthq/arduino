#ifndef _AIR_GRADIENT_H_
#define _AIR_GRADIENT_H_

#include "Display/Display.h"
#include "Main/BoardDef.h"
#include "Main/HardwareWatchdog.h"
#include "Main/LedBar.h"
#include "Main/PushButton.h"
#include "Main/StatusLed.h"
#include "PMS/PMS5003.h"
#include "PMS/PMS5003T.h"
#include "S8/S8.h"
#include "Sgp41/Sgp41.h"
#include "Sht/Sht.h"
#include "Main/utils.h"

#ifndef GIT_VERSION
#define GIT_VERSION "3.3.9-snap"
#endif


#ifndef ESP8266
// Airgradient server root ca certificate
const char *const AG_SERVER_ROOT_CA =
    "-----BEGIN CERTIFICATE-----\n"
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

/**
 * @brief Class with define all the sensor has supported by Airgradient. Each
 * sensor usage must be init before use.
 */
class AirGradient {
public:
  AirGradient(BoardType type);

  /**
   * @brief Plantower PMS5003 sensor
   */
  PMS5003 pms5003;
  /**
   * @brief Plantower PMS5003T sensor: connect to PM1 connector on
   * OPEN_AIR_OUTDOOR.
   */
  PMS5003T pms5003t_1;
  /**
   * @brief Plantower PMS5003T sensor: connect to PM2 connector on
   * OPEN_AIR_OUTDOOR.
   */
  PMS5003T pms5003t_2;

  /**
   * @brief SenseAirS8 CO2 sensor
   */
  S8 s8;

  /**
   * @brief Temperature and humidity sensor supported SHT3x and SHT4x
   *
   */
  Sht sht;

  /**
   * @brief  SGP41 TVOC and NOx sensor
   *
   */
  Sgp41 sgp41;

  /**
   * @brief OLED Display
   *
   */
  Display display;

  /**
   * @brief Push Button
   */
  PushButton button;

  /**
   * @brief LED
   */
  StatusLed statusLed;

  /**
   * @brief RGB LED array
   *
   */
  LedBar ledBar;

  /**
   * @brief External hardware watchdog
   */
  HardwareWatchdog watchdog;

  /**
   * @brief Get I2C SDA pin has of board supported
   *
   * @return int Pin number if -1 invalid
   */
  int getI2cSdaPin(void);
  /**
   * @brief Get I2C SCL pin has of board supported
   *
   * @return int Pin number if -1 invalid
   */
  int getI2cSclPin(void);

  /**
   * @brief Get the Board Type
   *
   * @return BoardType @ref BoardType
   */
  BoardType getBoardType(void);

  /**
   * @brief Get the library version string
   *
   * @return String
   */
  String getVersion(void);

  /**
   * @brief Get the Board Name object
   *
   * @return String
   */
  String getBoardName(void);

  /**
   * @brief Round double value with for 2 decimal
   *
   * @param valuem Round value
   * @return double
   */
  double round2(double value);

  /**
   * @brief Check that Airgradient object is ONE_INDOOR
   *
   * @return true Yes
   * @return false No
   */
  bool isOne(void);

  /**
   * @brief Check that Airgradient object is OPEN_AIR
   *
   * @return true
   * @return false
   */
  bool isOpenAir(void);

  /**
   * @brief Check that Airgradient object is DIY_PRO 4.2 indoor
   *
   * @return true Yes
   * @return false No
   */
  bool isPro4_2(void);
  /**
   * @brief Check that Airgradient object is DIY_PRO 3.7 indoor
   *
   * @return true Yes
   * @return false No
   */
  bool isPro3_3(void);

  /**
   * @brief Check that Airgradient object is DIY_BASIC
   *
   * @return true Yes
   * @return false No
   */
  bool isBasic(void);

  /**
   * @brief Get device Id
   *
   * @return String
   */
  String deviceId(void);

private:
  BoardType boardType;
};

#endif /** _AIR_GRADIENT_H_ */
