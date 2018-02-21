#include <Arduino.h>
#line 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
#line 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
/*
  QMAST_Main
  Arduino code for the QMAST 2017-2018 Boat System

  Communicates with all of the sensors, servos, the RC, XBee and Raspberry Pi.

  Created in December 2017, QMAST
*/

#include "pins.h"

// Variables related to hearbeat and connection status
unsigned long lastHeartbeat = 0;
unsigned long rpiLastResponse = 0;
unsigned long rpiLastQuery = 0;
unsigned long xbeeLastResponse = 0;
unsigned long xbeeLastQuery = 0;

int mode = 1; // Mega device mode (0 = Error/Disabled, 1 = Remote Control (Default), 2 = Autopiloted by RPi)

#line 21 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
void setup();
#line 41 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
void loop();
#line 52 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
void heartbeat();
#line 102 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
void setAutopilot(bool state);
#line 111 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
void setLastResponse(int port);
#line 18 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino"
void initLink();
#line 24 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino"
void sendTransmission(int port, String code, String message);
#line 36 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino"
void sendTransmission(int port, String code, int message);
#line 40 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino"
void executeXBeeTransmission(String code, String data);
#line 54 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino"
void executeRPiTransmission(String code, String data);
#line 67 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino"
void executeCommonTransmission(int port, String code, String data);
#line 91 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino"
void checkLink();
#line 18 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\RC.ino"
void initRC();
#line 23 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\RC.ino"
void setRCEnabled(bool state);
#line 27 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\RC.ino"
void updateRCWinch();
#line 40 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\RC.ino"
void updateRCRudders();
#line 55 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\RC.ino"
void checkRC();
#line 64 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\RC.ino"
float smooth(float cur, float prev, float chng);
#line 42 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino"
void initSensors();
#line 60 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino"
void checkSensors();
#line 107 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino"
void setSensor(String code, String data);
#line 119 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino"
String getSensor(String code, String data);
#line 132 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino"
void sendSensors();
#line 153 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino"
void setSensorTransInterval(int port, String code, int interval);
#line 16 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Servos.ino"
void initServos();
#line 21 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Servos.ino"
void moveRudder(int pos);
#line 27 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Servos.ino"
void moveWinch(int pos);
#line 21 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
void setup() {
  // Initialize hardware serial ports
  SERIAL_PORT_CONSOLE.begin(SERIAL_BAUD_CONSOLE);
  SERIAL_PORT_XBEE.begin(SERIAL_BAUD_XBEE);
  SERIAL_PORT_RPI.begin(SERIAL_BAUD_RPI);

  DEBUG_PRINTLN(F("Powered on"));

  initLink(); // Initialize communication between XBee + RPi
  initServos(); // Initialize servos
  initSensors(); // Initialize sensors
  initRC(); // Initialize RC

  delay(100);

  DEBUG_PRINTLN(F("Setup complete"));
  sendTransmission(PORT_RPI, "01", 1);
  sendTransmission(PORT_XBEE, "01", 1);
}

void loop() {
  // All code in loop() should respect protothreading and execute without locking the device into a single function for too long
  // This is because the Mega must remain responsive to sensor input and information requests
  // Slow function = slow response/missed messages = crashed boat
  checkLink(); // Check if any transmissions have been recieved from the XBee + RPi
  checkSensors(); // Update the stored sensor states if new info is available
  sendSensors(); // Send sensor data to XBee + RPi if appropriate
  heartbeat(); // Send Mega "heartbeat" indicating mode and queries if RPi + XBee are connected
  checkRC(); // Update the winch and rudder position if RC is enabled
}

void heartbeat() {
  // Send Mega "heartbeat" indicating mode and queries if RPi + XBee are connected, if needed
  unsigned long currentMillis = millis();
  //Send Mega "heartbeat" every 3 seconds
  if (currentMillis - lastHeartbeat > 3000) {
    DEBUG_PRINTLN(F("Heartbeat"));
    sendTransmission(PORT_XBEE, "00", String(mode));
    sendTransmission(PORT_RPI, "00", String(mode));
    lastHeartbeat = millis();
  }

  // Send query transmissions every 5 seconds if the device has never responded or if it is disconnected/dead
  // Otherwise, send a query transmission one query period from the last response time to check if the device is still alive
  // If either device does not respond within 2 query periods, it is disconnected/dead

  // Check if the RPi is connected/alive
  // One query period for the RPi is 20 seconds
  if (rpiLastResponse == 0 || currentMillis - rpiLastResponse >= 40000) {
    // If the RPi has never responded or if it's last response was over 40 seconds ago (two query periods)
    // Send query transmissions every 5 seconds
    if (currentMillis - rpiLastQuery >= 5000) {
      sendTransmission(PORT_RPI, "00", "?");
      sendTransmission(PORT_XBEE, "09", 1); // Notify XBee that RPi is offline
      rpiLastQuery = currentMillis;
    }
    if(rpiLastResponse != 0 && currentMillis - rpiLastResponse >= 40000){
        setAutopilot(false); // Disable autopilot and enable RC if RPi fails
    }
  } else if (currentMillis - rpiLastQuery >= 20000 && currentMillis - rpiLastQuery >= 5000) {
    // If the RPi responded 20 seconds ago, send another query transmission to check if it is still alive
    sendTransmission(PORT_RPI, "00", "?");
    rpiLastQuery = currentMillis;
  }

  // Check if the XBee is connected/alive
  // One query period for the XBee is 10 seconds (longer because link unstable)
  if (xbeeLastResponse == 0 || abs(currentMillis - xbeeLastResponse) >= 20000) {
    // If the XBee has never responded or if it's last response was over 20 seconds ago (two query periods)
    // Send query transmissions every 5 seconds
    if (abs(currentMillis - xbeeLastQuery) >= 5000) {
      sendTransmission(PORT_XBEE, "00", "?");
      xbeeLastQuery = currentMillis;
    }
  } else if (abs(currentMillis - xbeeLastResponse) >= 10000 && abs(currentMillis - xbeeLastQuery) >= 5000) {
    // If the XBee responded 10 seconds ago, send another query transmission to check if it is still alive
    sendTransmission(PORT_XBEE, "00", "?");
    xbeeLastQuery = currentMillis;
  }
}

void setAutopilot(bool state) {
  if (state) {
    mode = 2;
  } else {
    mode = 1;
  }
  setRCEnabled(!state);
}

void setLastResponse(int port) {
  // Called when a device responds to a alive/connected query transmission
  if (port == PORT_XBEE) {
    xbeeLastResponse = millis();
    xbeeLastQuery = millis();
  }
  if (port == PORT_RPI) {
    rpiLastResponse = millis();
    rpiLastQuery = millis();
  }
}


#line 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino"
/*
  Link
  Code that supports the communication ("cross-talk") between the XBee, Mega and Raspberry Pi.
  Follows format defined in "communication_standards.md"

  Created in December 2017, QMAST
*/
// TODO: Use char* for serial input and comparison with codes saved in PROGMEM to save RAM
// TODO: Implement 02 - Blink LED Strip
// TODO: Implement servo movement codes

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
  }else if(code.equals("SR")){
    moveRudder(data.toInt());
  }else if(code.equals("SW")){
    moveWinch(data.toInt());
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
      
      String code = xbeeInputBuffer.substring(0, 2);
      String data = xbeeInputBuffer.substring(2, xbeeInputBuffer.indexOf(";"));
      // Print parsed XBee input
      DEBUG_PRINT("Recieved (Xbee) Code: ");
      DEBUG_PRINT(code);
      DEBUG_PRINT(", Data: ");
      DEBUG_PRINTLN(data);

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
      DEBUG_PRINT(F("Recieved (RPi): "));
      DEBUG_PRINTLN(rpiInputBuffer);
      String code = rpiInputBuffer.substring(0, 2);
      String data = rpiInputBuffer.substring(2, rpiInputBuffer.indexOf(";"));
      executeRPiTransmission(code, data);
      rpiInputBuffer = "";
    }
  }
}

#line 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\RC.ino"
/*
  RC
  Code managing remote control of rudder(s) and winch

  Created in January 2018, QMAST
*/
// TODO: Determine which RC channels to use
// TODO: Calibrate RC high/low pulses
#include "pins.h"

unsigned long lastRCMillis;

int oldWinchPos = 0; // variable to store previous winch position for exponential smoothing
int oldRudderPos = 0; // variable to store previous rudder position for exponential smoothing

bool rcEnabled = true;

void initRC() {
  pinMode(CHANNEL_RUDDERS, INPUT);
  pinMode(CHANNEL_WINCH, INPUT);
}

void setRCEnabled(bool state) {
  rcEnabled = state;
}

void updateRCWinch() {
  int winchPos = pulseIn(CHANNEL_WINCH, HIGH, RC_STD_TIMEOUT); // Get winch pulse value
  if (winchPos > 1000) {
    winchPos = smooth(winchPos, oldWinchPos, (millis() - lastRCMillis)); // Perform exponential smoothing to reduce twitching
    oldWinchPos = winchPos; // Save current winch position for future exponential smoothing
    winchPos = constrain(winchPos, WINCH_PULSE_LOW, WINCH_PULSE_HIGH); //Trim bottom and upper end
    winchPos = map(winchPos,WINCH_PULSE_HIGH,WINCH_PULSE_LOW,180,0); // Map the winch pulse value to a number between 0-180 for the servo library
    moveWinch(winchPos); // Move the winch
  }else{
    DEBUG_PRINTLN("RC Offline. No winch data.");
  }
}

void updateRCRudders() {
  const int RudderMiddle = 0.5 * RUDDER_PULSE_HIGH + 0.5 * RUDDER_PULSE_LOW; // Calculate the centre value for the pulses, used to catch dead-band signals later
  int rudderPos = pulseIn(CHANNEL_RUDDERS, HIGH, RC_STD_TIMEOUT); // Get rudder pulse value
  if (rudderPos != 0) {
    rudderPos = smooth(rudderPos, oldRudderPos, (millis() - lastRCMillis)); // Perform exponential smoothing to reduce twitching
    rudderPos = constrain(rudderPos, RUDDER_PULSE_LOW, RUDDER_PULSE_HIGH); //Trim bottom and upper end
    if (rudderPos <= (RudderMiddle + RUDDER_DEAD_WIDTH / 2) && rudderPos >= (RudderMiddle - RUDDER_DEAD_WIDTH / 2)) rudderPos = RudderMiddle; // Catch dead-band signals    
    rudderPos = map(rudderPos,WINCH_PULSE_HIGH,WINCH_PULSE_LOW,180,0); // Map the rudder pulse value to a number between 0-180 for the servo library
    moveRudder(rudderPos); // Move the rudder
    oldRudderPos = rudderPos; // Save current rudder position for future exponential smoothing
  }else{
    DEBUG_PRINTLN(F("RC Offline. No rudder data."));
  }
}

void checkRC() {
  // Wrapper function to improve code readability in the main loop and also rate limit RC updating
  if (millis() - lastRCMillis >= RC_MIN_DELAY && rcEnabled) {
    updateRCWinch();
    updateRCRudders();
    lastRCMillis = millis();
  }
}

float smooth(float cur, float prev, float chng) {
  // Function that performs exponential smoothing given the current value, previous value, the time change and the time constant
  float w = min(1., chng / RC_SMOOTHING_CONS);
  float w1 = 1.0 - w;
  return (w1 * prev + w * cur);
}
#line 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino"
/*
  Sensors
  Handles updating and parsing of sensor inputs

  Created in January 2018, QMAST
*/
// TODO: Test GPS output format
// TODO: Implement Pixy and LIDAR

#include "pins.h"
#define DEFAULT_SENSOR_TRANSMILLIS 3000 // Default interval the Mega transmits sensor data

// GPS definitions
#include <Adafruit_GPS.h>
Adafruit_GPS GPS(&SERIAL_PORT_GPS);
// Turns off echoing of GPS to Serial1
#define GPSECHO false

// Compass definitions
#include <Wire.h>
#define CMPS11_ADDRESS 0x60  // Address of CMPS11 shifted right one bit for arduino wire library
#define ANGLE_8  1           // Register to read 8bit angle from
unsigned char high_byte, low_byte, angle8;
char pitch, roll;
unsigned int angle16;

// Wind vane calibration variables
#define WINDVANE_LOW 0
#define WINDVANE_HIGH 1023

unsigned long lastSensorMillis; // Last time sensor data was parsed

// Storage for sensor data and interval of data transmission
String sensorCodes[] = {"GP", "CP", "TM", "WV", "PX", "LD"};
#define NUMBER_OF_CODES 6
String sensorStates[NUMBER_OF_CODES];
int sensorTransIntervalXBee[NUMBER_OF_CODES];
int sensorTransIntervalRPi[NUMBER_OF_CODES];
unsigned long sensorLastTransXBee[NUMBER_OF_CODES];
unsigned long sensorLastTransRPi[NUMBER_OF_CODES];

void initSensors() {
  // Set default sensor transmission intervals and sensor states
  for (int i = 0; i < NUMBER_OF_CODES; i++) {
    sensorTransIntervalXBee[i] = DEFAULT_SENSOR_TRANSMILLIS;
    sensorTransIntervalRPi[i] = DEFAULT_SENSOR_TRANSMILLIS;
    sensorStates[i] = "";
  }

  //Initialize Compass
  Wire.begin(); // Initiate the Wire library

  // Initialize GPS
  GPS.begin(9600);
  // Turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate, don't go above 1Hz to prevent overload
}

void checkSensors() {  
  // Update sensor states if new information has arrived
  // Only runs once every 40 milliseconds, or about 25 times a second
  if (millis() - lastSensorMillis >= 20 && rcEnabled) {
    // Update compass
    Wire.beginTransmission(CMPS11_ADDRESS);  //starts communication with CMPS11
    Wire.write(ANGLE_8);                     //Sends the register we wish to start reading from
    Wire.endTransmission();
    // Request 5 bytes from the CMPS11
    // this will give us the 8 bit bearing,
    // both bytes of the 16 bit bearing, pitch and roll
    Wire.requestFrom(CMPS11_ADDRESS, 5);
    while (Wire.available() < 5);       // Wait for all bytes to come back
    angle8 = Wire.read();               // Read back the 5 bytes
    high_byte = Wire.read();
    low_byte = Wire.read();
    pitch = Wire.read();
    roll = Wire.read();
    angle16 = high_byte;                 // Calculate 16 bit angle
    angle16 <<= 8;
    angle16 += low_byte;
    setSensor("CP", String(angle16 / 10));

   // Update GPS
   if (GPS.fix) {
      String location = "";
      if(GPS.lat == 'S') location = "-";
      location = location + String(GPS.latitude,4) + ",";
      if(GPS.lon == 'E') location = location + "-";
      location = location + String(GPS.longitude,4) + ",";
      setSensor("GP", location);
    }else{
      setSensor("GP", "0");
    }
  
    // Update wind vane
    int angle = map(analogRead(APIN_WINDVANE), WINDVANE_LOW, WINDVANE_HIGH, 0, 360);
    setSensor("WV", String(angle));

    lastSensorMillis = millis();
  }

  // GPS data is can come in at any time (~1Hz) so keep looking for it to prevent dropping a message
  GPS.read();
  if (GPS.newNMEAreceived()) GPS.parse(GPS.lastNMEA());
}

void setSensor(String code, String data) {
  // Used to update data stored for a sensor
  for (int i = 0; i < NUMBER_OF_CODES; i++) {
    // Cycle through all of the sensor codes to find the one needing updating
    if (code.equals(sensorCodes[i])) {
      // Update the sensor storage
      sensorStates[i] = data;
      break;
    }
  }
}

String getSensor(String code, String data) {
  // Takes the sensor code and matches it with the appropriate data store
  // Used to update data stored for a sensor
  for (int i = 0; i < NUMBER_OF_CODES; i++) {
    // Cycle through all of the sensor codes to find the one requested
    if (code.equals(sensorCodes[i])) {
      // Update the sensor storage
      return sensorStates[i];
    }
  }
  return "";
}

void sendSensors() {
  // Check if the transmission interval/delay has passed for each sensor for the XBee + RPi
  // If the interval has passed, send the updated data
  unsigned long currentMillis = millis();
  for (int i = 0; i < NUMBER_OF_CODES; i++) {
    if (abs(currentMillis - sensorLastTransXBee[i]) >= sensorTransIntervalXBee[i] && sensorTransIntervalXBee[i] != 0 && !sensorStates[i].equals("")) {
      sendTransmission(PORT_XBEE, sensorCodes[i], sensorStates[i]);
      sensorLastTransXBee[i] = currentMillis;
    }
    if (abs(currentMillis - sensorLastTransRPi[i]) >= sensorTransIntervalRPi[i] && sensorTransIntervalRPi[i] != 0 && !sensorStates[i].equals("")) {
      sendTransmission(PORT_RPI, sensorCodes[i], sensorStates[i]);
      sensorLastTransRPi[i] = currentMillis;
      DEBUG_PRINT(sensorCodes[i]);
      DEBUG_PRINT(F(": "));
      DEBUG_PRINT(sensorStates[i]);
      DEBUG_PRINT(F(" "));
    }
  }
  DEBUG_PRINTLN("");
}

void setSensorTransInterval(int port, String code, int interval) {
  // Called when the XBee/RPi requests to be updated with sensor data at a different frequency
  for (int i = 0; i < NUMBER_OF_CODES; i++) {
    // Cycle through all of the sensor codes to find the one needing updating
    if (code.equals(sensorCodes[i])) {
      // Update the sensor transmission interval for the device sending the request
      if (port == PORT_XBEE) {
        if(interval !=0 && interval < 250) interval = 250; // After testing, the GUI does not respond to absurdly fast updates
        sensorTransIntervalXBee[i] = interval;
      }
      if (port == PORT_RPI) sensorTransIntervalRPi[i] = interval;
      break;
    }
  }
}

#line 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Servos.ino"
/*
  Servos
  Code controlling the movement of the rudder, winch and any other servos

  Created in January 2018, QMAST
*/

#include "pins.h"
#include <Servo.h>

Servo servoRudder;
Servo servoWinch;

int curWinchPos; // Store the current winch position

void initServos(){
  servoRudder.attach(PIN_SERVO_1);
  servoWinch.attach(PIN_SERVO_WINCH);
}

void moveRudder(int pos){
  // Move the rudder between 0-180
  pos = constrain(pos, 0, 180);
  servoRudder.write(pos);
}

void moveWinch(int pos){
  // Move the winch between 0-180 (fully in vs fully out)
  pos = constrain(pos, 0, 180);
  if(abs(pos-curWinchPos) > 2){ // Don't allow winch to twitch with RC noise
    servoWinch.write(pos);
    curWinchPos = pos;
  }
}


