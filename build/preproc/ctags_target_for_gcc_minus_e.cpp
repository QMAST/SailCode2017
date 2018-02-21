# 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
# 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
/*

  QMAST_Main

  Arduino code for the QMAST 2017-2018 Boat System



  Communicates with all of the sensors, servos, the RC, XBee and Raspberry Pi.



  Created in December 2017, QMAST

*/
# 10 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino"
# 11 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\QMAST_Main.ino" 2

// Variables related to hearbeat and connection status
unsigned long lastHeartbeat = 0;
unsigned long rpiLastResponse = 0;
unsigned long rpiLastQuery = 0;
unsigned long xbeeLastResponse = 0;
unsigned long xbeeLastQuery = 0;

int mode = 1; // Mega device mode (0 = Error/Disabled, 1 = Remote Control (Default), 2 = Autopiloted by RPi)

void setup() {
  // Initialize hardware serial ports
  Serial.begin(57600);
  Serial1.begin(57600);
  Serial2.begin(9600);

  ;

  initLink(); // Initialize communication between XBee + RPi
  initServos(); // Initialize servos
  initSensors(); // Initialize sensors
  initRC(); // Initialize RC

  delay(100);

  ;
  sendTransmission(1, "01", 1);
  sendTransmission(2, "01", 1);
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
    ;
    sendTransmission(2, "00", String(mode));
    sendTransmission(1, "00", String(mode));
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
      sendTransmission(1, "00", "?");
      sendTransmission(2, "09", 1); // Notify XBee that RPi is offline
      rpiLastQuery = currentMillis;
    }
    if(rpiLastResponse != 0 && currentMillis - rpiLastResponse >= 40000){
        setAutopilot(false); // Disable autopilot and enable RC if RPi fails
    }
  } else if (currentMillis - rpiLastQuery >= 20000 && currentMillis - rpiLastQuery >= 5000) {
    // If the RPi responded 20 seconds ago, send another query transmission to check if it is still alive
    sendTransmission(1, "00", "?");
    rpiLastQuery = currentMillis;
  }

  // Check if the XBee is connected/alive
  // One query period for the XBee is 10 seconds (longer because link unstable)
  if (xbeeLastResponse == 0 || ((currentMillis - xbeeLastResponse)>0?(currentMillis - xbeeLastResponse):-(currentMillis - xbeeLastResponse)) >= 20000) {
    // If the XBee has never responded or if it's last response was over 20 seconds ago (two query periods)
    // Send query transmissions every 5 seconds
    if (((currentMillis - xbeeLastQuery)>0?(currentMillis - xbeeLastQuery):-(currentMillis - xbeeLastQuery)) >= 5000) {
      sendTransmission(2, "00", "?");
      xbeeLastQuery = currentMillis;
    }
  } else if (((currentMillis - xbeeLastResponse)>0?(currentMillis - xbeeLastResponse):-(currentMillis - xbeeLastResponse)) >= 10000 && ((currentMillis - xbeeLastQuery)>0?(currentMillis - xbeeLastQuery):-(currentMillis - xbeeLastQuery)) >= 5000) {
    // If the XBee responded 10 seconds ago, send another query transmission to check if it is still alive
    sendTransmission(2, "00", "?");
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
  if (port == 2) {
    xbeeLastResponse = millis();
    xbeeLastQuery = millis();
  }
  if (port == 1) {
    rpiLastResponse = millis();
    rpiLastQuery = millis();
  }
}
# 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino"
/*

  Link

  Code that supports the communication ("cross-talk") between the XBee, Mega and Raspberry Pi.

  Follows format defined in "communication_standards.md"



  Created in December 2017, QMAST

*/
# 8 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino"
// TODO: Use char* for serial input and comparison with codes saved in PROGMEM to save RAM
// TODO: Implement 02 - Blink LED Strip
// TODO: Implement servo movement codes

# 13 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Link.ino" 2
String xbeeInputBuffer = ""; // a string to hold incoming data
String rpiInputBuffer = ""; // a string to hold incoming data
// int calibrationStage;

// Operations that only need to be performed once, such as reserving variable space
void initLink() {
  xbeeInputBuffer.reserve(200); // reserve 200 bytes for the XBee input buffer:
  rpiInputBuffer.reserve(200); // reserve 200 bytes for the RPi input buffer:
}

//
void sendTransmission(int port, String code, String message) {
  if (port == 1) {
    Serial2.print(code);
    Serial2.print(message);
    Serial2.println((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = (";"); &__c[0];})))));
  } else if (port == 2) {
    Serial1.print(code);
    Serial1.print(message);
    Serial1.print((reinterpret_cast<const __FlashStringHelper *>((__extension__({static const char __c[] __attribute__((__progmem__)) = (";"); &__c[0];})))));
  }
}

void sendTransmission(int port, String code, int message) {
  sendTransmission(port, code, String(message));
}

void executeXBeeTransmission(String code, String data) {
  bool error = false;
  if (code.equals("A0")) {
    // Relay autopilot request to RPi
    sendTransmission(1, code, data);
    if (data.equals("0")) setAutopilot(false); // Exit autopilot without waiting for RPi response
  } else if (code.equals("A1") || code.equals("A2") || code.equals("A3")) {
    // Messages sent by the XBee to be relayed to RPi transparently
    sendTransmission(1, code, data);
  } else {
    executeCommonTransmission(2, code, data);
  }
}

void executeRPiTransmission(String code, String data) {
  bool error = false;
  if (code.equals("A0")) {
    if (data.equals("1")) setAutopilot(true); // Enter autopilot
    if (data.equals("0")) setAutopilot(false); // Exit autopilot
  } else if (code.equals("05") || code.equals("07") || code.equals("A1") || code.equals("A2") || code.equals("A3") || code.equals("A8") || code.equals("A9")) {
    // Messages sent by the RPi to be relayed to XBee transparently
    sendTransmission(2, code, data);
  } else {
    executeCommonTransmission(1, code, data);
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
  while (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();
    // add it to the inputString:
    xbeeInputBuffer += inChar;
    // if the incoming character is a ";" do something about it:
    if (inChar == ';') {

      String code = xbeeInputBuffer.substring(0, 2);
      String data = xbeeInputBuffer.substring(2, xbeeInputBuffer.indexOf(";"));
      // Print parsed XBee input
      ;
      ;
      ;
      ;

      executeXBeeTransmission(code, data);
      xbeeInputBuffer = "";
    }
  }
  while (Serial2.available()) {
    // get the new byte:
    char inChar = (char)Serial2.read();
    // add it to the inputString:
    rpiInputBuffer += inChar;
    // if the incoming character is a ";" do something about it:
    if (inChar == ';') {
      ;
      ;
      String code = rpiInputBuffer.substring(0, 2);
      String data = rpiInputBuffer.substring(2, rpiInputBuffer.indexOf(";"));
      executeRPiTransmission(code, data);
      rpiInputBuffer = "";
    }
  }
}
# 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\RC.ino"
/*

  RC

  Code managing remote control of rudder(s) and winch



  Created in January 2018, QMAST

*/
# 7 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\RC.ino"
// TODO: Determine which RC channels to use
// TODO: Calibrate RC high/low pulses
# 10 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\RC.ino" 2

unsigned long lastRCMillis;

int oldWinchPos = 0; // variable to store previous winch position for exponential smoothing
int oldRudderPos = 0; // variable to store previous rudder position for exponential smoothing

bool rcEnabled = true;

void initRC() {
  pinMode(23 /* Adjust this so the left/right motion of the right stick corresponds with rudder movement*/, 0x0);
  pinMode(22 /* Adjust this so that the up/down motion of the left stick corresponds with the winch channel*/, 0x0);
}

void setRCEnabled(bool state) {
  rcEnabled = state;
}

void updateRCWinch() {
  int winchPos = pulseIn(22 /* Adjust this so that the up/down motion of the left stick corresponds with the winch channel*/, 0x1, 50000 /* Time (micros) to wait for pulses to begin from the RC receiver, recommend at least 20000*/); // Get winch pulse value
  if (winchPos > 1000) {
    winchPos = smooth(winchPos, oldWinchPos, (millis() - lastRCMillis)); // Perform exponential smoothing to reduce twitching
    oldWinchPos = winchPos; // Save current winch position for future exponential smoothing
    winchPos = ((winchPos)<(1100)?(1100):((winchPos)>(1880)?(1880):(winchPos))); //Trim bottom and upper end
    winchPos = map(winchPos,1880,1100,180,0); // Map the winch pulse value to a number between 0-180 for the servo library
    moveWinch(winchPos); // Move the winch
  }else{
    ;
  }
}

void updateRCRudders() {
  const int RudderMiddle = 0.5 * 1890 + 0.5 * 1093; // Calculate the centre value for the pulses, used to catch dead-band signals later
  int rudderPos = pulseIn(23 /* Adjust this so the left/right motion of the right stick corresponds with rudder movement*/, 0x1, 50000 /* Time (micros) to wait for pulses to begin from the RC receiver, recommend at least 20000*/); // Get rudder pulse value
  if (rudderPos != 0) {
    rudderPos = smooth(rudderPos, oldRudderPos, (millis() - lastRCMillis)); // Perform exponential smoothing to reduce twitching
    rudderPos = ((rudderPos)<(1093)?(1093):((rudderPos)>(1890)?(1890):(rudderPos))); //Trim bottom and upper end
    if (rudderPos <= (RudderMiddle + 30 / 2) && rudderPos >= (RudderMiddle - 30 / 2)) rudderPos = RudderMiddle; // Catch dead-band signals    
    rudderPos = map(rudderPos,1880,1100,180,0); // Map the rudder pulse value to a number between 0-180 for the servo library
    moveRudder(rudderPos); // Move the rudder
    oldRudderPos = rudderPos; // Save current rudder position for future exponential smoothing
  }else{
    ;
  }
}

void checkRC() {
  // Wrapper function to improve code readability in the main loop and also rate limit RC updating
  if (millis() - lastRCMillis >= 50 /* Minimum time (millis) between checking RC input*/ && rcEnabled) {
    updateRCWinch();
    updateRCRudders();
    lastRCMillis = millis();
  }
}

float smooth(float cur, float prev, float chng) {
  // Function that performs exponential smoothing given the current value, previous value, the time change and the time constant
  float w = ((1.)<(chng / 250 /*Time (millis) over which to perform exponential smoothing (tau value)*/)?(1.):(chng / 250 /*Time (millis) over which to perform exponential smoothing (tau value)*/));
  float w1 = 1.0 - w;
  return (w1 * prev + w * cur);
}
# 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino"
/*

  Sensors

  Handles updating and parsing of sensor inputs



  Created in January 2018, QMAST

*/
# 7 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino"
// TODO: Test GPS output format
// TODO: Implement Pixy and LIDAR

# 11 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino" 2


// GPS definitions
# 15 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino" 2
Adafruit_GPS GPS(&Serial3);
// Turns off echoing of GPS to Serial1


// Compass definitions
# 21 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Sensors.ino" 2


unsigned char high_byte, low_byte, angle8;
char pitch, roll;
unsigned int angle16;

// Wind vane calibration variables



unsigned long lastSensorMillis; // Last time sensor data was parsed

// Storage for sensor data and interval of data transmission
String sensorCodes[] = {"GP", "CP", "TM", "WV", "PX", "LD"};

String sensorStates[6];
int sensorTransIntervalXBee[6];
int sensorTransIntervalRPi[6];
unsigned long sensorLastTransXBee[6];
unsigned long sensorLastTransRPi[6];

void initSensors() {
  // Set default sensor transmission intervals and sensor states
  for (int i = 0; i < 6; i++) {
    sensorTransIntervalXBee[i] = 3000 /* Default interval the Mega transmits sensor data*/;
    sensorTransIntervalRPi[i] = 3000 /* Default interval the Mega transmits sensor data*/;
    sensorStates[i] = "";
  }

  //Initialize Compass
  Wire.begin(); // Initiate the Wire library

  // Initialize GPS
  GPS.begin(9600);
  // Turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28");
  GPS.sendCommand("$PMTK220,1000*1F"); // 1 Hz update rate, don't go above 1Hz to prevent overload
}

void checkSensors() {
  // Update sensor states if new information has arrived
  // Only runs once every 40 milliseconds, or about 25 times a second
  if (millis() - lastSensorMillis >= 20 && rcEnabled) {
    // Update compass
    Wire.beginTransmission(0x60 /* Address of CMPS11 shifted right one bit for arduino wire library*/); //starts communication with CMPS11
    Wire.write(1 /* Register to read 8bit angle from*/); //Sends the register we wish to start reading from
    Wire.endTransmission();
    // Request 5 bytes from the CMPS11
    // this will give us the 8 bit bearing,
    // both bytes of the 16 bit bearing, pitch and roll
    Wire.requestFrom(0x60 /* Address of CMPS11 shifted right one bit for arduino wire library*/, 5);
    while (Wire.available() < 5); // Wait for all bytes to come back
    angle8 = Wire.read(); // Read back the 5 bytes
    high_byte = Wire.read();
    low_byte = Wire.read();
    pitch = Wire.read();
    roll = Wire.read();
    angle16 = high_byte; // Calculate 16 bit angle
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
    int angle = map(analogRead(1), 0, 1023, 0, 360);
    setSensor("WV", String(angle));

    lastSensorMillis = millis();
  }

  // GPS data is can come in at any time (~1Hz) so keep looking for it to prevent dropping a message
  GPS.read();
  if (GPS.newNMEAreceived()) GPS.parse(GPS.lastNMEA());
}

void setSensor(String code, String data) {
  // Used to update data stored for a sensor
  for (int i = 0; i < 6; i++) {
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
  for (int i = 0; i < 6; i++) {
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
  for (int i = 0; i < 6; i++) {
    if (abs(currentMillis - sensorLastTransXBee[i]) >= sensorTransIntervalXBee[i] && sensorTransIntervalXBee[i] != 0 && !sensorStates[i].equals("")) {
      sendTransmission(2, sensorCodes[i], sensorStates[i]);
      sensorLastTransXBee[i] = currentMillis;
    }
    if (abs(currentMillis - sensorLastTransRPi[i]) >= sensorTransIntervalRPi[i] && sensorTransIntervalRPi[i] != 0 && !sensorStates[i].equals("")) {
      sendTransmission(1, sensorCodes[i], sensorStates[i]);
      sensorLastTransRPi[i] = currentMillis;
      ;
      ;
      ;
      ;
    }
  }
  ;
}

void setSensorTransInterval(int port, String code, int interval) {
  // Called when the XBee/RPi requests to be updated with sensor data at a different frequency
  for (int i = 0; i < 6; i++) {
    // Cycle through all of the sensor codes to find the one needing updating
    if (code.equals(sensorCodes[i])) {
      // Update the sensor transmission interval for the device sending the request
      if (port == 2) {
        if(interval !=0 && interval < 250) interval = 250; // After testing, the GUI does not respond to absurdly fast updates
        sensorTransIntervalXBee[i] = interval;
      }
      if (port == 1) sensorTransIntervalRPi[i] = interval;
      break;
    }
  }
}
# 1 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Servos.ino"
/*

  Servos

  Code controlling the movement of the rudder, winch and any other servos



  Created in January 2018, QMAST

*/
# 8 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Servos.ino"
# 9 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Servos.ino" 2
# 10 "c:\\Users\\angel\\Documents\\GitHub\\SailCode2017\\Arduino\\QMAST_Main\\Servos.ino" 2

Servo servoRudder;
Servo servoWinch;

int curWinchPos; // Store the current winch position

void initServos(){
  servoRudder.attach(8);
  servoWinch.attach(7);
}

void moveRudder(int pos){
  // Move the rudder between 0-180
  pos = ((pos)<(0)?(0):((pos)>(180)?(180):(pos)));
  servoRudder.write(pos);
}

void moveWinch(int pos){
  // Move the winch between 0-180 (fully in vs fully out)
  pos = ((pos)<(0)?(0):((pos)>(180)?(180):(pos)));
  if(abs(pos-curWinchPos) > 2){ // Don't allow winch to twitch with RC noise
    servoWinch.write(pos);
    curWinchPos = pos;
  }
}
