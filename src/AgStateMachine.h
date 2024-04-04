#ifndef _AG_STATE_MACHINE_H_
#define _AG_STATE_MACHINE_H_

#include "AgOledDisplay.h"
#include "AgValue.h"
#include "AgConfigure.h"
#include "Main/PrintLog.h"
#include "App/AppDef.h"

class AgStateMachine : public PrintLog {
private:
  // AgStateMachineState state;
  AgStateMachineState ledState;
  AgStateMachineState dispState;
  AirGradient *ag;
  AgOledDisplay &disp;
  AgValue &value;
  AgConfigure &config;

  bool addToDashBoard = false;
  uint32_t addToDashboardTime;
  int wifiConnectCountDown;
  int ledBarAnimationCount;

  void ledBarSingleLedAnimation(uint8_t r, uint8_t g, uint8_t b);
  void ledStatusBlinkDelay(uint32_t delay);
  void sensorLedHandle(void);
  void co2LedHandle(void);
  void pm25LedHandle(void);

public:
  AgStateMachine(AgOledDisplay &disp, Stream &log,
                 AgValue &value, AgConfigure& config);
  ~AgStateMachine();
  void setAirGradient(AirGradient* ag);
  void displayHandle(AgStateMachineState state);
  void displayHandle(void);
  void displaySetAddToDashBoard(void);
  void displayClearAddToDashBoard(void);
  void displayWiFiConnectCountDown(int count);
  void ledAnimationInit(void);
  void ledHandle(AgStateMachineState state);
  void ledHandle(void);
  void setDisplayState(AgStateMachineState state);
  AgStateMachineState getDisplayState(void);
};

#endif /** _AG_STATE_MACHINE_H_ */
