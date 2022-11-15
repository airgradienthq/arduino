/*
  Test.h - Test library for Wiring - description
  Copyright (c) 2006 John Doe.  All right reserved.
*/

// ensure this library description is only included once
#ifndef AirGradient_h
#define AirGradient_h

#include <SoftwareSerial.h>
#include <Print.h>
#include "Stream.h"
//MHZ19 CONSTANTS START
// types of sensors.
extern const int MHZ14A;
extern const int MHZ19B;

// status codes
extern const int STATUS_NO_RESPONSE;
extern const int STATUS_CHECKSUM_MISMATCH;
extern const int STATUS_INCOMPLETE;
extern const int STATUS_NOT_READY;
//MHZ19 CONSTANTS END

//ENUMS AND STRUCT FOR TMP_RH START
typedef enum {
      SHT3XD_CMD_READ_SERIAL_NUMBER = 0x3780,

      SHT3XD_CMD_READ_STATUS = 0xF32D,
      SHT3XD_CMD_CLEAR_STATUS = 0x3041,

      SHT3XD_CMD_HEATER_ENABLE = 0x306D,
      SHT3XD_CMD_HEATER_DISABLE = 0x3066,

      SHT3XD_CMD_SOFT_RESET = 0x30A2,

      SHT3XD_CMD_CLOCK_STRETCH_H = 0x2C06,
      SHT3XD_CMD_CLOCK_STRETCH_M = 0x2C0D,
      SHT3XD_CMD_CLOCK_STRETCH_L = 0x2C10,

      SHT3XD_CMD_POLLING_H = 0x2400,
      SHT3XD_CMD_POLLING_M = 0x240B,
      SHT3XD_CMD_POLLING_L = 0x2416,

      SHT3XD_CMD_ART = 0x2B32,

      SHT3XD_CMD_PERIODIC_HALF_H = 0x2032,
      SHT3XD_CMD_PERIODIC_HALF_M = 0x2024,
      SHT3XD_CMD_PERIODIC_HALF_L = 0x202F,
      SHT3XD_CMD_PERIODIC_1_H = 0x2130,
      SHT3XD_CMD_PERIODIC_1_M = 0x2126,
      SHT3XD_CMD_PERIODIC_1_L = 0x212D,
      SHT3XD_CMD_PERIODIC_2_H = 0x2236,
      SHT3XD_CMD_PERIODIC_2_M = 0x2220,
      SHT3XD_CMD_PERIODIC_2_L = 0x222B,
      SHT3XD_CMD_PERIODIC_4_H = 0x2334,
      SHT3XD_CMD_PERIODIC_4_M = 0x2322,
      SHT3XD_CMD_PERIODIC_4_L = 0x2329,
      SHT3XD_CMD_PERIODIC_10_H = 0x2737,
      SHT3XD_CMD_PERIODIC_10_M = 0x2721,
      SHT3XD_CMD_PERIODIC_10_L = 0x272A,

      SHT3XD_CMD_FETCH_DATA = 0xE000,
      SHT3XD_CMD_STOP_PERIODIC = 0x3093,

      SHT3XD_CMD_READ_ALR_LIMIT_LS = 0xE102,
      SHT3XD_CMD_READ_ALR_LIMIT_LC = 0xE109,
      SHT3XD_CMD_READ_ALR_LIMIT_HS = 0xE11F,
      SHT3XD_CMD_READ_ALR_LIMIT_HC = 0xE114,

      SHT3XD_CMD_WRITE_ALR_LIMIT_HS = 0x611D,
      SHT3XD_CMD_WRITE_ALR_LIMIT_HC = 0x6116,
      SHT3XD_CMD_WRITE_ALR_LIMIT_LC = 0x610B,
      SHT3XD_CMD_WRITE_ALR_LIMIT_LS = 0x6100,

      SHT3XD_CMD_NO_SLEEP = 0x303E,
    } TMP_RH_Commands;


    typedef enum {
      SHT3XD_REPEATABILITY_HIGH,
      SHT3XD_REPEATABILITY_MEDIUM,
      SHT3XD_REPEATABILITY_LOW,
    } TMP_RH_Repeatability;

    typedef enum {
      SHT3XD_MODE_CLOCK_STRETCH,
      SHT3XD_MODE_POLLING,
    } TMP_RH_Mode;

    typedef enum {
      SHT3XD_FREQUENCY_HZ5,
      SHT3XD_FREQUENCY_1HZ,
      SHT3XD_FREQUENCY_2HZ,
      SHT3XD_FREQUENCY_4HZ,
      SHT3XD_FREQUENCY_10HZ
    } TMP_RH_Frequency;

    typedef enum {
      SHT3XD_NO_ERROR = 0,

      SHT3XD_CRC_ERROR = -101,
      SHT3XD_TIMEOUT_ERROR = -102,

      SHT3XD_PARAM_WRONG_MODE = -501,
      SHT3XD_PARAM_WRONG_REPEATABILITY = -502,
      SHT3XD_PARAM_WRONG_FREQUENCY = -503,
      SHT3XD_PARAM_WRONG_ALERT = -504,

      // Wire I2C translated error codes
      SHT3XD_WIRE_I2C_DATA_TOO_LOG = -10,
      SHT3XD_WIRE_I2C_RECEIVED_NACK_ON_ADDRESS = -20,
      SHT3XD_WIRE_I2C_RECEIVED_NACK_ON_DATA = -30,
      SHT3XD_WIRE_I2C_UNKNOW_ERROR = -40
    } TMP_RH_ErrorCode;

    typedef union {
      uint16_t rawData;
      struct {
        uint8_t WriteDataChecksumStatus : 1;
        uint8_t CommandStatus : 1;
        uint8_t Reserved0 : 2;
        uint8_t SystemResetDetected : 1;
        uint8_t Reserved1 : 5;
        uint8_t T_TrackingAlert : 1;
        uint8_t RH_TrackingAlert : 1;
        uint8_t Reserved2 : 1;
        uint8_t HeaterStatus : 1;
        uint8_t Reserved3 : 1;
        uint8_t AlertPending : 1;
      };
    } TMP_RH_RegisterStatus;

    struct TMP_RH {
      float t;
      int rh;
      char t_char[10];
      char rh_char[10];
      TMP_RH_ErrorCode error;
    };
    struct TMP_RH_Char {
      TMP_RH_ErrorCode error;
    };
// ENUMS AND STRUCTS FOR TMP_RH END

//ENUMS STRUCTS FOR CO2 START
    struct CO2_READ_RESULT {
    int co2 = -1;
    bool success = false;
};
//ENUMS STRUCTS FOR CO2 END

// library interface description
class AirGradient
{
  // user-accessible "public" interface
  public:
    AirGradient(bool displayMsg=false,int baudRate=9600);
    //void begin(int baudRate=9600);

    static void setOutput(Print& debugOut, bool verbose = true);

    void beginCO2(void);
    void beginCO2(int,int);
    void PMS_Init(void);
    void PMS_Init(int,int);
    void PMS_Init(int,int,int);

    bool _debugMsg;


    //PMS VARIABLES PUBLIC_START
    static const uint16_t SINGLE_RESPONSE_TIME = 1000;
    static const uint16_t TOTAL_RESPONSE_TIME = 1000 * 10;
    static const uint16_t STEADY_RESPONSE_TIME = 1000 * 30;

    static const uint16_t BAUD_RATE = 9600;

    struct DATA {
      // Standard Particles, CF=1
      uint16_t PM_SP_UG_1_0;
      uint16_t PM_SP_UG_2_5;
      uint16_t PM_SP_UG_10_0;

      // Atmospheric environment
      uint16_t PM_AE_UG_1_0;
      uint16_t PM_AE_UG_2_5;
      uint16_t PM_AE_UG_10_0;

      // Raw particles count (number of particles in 0.1l of air
      uint16_t PM_RAW_0_3;
      uint16_t PM_RAW_0_5;
      uint16_t PM_RAW_1_0;
      uint16_t PM_RAW_2_5;
      uint16_t PM_RAW_5_0;
      uint16_t PM_RAW_10_0;

      // Formaldehyde (HCHO) concentration in mg/m^3 - PMSxxxxST units only
      uint16_t AMB_HCHO;

      // Temperature & humidity - PMSxxxxST units only
      int16_t PM_TMP;
      uint16_t PM_HUM;
    };

    void PMS(Stream&);
    void sleep();
    void wakeUp();
    void activeMode();
    void passiveMode();

    void requestRead();
    bool read_PMS(DATA& data);
    bool readUntil(DATA& data, uint16_t timeout = SINGLE_RESPONSE_TIME);


    const char* getPM2();
    int getPM2_Raw();
    int getPM1_Raw();
    int getPM10_Raw();

    int getPM0_3Count();
    int getPM0_5Count();
    int getPM1_0Count();
    int getPM2_5Count();
    int getPM5_0Count();
    int getPM10_0Count();

    int getAMB_TMP();
    int getAMB_HUM();

    //PMS VARIABLES PUBLIC_END

    //TMP_RH VARIABLES PUBLIC START
    void ClosedCube_TMP_RH();
    TMP_RH_ErrorCode TMP_RH_Init(uint8_t address);
    TMP_RH_ErrorCode clearAll();

     TMP_RH_ErrorCode softReset();
    TMP_RH_ErrorCode reset(); // same as softReset

    uint32_t readSerialNumber();
    uint32_t testTMP_RH();

    TMP_RH_ErrorCode periodicStart(TMP_RH_Repeatability repeatability, TMP_RH_Frequency frequency);
    TMP_RH periodicFetchData();
    TMP_RH_ErrorCode periodicStop();

    //TMP_RH VARIABLES PUBLIC END

    //CO2 VARIABLES PUBLIC START
    void CO2_Init();
    void CO2_Init(int,int);
    void CO2_Init(int,int,int);
    int getCO2(int numberOfSamplesToTake = 5);
    int getCO2_Raw();
    SoftwareSerial *_SoftSerial_CO2;

    //CO2 VARIABLES PUBLIC END

    //MHZ19 VARIABLES PUBLIC START
    void MHZ19_Init(uint8_t);
    void MHZ19_Init(int,int,uint8_t);
    void MHZ19_Init(int,int,int,uint8_t);
    void setDebug_MHZ19(bool enable);
    bool isPreHeating_MHZ19();
    bool isReady_MHZ19();

    int readMHZ19();

    //MHZ19 VARIABLES PUBLIC END



  // library-accessible "private" interface
  private:
    int value;


     //PMS VARIABLES PRIVATE START
    enum STATUS { STATUS_WAITING, STATUS_OK };
    enum MODE { MODE_ACTIVE, MODE_PASSIVE };

    uint8_t _payload[32];
    Stream* _stream;
    DATA* _data;
    STATUS _PMSstatus;
    MODE _mode = MODE_ACTIVE;

    uint8_t _index = 0;
    uint16_t _frameLen;
    uint16_t _checksum;
    uint16_t _calculatedChecksum;
    SoftwareSerial *_SoftSerial_PMS;
    void loop();
    char Char_PM1[10];
    	char Char_PM2[10];
    	char Char_PM10[10];
    //PMS VARIABLES PRIVATE END

    //TMP_RH VARIABLES PRIVATE START
    uint8_t _address;
    TMP_RH_RegisterStatus _status;

    TMP_RH_ErrorCode writeCommand(TMP_RH_Commands command);
    TMP_RH_ErrorCode writeAlertData(TMP_RH_Commands command, float temperature, float humidity);

    uint8_t checkCrc(uint8_t data[], uint8_t checksum);
    uint8_t calculateCrc(uint8_t data[]);

    float calculateHumidity(uint16_t rawValue);
    float calculateTemperature(uint16_t rawValue);

    TMP_RH readTemperatureAndHumidity();
    TMP_RH_ErrorCode read_TMP_RH(uint16_t* data, uint8_t numOfPair);

    TMP_RH returnError(TMP_RH_ErrorCode command);
    //TMP_RH VARIABLES PRIVATE END

    //CO2 VARABLES PUBLIC START
    char Char_CO2[10];

    //CO2 VARABLES PUBLIC END
    //MHZ19 VARABLES PUBLIC START

    int readInternal_MHZ19();

    uint8_t _type_MHZ19, temperature_MHZ19;
    bool debug_MHZ19 = false;

    Stream * _serial_MHZ19;
    SoftwareSerial *_SoftSerial_MHZ19;
    uint8_t getCheckSum_MHZ19(unsigned char *packet);

    //MHZ19 VARABLES PUBLIC END

};


#endif

