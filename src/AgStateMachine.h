#ifndef _AG_STATE_MACHINE_H_
#define _AG_STATE_MACHINE_H_

/**
 * @brief Application state machine state
 *
 */
enum AgStateMachine {
  /** In WiFi Manger Mode */
  AgStateMachineWiFiManagerMode,

  /** WiFi Manager has connected to mobile phone */
  AgStateMachineWiFiManagerPortalActive,

  /** After SSID and PW entered and OK clicked, connection to WiFI network is
     attempted*/
  AgStateMachineWiFiManagerStaConnecting,

  /** Connecting to WiFi worked */
  AgStateMachineWiFiManagerStaConnected,

  /** Once connected to WiFi an attempt to reach the server is performed */
  AgStateMachineWiFiOkServerConnecting,

  /** Server is reachable, all ﬁne */
  AgStateMachineWiFiOkServerConnected,

  /** =================================== *
   * Exceptions during WIFi Setup         *
   * =================================== **/
  /** Cannot connect to WiFi (e.g. wrong password, WPA Enterprise etc.) */
  AgStateMachineWiFiManagerConnectFailed,

  /** Connected to WiFi but server not reachable, e.g. firewall
     block/whitelisting needed etc. */
  AgStateMachineWiFiOkServerConnectFailed,

  /** Server reachable but sensor not configured correctly*/
  AgStateMachineWiFiOkServerOkSensorConfigFailed,

  /** =================================== *
   * During Normal Operation              *
   * =================================== **/

  /** Connection to WiFi network failed credentials incorrect encryption not
     supported etc. */
  AgStateMachineWiFiLost,

  /** Connected to WiFi network but the server cannot be reached through the
     internet, e.g. blocked by firewall */
  AgStateMachineServerLost,

  /** Server is reachable but there is some conﬁguration issue to be fixed on
     the server side */
  AgStateMachineSensorConfigFailed,

  AgStateMachineNormal,
};

#endif /** _AG_STATE_MACHINE_H_ */
