/*
  RC
  Code managing remote control of rudder(s) and winch

  Created in January 2018, QMAST
*/
// TODO: Determine which RC channels to use
// TODO: Calibrate RC high/low pulses
#include "pins.h"

#define MIN_RC_DELAY 50 // Minimum time before updating in millis
#define CHANNEL_RUDDERS PIN_RC_CH1
#define CHANNEL_WINCH PIN_RC_CH2

#define RUDDER_PULSE_LOW 920
#define RUDDER_PULSE_HIGH 1640
#define RUDDER_DEAD_WIDTH 30

#define WINCH_PULSE_LOW 920
#define WINCH_PULSE_HIGH 1640

unsigned long lastRCMillis;

bool rcEnabled = true;

void initRC() {
  pinMode(CHANNEL_RUDDERS, INPUT);
  pinMode(CHANNEL_WINCH, INPUT);
}

void setRCEnabled(bool state) {
  rcEnabled = state;
}

void updateRCWinch() {
  int winchPos = pulseIn(CHANNEL_WINCH, HIGH);
  winchPos = constrain(winchPos, WINCH_PULSE_LOW, WINCH_PULSE_HIGH); //Trim bottom and upper end
  moveWinch(winchPos);
}

void updateRCRudders() {
  int rudderPos = pulseIn(CHANNEL_RUDDERS, HIGH);
  rudderPos = constrain(rudderPos, RUDDER_PULSE_LOW, RUDDER_PULSE_HIGH); //Trim bottom and upper end
  const int RudderMiddle = 0.5 * RUDDER_PULSE_HIGH + 0.5 * RUDDER_PULSE_LOW;
  if (rudderPos <= (RudderMiddle + RUDDER_DEAD_WIDTH / 2) && rudderPos >= (RudderMiddle - RUDDER_DEAD_WIDTH / 2)) { //Create Dead-Band
    rudderPos = RudderMiddle; // calculate the middle
  }
  moveRudder(rudderPos);
}

void checkRC() {
  // Wrapper function to improve code readability in the main loop and also rate limit RC updating
  if (millis() - lastRCMillis >= MIN_RC_DELAY && rcEnabled) {
    updateRCRudders();
    updateRCWinch();
    lastRCMillis = millis();
  }
}