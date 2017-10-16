#include "Globals.h"

String transmissionBuffer = "";         // a string to hold incoming data

void initXbee() {
  SERIAL_PORT_XBEE.begin(9600);
  transmissionBuffer.reserve(200); // reserve 200 bytes for the inputString:
}

void sendTransmission(int code, int resp_code, String message) {
  SERIAL_PORT_XBEE.print(code);
  SERIAL_PORT_XBEE.print(",");
  SERIAL_PORT_XBEE.print(resp_code);
  SERIAL_PORT_XBEE.print(",");
  SERIAL_PORT_XBEE.print(message);
  SERIAL_PORT_XBEE.print(";");
}

void updateAndExecute() {
  // Checks for new bytes and executes commands if found
  if (SERIAL_PORT_XBEE.available()) {
    // get the new byte:
    char inChar = (char)SERIAL_PORT_XBEE.read();
    //Serial.print(inChar);
    // add it to the inputString:
    transmissionBuffer += inChar;
    // if the incoming character is a ";" do something about it:
    if (inChar == ';') {
      int iCommand = transmissionBuffer.substring(0, 2).toInt();
      String data = transmissionBuffer.substring(3, transmissionBuffer.indexOf(";"));
      switch (iCommand) {
        case 20:
          SERIAL_PORT_XBEE.print("20,200,added;");
          break;
        case 21:
          if (data == "0") {
            //digitalWrite(ledPin, LOW);
            ledSuccess();
            SERIAL_PORT_XBEE.print("21,200,0;");
          } else if (data == "1") {
            ledSuccess();
            //digitalWrite(ledPin, HIGH);
            SERIAL_PORT_XBEE.print("21,200,1;");
          } else {
            SERIAL_PORT_XBEE.print("21,501,Unknown Command:'" + data + "';");
          }
          break;
        default:
          SERIAL_PORT_XBEE.print("501;");
          break;
      }
      // clear the string:
      transmissionBuffer = "";
    }
  }
}

