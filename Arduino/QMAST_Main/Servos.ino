/*
  Servos
  Code controlling the movement of the rudder, winch and any other servos

  Created in January 2018, QMAST
*/

#include "pins.h"
#include <Servo.h>

Servo servoRudder;
Servo servoWinch;

void initServos(){
  servoRudder.attach(PIN_SERVO_1);
  servoWinch.attach(PIN_SERVO_WINCH);
}

void moveRudder(int pos){
  // Move the rudder between 0-180
  servoRudder.write(pos);
}

void moveWinch(int pos){
  // Move the winch between 0-180 (fully in vs fully out)
  //Serial.print("Moving Winch to ");
  //Serial.println(pos);
  servoWinch.write(pos);
}

