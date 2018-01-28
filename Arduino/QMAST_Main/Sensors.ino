/*
  Sensors
  Handles updating and parsing of sensor inputs

  Created in January 2018, QMAST
*/
// TODO: Calibrate wind vane
// TODO: Calibrate RC
// TODO: Implement GPS
// TODO: Implement Pixy and LIDAR

#include "pins.h"

// Compass related code
#include <Wire.h>
#define CMPS11_ADDRESS 0x60  // Address of CMPS11 shifted right one bit for arduino wire library
#define ANGLE_8  1           // Register to read 8bit angle from
unsigned char high_byte, low_byte, angle8;
char pitch, roll;
unsigned int angle16;

// Wind vane calibration variables
#define WINDVANE_LOW 0
#define WINDVANE_HIGH 1023

unsigned long lastSensorMillis;

// Storage for sensor data and interval of data transmission
String sensorCodes[] = {"GP", "CP", "TM", "WV", "PX", "LD"};
String sensorStates[sizeof(sensorCodes)];
int sensorTransIntervalXBee[sizeof(sensorCodes)];
int sensorTransIntervalRPi[sizeof(sensorCodes)];
unsigned long sensorLastTransXBee[sizeof(sensorCodes)];
unsigned long sensorLastTransRPi[sizeof(sensorCodes)];

void initSensors() {
  // Set default sensor transmission intervals and sensor states
  for (int i = 0; i < sizeof(sensorCodes); i++) {
    sensorTransIntervalXBee[i] = DEFAULT_SENSOR_TRANSMILLIS;
    sensorTransIntervalRPi[i] = DEFAULT_SENSOR_TRANSMILLIS;
    sensorStates[i] = "";
  }

  Wire.begin(); // Initiate the Wire library

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

    // Update wind vane
    int angle = map(analogRead(APIN_WINDVANE), WINDVANE_LOW, WINDVANE_HIGH, 0, 360);
    setSensor("WV", String(angle));

    lastSensorMillis = millis();
  }
}

void setSensor(String code, String data) {
  // Used to update data stored for a sensor
  for (int i = 0; i < sizeof(sensorCodes); i++) {
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
  for (int i = 0; i < sizeof(sensorCodes); i++) {
    // Cycle through all of the sensor codes to find the one requested
    if (code.equals(sensorCodes[i])) {
      // Update the sensor storage
      return sensorStates[i];
    }
  }
  return "";
}

void sensorTrans() {
  // Check if the transmission interval/delay has passed for each sensor for the XBee + RPi
  // If the interval has passed, send the updated data
  unsigned long currentMillis = millis();
  for (int i = 0; i < sizeof(sensorCodes); i++) {
    if (abs(currentMillis - sensorLastTransXBee[i]) >= sensorTransIntervalXBee[i] && sensorTransIntervalXBee[i] != 0) {
      sendTransmission(SERIAL_PORT_XBEE, sensorCodes[i], sensorStates[i]);
      sensorLastTransXBee[i] = currentMillis;
    }
    if (abs(currentMillis - sensorLastTransRPi[i]) >= sensorTransIntervalRPi[i] && sensorTransIntervalRPi[i] != 0) {
      sendTransmission(SERIAL_PORT_RPI, sensorCodes[i], sensorStates[i]);
      sensorLastTransRPi[i] = currentMillis;
    }
  }
}

void setSensorTransInterval(int port, String code, int interval) {
  // Called when the XBee/RPi requests to be updated with sensor data at a different frequency
  for (int i = 0; i < sizeof(sensorCodes); i++) {
    // Cycle through all of the sensor codes to find the one needing updating
    if (code.equals(sensorCodes[i])) {
      // Update the sensor transmission interval for the device sending the request
      if (port == SERIAL_PORT_XBEE) sensorTransIntervalXBee[i] = interval;
      if (port == SERIAL_PORT_RPI) sensorTransIntervalRPi[i] = interval;
      break;
    }
  }
}
