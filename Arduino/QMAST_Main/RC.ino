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
  if (winchPos != 0) {
    winchPos = smooth(winchPos, oldWinchPos, (millis() - lastRCMillis)); // Perform exponential smoothing to reduce twitching
    winchPos = constrain(winchPos, WINCH_PULSE_LOW, WINCH_PULSE_HIGH); //Trim bottom and upper end
    winchPos = map(winchPos,WINCH_PULSE_HIGH,WINCH_PULSE_LOW,180,0); // Map the winch pulse value to a number between 0-180 for the servo library
    moveWinch(winchPos); // Move the winch
    oldWinchPos = winchPos; // Save current winch position for future exponential smoothing
  }else{
    DEBUG_PRINTLN(F("RC Offline. No winch data."));
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