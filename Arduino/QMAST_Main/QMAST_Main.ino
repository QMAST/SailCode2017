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

void setup() {
  // Initialize hardware serial ports
  SERIAL_PORT_CONSOLE.begin(SERIAL_BAUD_CONSOLE);
  SERIAL_PORT_XBEE.begin(SERIAL_BAUD_XBEE);
  SERIAL_PORT_RPI.begin(SERIAL_BAUD_RPI);

  SERIAL_PORT_CONSOLE.println("Powered on");

  initLink(); // Initialize communication between XBee + RPi
  initServos(); // Initialize servos
  initSensors(); // Initialize sensors
  initRC(); // Initialize RC

  delay(100);

  SERIAL_PORT_CONSOLE.println("Setup complete");
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
    Serial.println("Heartbeat");
    sendTransmission(PORT_XBEE, "00", String(mode));
    sendTransmission(PORT_RPI, "00", String(mode));
    lastHeartbeat = millis();
  }

  // Send query transmissions every 5 seconds if the device has never responded or if it is disconnected/dead
  // Otherwise, send a query transmission one query period from the last response time to check if the device is still alive
  // If either device does not repond within 2 query periods, it is disconnected/dead

  // Check if the RPi is connected/alive
  // One query period for the RPi is 20 seconds
  if (rpiLastResponse == 0 || currentMillis - rpiLastResponse >= 40000) {
    // If the RPi has never responded or if it's last response was over 40 seconds ago (two query periods)
    // Send query transmissions every 5 seconds
    if (currentMillis - rpiLastQuery >= 5000) {
      sendTransmission(PORT_RPI, "00", "?");
      sendTransmission(PORT_XBEE, "09", 1); // Notify XBee that RPi is offline
      setAutopilot(false); // Disable autopilot and enable RC if RPi fails
      rpiLastQuery = currentMillis;
    }
  } else if (currentMillis - rpiLastQuery >= 20000) {
    // If the RPi reponded 20 seconds ago, send another query transmission to check if it is still alive
    sendTransmission(PORT_RPI, "00", "?");
    rpiLastQuery = currentMillis;
  }

  // Check if the XBee is connected/alive
  // One query period for the XBee is 10 seconds (longer because link unstable)
  if (xbeeLastResponse == 0 || currentMillis - xbeeLastResponse >= 20000) {
    // If the XBee has never responded or if it's last response was over 20 seconds ago (two query periods)
    // Send query transmissions every 5 seconds
    if (currentMillis - xbeeLastQuery >= 5000) {
      sendTransmission(PORT_XBEE, "00", "?");
      xbeeLastQuery = currentMillis;
    }
  } else if (currentMillis - xbeeLastResponse >= 10000) {
    // If the XBee reponded 10 seconds ago, send another query transmission to check if it is still alive
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
  if (port == SERIAL_PORT_XBEE) xbeeLastResponse = xbeeLastQuery = millis();
  if (port == SERIAL_PORT_RPI) rpiLastResponse = rpiLastQuery = millis();
}

