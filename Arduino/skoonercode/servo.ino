#include "pins.h"

SoftwareSerial maestroSerial(52, 53);
MicroMaestro maestro(maestroSerial);

int currentServoDeg = 0;

void initServo() {
  // Set the pololu serial baud rate.
  maestroSerial.begin(SERIAL_BAUD_POLOLU_MAESTRO);
  setServoSpeed(PIXY_SERVO_CHANNEL, 40);
  setServoSpeed(1, 0);
  setServoSpeed(2, 0);
}

int getServoPixyDeg() {
  // Gets the pixy servo's position as a degree between -90 and 90
  return map((maestro.getPosition(PIXY_SERVO_CHANNEL) / 4), PIXY_SERVO_MAX_LEFT, PIXY_SERVO_MAX_RIGHT, -90, 90);
}

int getServoRudderDeg() {
  // Gets the rudder servo's position as a degree between -90 and 90
  return map((maestro.getPosition(RUDDER_0_SERVO_CHANNEL) / 4), RUDDER_0_SERVO_MAX_LEFT, RUDDER_0_SERVO_MAX_RIGHT, -90, 90);
}

int getServoPosition(int servo) {
  return maestro.getPosition(servo);
}

void setServoSpeed(int servo, int speed) {
  // Sets the passed servo to a speed between 1 - 100
  // Alternatively, speed = 0 = unrestricted speed
  if (speed == 0) {
    maestro.setSpeed(servo, 0);
  } else {
    maestro.setSpeed(servo, speed);
  }
}


void setServoPosition(int servo, int position) {
  maestro.setTarget(servo, position);
}

void setServoPixyDeg(int degree) {
  // Sets the pixy servo to a degree between -90 and 90
  maestro.setTarget(PIXY_SERVO_CHANNEL, map(degree, -90, 90, PIXY_SERVO_MAX_LEFT, PIXY_SERVO_MAX_RIGHT) * 4);
}

void setServoRudderDeg(int target ) {
  target = constrain( target, -85, 85 );
  int target0 = map( target,
                     -90, 90,
                     RUDDER_0_SERVO_MAX_LEFT,
                     RUDDER_0_SERVO_MAX_RIGHT );
  int target1 = map( target,
                     -90, 90,
                     RUDDER_1_SERVO_MAX_LEFT,
                     RUDDER_1_SERVO_MAX_RIGHT );
  maestro.setTarget(1, (target0 * 4));
  maestro.setTarget(2, (target1 * 4));
  //Serial.println(target);
}

void setServoRudder( int target ) {
  //between -100 to 100 max

  target = constrain( target, -95, 95 );
  int target0 = (map( target,
                      -100, 100,
                      RUDDER_0_SERVO_MAX_LEFT,
                      RUDDER_0_SERVO_MAX_RIGHT )) * 4;
  int target1 = (map( target,
                      -100, 100,
                      RUDDER_1_SERVO_MAX_LEFT,
                      RUDDER_1_SERVO_MAX_RIGHT )) * 4;

  maestro.setTarget(1, target0);
  maestro.setTarget(2, target1);
}
