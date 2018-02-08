/*
  Link
  Code that supports the communication ("cross-talk") between the XBee, Mega and Raspberry Pi.
  Follows format defined in "communication_standards.md"

  Created in December 2017, QMAST
*/
// TODO: Use char* for serial input and comparison with codes saved in PROGMEM to save RAM
// TODO: Implement 02 - Blink LED Strip

#include "pins.h"
String xbeeInputBuffer = "";         // a string to hold incoming data
String rpiInputBuffer = "";         // a string to hold incoming data
// int calibrationStage;

// Operations that only need to be performed once, such as reserving variable space
void initLink() {
  xbeeInputBuffer.reserve(200); // reserve 200 bytes for the XBee input buffer:
  rpiInputBuffer.reserve(200); // reserve 200 bytes for the RPi input buffer:
}

//
void sendTransmission(int port, String code, String message) {
  if (port == PORT_RPI) {
    SERIAL_PORT_RPI.print(code);
    SERIAL_PORT_RPI.print(message);
    SERIAL_PORT_RPI.println(F(";"));
  } else if (port == PORT_XBEE) {
    SERIAL_PORT_XBEE.print(code);
    SERIAL_PORT_XBEE.print(message);
    SERIAL_PORT_XBEE.print(F(";"));
  }
}

void sendTransmission(int port, String code, int message) {
  sendTransmission(port, code, String(message));
}

void executeXBeeTransmission(String code, String data) {
  bool error = false;
  if (code.equals("A0")) {
    // Relay autopilot request to RPi
    sendTransmission(PORT_RPI, code, data);
    if (data.equals("0")) setAutopilot(false); // Exit autopilot without waiting for RPi response
  } else if (code.equals("A1") || code.equals("A2") || code.equals("A3")) {
    // Messages sent by the XBee to be relayed to RPi transparently
    sendTransmission(PORT_RPI, code, data);
  } else {
    executeCommonTransmission(PORT_XBEE, code, data);
  }
}

void executeRPiTransmission(String code, String data) {
  bool error = false;
  if (code.equals("A0")) {
    if (data.equals("1")) setAutopilot(true); // Enter autopilot
    if (data.equals("0")) setAutopilot(false); // Exit autopilot
  } else if (code.equals("05") || code.equals("07") || code.equals("A1") || code.equals("A2") || code.equals("A3") || code.equals("A8") || code.equals("A9")) {
    // Messages sent by the RPi to be relayed to XBee transparently
    sendTransmission(PORT_XBEE, code, data);
  } else {
    executeCommonTransmission(PORT_RPI, code, data);
  }
}

void executeCommonTransmission(int port, String code, String data) {
  // This function handles the transmissions whose codes call the same function
  if (data.charAt(0) == '?') {
    // Handle requests to change transmission interval (no other message has ? as it's first character)
    setSensorTransInterval(port, code, data.substring(1).toInt());
  } else if (code.equals("00")) {
    if (data.equals("1")) setLastResponse(port);
  } else if (code.equals("02")) {
    // Blink LED Strip
  } else if (code.equals("03")) {
    // TODO: Change enabled/disabled from 1/0 because to toInt fails, 0 is returned
    if (data.toInt() == 1) {
      setRCEnabled(true);
    } else if (data.toInt() == 0) {
      setRCEnabled(false);
    }
  }
}

void checkLink() {
  while (SERIAL_PORT_XBEE.available()) {
    // get the new byte:
    char inChar = (char)SERIAL_PORT_XBEE.read();
    // add it to the inputString:
    xbeeInputBuffer += inChar;
    // if the incoming character is a ";" do something about it:
    if (inChar == ';') {
      DEBUG_PRINT("Recieved (Xbee): ");
      DEBUG_PRINTLN(xbeeInputBuffer);
      String code = xbeeInputBuffer.substring(0, 2);
      String data = xbeeInputBuffer.substring(2, xbeeInputBuffer.indexOf(";"));
      executeXBeeTransmission(code, data);
      xbeeInputBuffer = "";
    }
  }
  while (SERIAL_PORT_RPI.available()) {
    // get the new byte:
    char inChar = (char)SERIAL_PORT_RPI.read();
    // add it to the inputString:
    rpiInputBuffer += inChar;
    // if the incoming character is a ";" do something about it:
    if (inChar == ';') {
      DEBUG_PRINTLN("Recieved (RPi): ");
      DEBUG_PRINTLN(rpiInputBuffer);
      String code = rpiInputBuffer.substring(0, 2);
      String data = rpiInputBuffer.substring(2, rpiInputBuffer.indexOf(";"));
      executeRPiTransmission(code, data);
      rpiInputBuffer = "";
    }
  }
}
