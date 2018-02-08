#include "pins.h"
String transmissionBuffer = "";         // a string to hold incoming data
int calibrationStage;

void initComms() {
  transmissionBuffer.reserve(200); // reserve 200 bytes for the inputString:
}

void sendTransmission(int code, int resp_code, String message) {
  // Sends data over xbee in the defined format
  sendTransmissionStart(code, resp_code);
  SERIAL_PORT_XBEE.print(message);
  sendTransmissionEnd();
}

void sendTransmissionStart(int code, int resp_code) {
  // Sends data over xbee in the defined format
  SERIAL_PORT_XBEE.print(code);
  SERIAL_PORT_XBEE.print(F(","));
  SERIAL_PORT_XBEE.print(resp_code);
  SERIAL_PORT_XBEE.print(F(","));
}
void sendTransmissionEnd() {
  // Sends data over xbee in the defined format
  SERIAL_PORT_XBEE.print(F(";"));
}

void sendTransmissionAndPrint(int code, int resp_code, String message) {
  // Sends data over xbee and also prints to console
  SERIAL_PORT_CONSOLE.println(message);
  sendTransmission(code, resp_code, message);
}

void xBeeUpdateAndExecute() {
  // Checks for new bytes and executes commands if found
  while (SERIAL_PORT_XBEE.available()) {
    // get the new byte:
    char inChar = (char)SERIAL_PORT_XBEE.read();
    //Serial.print(inChar);
    // add it to the inputString:
    transmissionBuffer += inChar;
    // if the incoming character is a ";" do something about it:
    if (inChar == ';') {
      Serial.println(transmissionBuffer);
      int iCommand = transmissionBuffer.substring(0, 2).toInt();
      String data = transmissionBuffer.substring(3, transmissionBuffer.indexOf(";"));
      bool error = false;
      switch (iCommand) {
        case 20:
          {
            String workingdata = data;
            int indexToAlter = data.substring(0, workingdata.indexOf(" ")).toInt();
            workingdata = workingdata.substring(workingdata.indexOf(" ") + 1);
            int32_t add_lat;
            sscanf(workingdata.substring(0, workingdata.indexOf(" ")).c_str(), "%"SCNd32, &add_lat);

            workingdata = workingdata.substring(workingdata.indexOf(" ") + 1);
            int32_t add_lon;
            sscanf(workingdata.c_str(), "%"SCNd32, &add_lon);

            waypoint[indexToAlter].lat = add_lat;
            waypoint[indexToAlter].lon = add_lon;

            //sendTransmissionStart(10, 201);
            Serial.print(F("-> Added "));
            Serial.print(add_lat);
            Serial.print(F(", "));
            Serial.print(add_lon);
            Serial.print(F(" at "));
            Serial.println(indexToAlter);
            //sendTransmissionEnd();
          }
          break;
        case 21:
          // Change operating mode
          if (data == "0") {
            gaelforce = MODE_COMMAND_LINE;
          } else if (data == "1") {
            gaelforce = MODE_RC_CONTROL;
          } else if (data == "2") {
            gaelforce = MODE_AUTOSAIL;
          } else {
            error = true;
          }
          break;
        case 23:
          // Select start_wp_index waypoint to go to.
          start_wp_index = data.toInt();
          target_wp = start_wp_index;
          break;
        case 24:
          // Select finish_wp_index waypoint to go to.
          finish_wp_index = data.toInt();
          break;
        case 30:
          // Lock motor
          Serial.println("Locking winch");
          stopWinch();
          //motor_lock();
          break;
        case 31:
          // Unlock motor
          Serial.println("Unlocking winch");
          //motor_unlock();
          break;
        case 37:
          // Multistage remote control calibration
          if (data == "1") {
            // Intentionally blank, ping to check if device ready
            // On device, display "Set RC and press next"
          } else if (data == "2") {
            radio_controller.rsy.high = rc_get_raw_analog( radio_controller.rsy );
            radio_controller.lsy.high = rc_get_raw_analog( radio_controller.lsy );
            radio_controller.rsx.high = rc_get_raw_analog( radio_controller.rsx );
            radio_controller.lsx.high = rc_get_raw_analog( radio_controller.lsx );
            // On device, display "Set RC and press next"
          } else if (data == "3") {
            radio_controller.rsy.low = rc_get_raw_analog( radio_controller.rsy );
            radio_controller.lsy.low = rc_get_raw_analog( radio_controller.lsy );
            radio_controller.rsx.low = rc_get_raw_analog( radio_controller.rsx );
            radio_controller.lsx.low = rc_get_raw_analog( radio_controller.lsx );
            // Calibration complete
          } else {
            error = true;
          }
          break;
        case 40:
          autosail_type = data.toInt();
          break;
        case 56:
          if (data == "1") {
            // Intentionally blank, ping to check if device ready
            // On device, display "Set Winch and press next"
            //guardOff();
          } else if (data == "2") {
            //calibrateWinchIn();
            // Done
          } else {
            error = true;
          }
          break;
        case 62:
          // Flash success LED sequence
          ledSuccess();
          break;
        default:
          // Code not reconized, may not be implemented
          SERIAL_PORT_XBEE.print(F("501;"));
          transmissionBuffer = "";
          return;
          break;
      }

      // Send response (echo if successful)
      if (error) {
        sendTransmission(iCommand, 501, "Error with: " + data);
      } else {
        sendTransmission(iCommand, 200, data);
      }
      // clear the string:
      transmissionBuffer = "";
    }
  }
}
