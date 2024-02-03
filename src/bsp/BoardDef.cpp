#include "BoardDef.h"
#if defined(ESP32)
#include "esp32-hal-log.h"
#endif

const BoardDef bsps[BOARD_DEF_MAX] = {
    /** BOARD_DIY_BASIC_KIT */
    [BOARD_DIY_BASIC_KIT] =
        {
            .SenseAirS8 =
                {
                    .uart_tx_pin = 2,
                    .uart_rx_pin = 0,
#if defined(ESP8266)
                    .supported = true,
#else
                    .supported = false,
#endif
                },
            .PMS5003 =
                {
                    .uart_tx_pin = 14,
                    .uart_rx_pin = 12,
#if defined(ESP8266)
                    .supported = true,
#else
                    .supported = false,
#endif
                },
            .I2C =
                {
                    .sda_pin = 4,
                    .scl_pin = 5,
#if defined(ESP8266)
                    .supported = true,
#else
                    .supported = false,
#endif
                },
            .SW =
                {
                    .pin = -1,          /** Not supported */
                    .activeLevel = 0,   /** Don't care */
                    .supported = false, /** Not supported */
                },
            .LED =
                {
                    .pin = -1,
                    .rgbNum = 0,
                    .onState = 0,
                    .supported = false,
                    .rgbSupported = false,
                },
            .OLED =
                {
#if defined(ESP8266)
                    .width = 64,
                    .height = 48,
                    .addr = 0x3C,
                    .supported = true,
#else
                    .width = 0,
                    .height = 0,
                    .addr = 0,
                    .supported = false,
#endif
                },
            .WDG =
                {
                    .resetPin = -1,
                    .supported = false,
                },
            .name = "BOARD_DIY_BASIC_KIT",
        },
    /** BOARD_DIY_PRO_INDOOR_V4_2 */
    [BOARD_DIY_PRO_INDOOR_V4_2] =
        {
            .SenseAirS8 =
                {
                    .uart_tx_pin = 2,
                    .uart_rx_pin = 0,
#if defined(ESP8266)
                    .supported = true,
#else
                    .supported = false,
#endif
                },
            .PMS5003 =
                {
                    .uart_tx_pin = 14,
                    .uart_rx_pin = 12,
#if defined(ESP8266)
                    .supported = true,
#else
                    .supported = false,
#endif
                },
            .I2C =
                {
                    .sda_pin = 4,
                    .scl_pin = 5,
#if defined(ESP8266)
                    .supported = true,
#else
                    .supported = false,
#endif
                },
            .SW =
                {
#if defined(ESP8266)
                    .pin = 13, /** D7 */
                    .activeLevel = 0,
                    .supported = true,
#else
                    .pin = -1,
                    .activeLevel = 1,
                    .supported = false,
#endif
                },
            .LED =
                {
                    .pin = -1,
                    .rgbNum = 0,
                    .onState = 0,
                    .supported = false,
                    .rgbSupported = false,
                },
            .OLED =
                {
#if defined(ESP8266)
                    .width = 128,
                    .height = 64,
                    .addr = 0x3C,
                    .supported = true,
#else
                    .width = 0,
                    .height = 0,
                    .addr = 0,
                    .supported = false,
#endif
                },
            .WDG =
                {
                    .resetPin = -1,
                    .supported = false,
                },
            .name = "BOARD_DIY_PRO_INDOOR_V4_2",
        },
    /** BOARD_ONE_INDOOR_MONITOR_V9_0 */
    [BOARD_ONE_INDOOR_MONITOR_V9_0] =
        {
            .SenseAirS8 =
                {
                    .uart_tx_pin = 1,
                    .uart_rx_pin = 0,
#if defined(ESP8266)
                    .supported = false,
#else
                    .supported = true,
#endif
                },
            /** Use UART0 don't use define pin number */
            .PMS5003 =
                {
                    .uart_tx_pin = -1,
                    .uart_rx_pin = -1,
#if defined(ESP8266)
                    .supported = false,
#else
                    .supported = true,
#endif
                },
            .I2C =
                {
                    .sda_pin = 7,
                    .scl_pin = 6,
#if defined(ESP8266)
                    .supported = false,
#else
                    .supported = true,
#endif
                },
            .SW =
                {
#if defined(ESP8266)
                    .pin = -1,
                    .activeLevel = 1,
                    .supported = false,
#else
                    .pin = 9,
                    .activeLevel = 0,
                    .supported = true,
#endif
                },
            .LED =
                {
#if defined(ESP8266)
                    .pin = -1,
                    .rgbNum = 0,
                    .onState = 0,
                    .supported = false,
                    .rgbSupported = false,
#else
                    .pin = 10,
                    .rgbNum = 11,
                    .onState = 1,
                    .supported = true,
                    .rgbSupported = true,
#endif
                },
            .OLED =
                {
#if defined(ESP8266)
                    .width = 0,
                    .height = 0,
                    .addr = 0,
                    .supported = false,
#else
                    .width = 128,
                    .height = 64,
                    .addr = 0x3C,
                    .supported = true,
#endif
                },
            .WDG =
                {
#if defined(ESP8266)
                    .resetPin = -1,
                    .supported = false,
#else
                    .resetPin = 2,
                    .supported = true,
#endif
                },
            .name = "BOARD_ONE_INDOOR_MONITOR_V9_0",
        },
    /** BOARD_OUTDOOR_MONITOR_V1_3 */
    [BOARD_OUTDOOR_MONITOR_V1_3] = {
        .SenseAirS8 =
            {
                .uart_tx_pin = 1,
                .uart_rx_pin = 0,
#if defined(ESP8266)
                .supported = false,
#else
                .supported = true,
#endif
            },
        /** Use UART0 don't use define pin number */
        .PMS5003 =
            {
                .uart_tx_pin = -1,
                .uart_rx_pin = -1,
#if defined(ESP8266)
                .supported = false,
#else
                .supported = true,
#endif
            },
        .I2C =
            {
                .sda_pin = 7,
                .scl_pin = 6,
#if defined(ESP8266)
                .supported = false,
#else
                .supported = true,
#endif
            },
        .SW =
            {
#if defined(ESP8266)
                .pin = -1,
                .activeLevel = 1,
                .supported = false,
#else
                .pin = 9,
                .activeLevel = 0,
                .supported = true,
#endif
            },
        .LED =
            {
#if defined(ESP8266)
                .pin = -1,
                .rgbNum = 0,
                .onState = 0,
                .supported = false,
                .rgbSupported = false,
#else
                .pin = 10,
                .rgbNum = 0,
                .onState = 1,
                .supported = true,
                .rgbSupported = false,
#endif
            },
        .OLED =
            {
#if defined(ESP8266)
                .width = 0,
                .height = 0,
                .addr = 0,
                .supported = false,
#else
                .width = 128,
                .height = 64,
                .addr = 0x3C,
                .supported = true,
#endif
            },
        .WDG =
            {
#if defined(ESP8266)
                .resetPin = -1,
                .supported = false,
#else
                .resetPin = 2,
                .supported = true,
#endif
            },
        .name = "BOARD_OUTDOOR_MONITOR_V1_3",
    }};

/**
 * @brief Get Board Support Package
 *
 * @param def Board define @ref BoardType
 * @return const BoardDef*
 */
const BoardDef *getBoardDef(BoardType def) {
  if (def >= BOARD_DEF_MAX) {
    return NULL;
  }
  return &bsps[def];
}

#if defined(ESP8266)
#define bspPrintf(c, ...)                                                      \
  if (_debug != nullptr) {                                                     \
    _debug->printf("[BSP] " c "\r\n", ##__VA_ARGS__);                          \
  }
#else
#define bspPrintf(c, ...) log_i(c, ##__VA_ARGS__)
#endif

/**
 * @brief Print list of support Board and sensor
 *
 * @param _debug Serial debug
 */
void printBoardDef(Stream *_debug) {
#if defined(ESP8266)
  if (_debug == NULL) {
    return;
  }
#endif

  for (int i = 0; i < BOARD_DEF_MAX; i++) {
    bspPrintf("Board name: %s", bsps[i].name);
    bspPrintf("\tSensor CO2 S8:");
    bspPrintf("\t\tSupported: %d", bsps[i].SenseAirS8.supported);
    if (bsps[i].SenseAirS8.supported) {
      bspPrintf("\t\tUART Tx: %d", bsps[i].SenseAirS8.uart_tx_pin);
      bspPrintf("\t\tUART Rx: %d", bsps[i].SenseAirS8.uart_rx_pin);
    }

    bspPrintf("\tSensor PMS5003:");
    bspPrintf("\t\tSupported: %d", bsps[i].PMS5003.supported);
    if (bsps[i].PMS5003.supported) {
      bspPrintf("\t\tUART Tx: %d", bsps[i].PMS5003.uart_tx_pin);
      bspPrintf("\t\tUART Rx: %d", bsps[i].PMS5003.uart_rx_pin);
    }

    bspPrintf("\tI2C");
    bspPrintf("\t\tSupported: %d", bsps[i].I2C.supported);
    if (bsps[i].I2C.supported) {
      bspPrintf("\t\tI2C SCL: %d", bsps[i].I2C.scl_pin);
      bspPrintf("\t\tI2C SDA: %d", bsps[i].I2C.sda_pin);
    }

    bspPrintf("\tSwitch");
    bspPrintf("\t\tSupported: %d", bsps[i].SW.supported);
    if (bsps[i].SW.supported) {
      bspPrintf("\t\tPin         : %d", bsps[i].SW.pin);
      bspPrintf("\t\tActive Level: %d", bsps[i].SW.activeLevel);
    }

    bspPrintf("\tLED");
    bspPrintf("\t\tSupported: %d", bsps[i].LED.supported);
    if (bsps[i].LED.supported) {
      bspPrintf("\t\tPin : %d", bsps[i].LED.pin);
      bspPrintf("\t\tRGB : %d", bsps[i].LED.rgbSupported);
      if (bsps[i].LED.rgbSupported) {
        bspPrintf("\t\tNumber of RGB: %d", bsps[i].LED.rgbNum);
      } else {
        bspPrintf("\t\tLED state ON: %d (Single LED)", bsps[i].LED.onState);
      }
    }

    bspPrintf("\tOLED");
    bspPrintf("\t\tSupported: %d", bsps[i].OLED.supported);
    if (bsps[i].OLED.supported) {
      bspPrintf("\t\tWidth   : %d", bsps[i].OLED.width);
      bspPrintf("\t\tHeigth  : %d", bsps[i].OLED.height);
      bspPrintf("\t\tI2C Addr: %d", bsps[i].OLED.addr);
    }

    bspPrintf("\tWatchDog");
    bspPrintf("\t\tSupported: %d", bsps[i].WDG.supported);
    if (bsps[i].OLED.supported) {
      bspPrintf("\t\tReset Pin: %d", bsps[i].WDG.resetPin);
    }
  }
}

bool getBoardDef_I2C_Supported(const BoardDef *bsp) {
  if (bsp == nullptr) {
    return false;
  }
  return bsp->I2C.supported;
}

int getBoardDef_I2C_SDA(const BoardDef *bsp) {
  if ((bsp == nullptr) || (bsp->I2C.supported == false)) {
    return -1;
  }
  return bsp->I2C.sda_pin;
}

int getBoardDef_I2C_SCL(const BoardDef *bsp) {
  if ((bsp == nullptr) || (bsp->I2C.supported == false)) {
    return -1;
  }
  return bsp->I2C.scl_pin;
}

bool getBoardDef_SW_Supported(const BoardDef *bsp) {
  if (bsp == nullptr) {
    return false;
  }
  return bsp->SW.supported;
}

int getBoardDef_SW_Pin(const BoardDef *bsp) {
  if ((bsp == nullptr) || (bsp->SW.supported == false)) {
    return -1;
  }
  return bsp->SW.supported;
}

int getBoardDef_SW_ActiveLevel(const BoardDef *bsp) {
  if (bsp == nullptr) {
    return 0;
  }
  return bsp->SW.activeLevel;
}

void AirGradientBspWdgInit(const BoardDef *bsp) {
  if (bsp == nullptr) {
    return;
  }
  if (bsp->WDG.supported) {
    pinMode(bsp->WDG.resetPin, OUTPUT);
    digitalWrite(bsp->WDG.resetPin, LOW);
    delay(25); // Delay 25ms
    digitalWrite(bsp->WDG.resetPin, HIGH);
  }
}

/**
 * @brief Begin reset external watchdog. Must call @ref AirGradientBspWdgFeedEnd
 * after 20 ms
 *
 * @param bsp
 */
void AirGradientBspWdgFeedBegin(const BoardDef *bsp) {
  if (bsp == nullptr) {
    return;
  }

  if (bsp->WDG.supported) {
    digitalWrite(bsp->WDG.resetPin, HIGH);
  }
}

/**
 * @brief Call this function to finish watchdog feed after call @ref
 * AirGradientBspWdgFeedBegin 25 ms
 *
 * @param bsp
 */
void AirGradientBspWdgFeedEnd(const BoardDef *bsp) {
  if (bsp == nullptr) {
    return;
  }

  if (bsp->WDG.supported) {
    digitalWrite(bsp->WDG.resetPin, LOW);
  }
}
