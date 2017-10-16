#include "Globals.h"

// Some pins don't work due to interference. Test and change pins if needed
SoftwareSerial maestroSerial(PIXY_SERVO_RX, PIXY_SERVO_TX);
MicroMaestro maestro(maestroSerial);

void initServo() {
  // Set the pololu serial baud rate.
  maestroSerial.begin(57600);
  setServoSpeed(PIXY_SERVO_CHANNEL, 40);
}

int getServoDeg(int servo) {
  // Gets the passed servo's position as a degree between -90 and 90
  if (servo == PIXY_SERVO_CHANNEL) {
    return map((maestro.getPosition(servo) / 4), PIXY_SERVO_MAX_LEFT, PIXY_SERVO_MAX_RIGHT, -90, 90);
  }
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

void setServoDeg(int servo, int degree) {
  // Sets the passed servo to a degree between -90 and 90
  if (servo == PIXY_SERVO_CHANNEL) {
    //Serial.println("Target set to " + String(map(degree, -90, 90, PIXY_SERVO_MAX_LEFT, PIXY_SERVO_MAX_RIGHT)));
    maestro.setTarget(PIXY_SERVO_CHANNEL, map(degree, -90, 90, PIXY_SERVO_MAX_LEFT, PIXY_SERVO_MAX_RIGHT) * 4);
  }
}

