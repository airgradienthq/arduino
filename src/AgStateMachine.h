#ifndef _AG_STATE_MACHINE_H_
#define _AG_STATE_MACHINE_H_

#include "AgOledDisplay.h"
#include "AgValue.h"
#include "AgConfigure.h"
#include "Main/PrintLog.h"
#include "App/AppDef.h"

class StateMachine : public PrintLog {
private:
  // AgStateMachineState state;
  AgStateMachineState ledState;
  AgStateMachineState dispState;
  AirGradient *ag;
  OledDisplay &disp;
  Measurements &value;
  Configuration &config;
  bool addToDashBoard = false;
  uint32_t addToDashboardTime;
  int wifiConnectCountDown;
  int ledBarAnimationCount;

  void ledBarSingleLedAnimation(uint8_t r, uint8_t g, uint8_t b);
  void ledStatusBlinkDelay(uint32_t delay);
  void sensorhandleLeds(void);
  void co2handleLeds(void);
  void pm25handleLeds(void);
  void co2Calibration(void);
  void ledBarTest(void);
  void ledBarRunTest(void);
  void runLedTest(char color);

public:
  StateMachine(OledDisplay &disp, Stream &log,
                 Measurements &value, Configuration& config);
  ~StateMachine();
  void setAirGradient(AirGradient* ag);
  void displayHandle(AgStateMachineState state);
  void displayHandle(void);
  void displaySetAddToDashBoard(void);
  void displayClearAddToDashBoard(void);
  void displayWiFiConnectCountDown(int count);
  void ledAnimationInit(void);
  void handleLeds(AgStateMachineState state);
  void handleLeds(void);
  void setDisplayState(AgStateMachineState state);
  AgStateMachineState getDisplayState(void);
  AgStateMachineState getLedState(void);
  void executeCo2Calibration(void);
  void executeLedBarTest(void);

  enum OtaState {
    OTA_STATE_BEGIN,
    OTA_STATE_FAIL,
    OTA_STATE_PROCESSING,
    OTA_STATE_SUCCESS
  };
  void executeOTA(OtaState state, String msg, int processing);
};

#endif /** _AG_STATE_MACHINE_H_ */
