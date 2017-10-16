#define IN_BOUND 835
#define OUT_BOUND 610

boolean target_enabled = false;
int targetPosition;

boolean moving = false;
uint8_t movingDirection;
boolean guardEnabled = false;


void moveWinch(uint8_t goDirection) {
  Serial.print(F("s:"));
  Serial.print(goDirection);
  Serial.println(PCHAMP_DC_REVERSE);
  if (moving && movingDirection == goDirection) {
    Serial.println(F("Moving in same direction, ignoring"));
  } else if (goDirection ==  PCHAMP_DC_FORWARD && currentPosition > IN_BOUND) {
    Serial.println(F("Bound reached, cannot move forward."));
  } else if (goDirection ==  PCHAMP_DC_REVERSE && currentPosition < OUT_BOUND) {
    Serial.println(F("Bound reached, cannot in reverse."));
  } else {
    if (moving) stopWinch();
    Serial.print("Moving ");
    Serial.println(goDirection);
    pchamp_set_target_speed( &(winch_control[0]), 1600, goDirection );
    movingDirection = goDirection;
    moving = true;
  }
}

void stopWinch() {
  Serial.print(F("Stopping winch"));
  if (moving) {
    pchamp_set_target_speed( &(winch_control[0]), 0, PCHAMP_DC_FORWARD );
    moving = false;
    target_enabled = false;
  }
}

void setWinchTarget(int target) {
  target = constrain(target, 0, 1000);
  targetPosition = map(target, 0, 1000, IN_BOUND, OUT_BOUND);
  // Move winch to a position from 0 to 1000
  if (target != currentPosition) {
    uint8_t targetDirection = (targetPosition - currentPosition) > 0 ? PCHAMP_DC_FORWARD : PCHAMP_DC_REVERSE;
    target_enabled = true;
    moveWinch(targetDirection);
  }
}

long lastReportW = 0;

void updateWinchPosition() {
  if (millis() - lastReportW > 250) {
    lastReportW = millis();
    //Serial.println(currentPosition);
    sendTransmission(55, 200, String(currentPosition));
  }
  if (moving) {
    if (movingDirection == PCHAMP_DC_REVERSE && currentPosition < OUT_BOUND) {
      Serial.println(F("Bound reached, cannot move in reverse!"));
      stopWinch();
    } else if (movingDirection == PCHAMP_DC_FORWARD && currentPosition > IN_BOUND) {
      Serial.println(F("Bound reached, cannot move forward!"));
      stopWinch();
    }

    if (currentPosition < 10) {
      stopWinch();
    }
    
    if (target_enabled) {
      if (movingDirection == PCHAMP_DC_FORWARD && currentPosition > targetPosition) {
        Serial.println(F("Target reached forward"));
        stopWinch();
      } else if (movingDirection == PCHAMP_DC_REVERSE && currentPosition < targetPosition) {
        Serial.println(F("Target reached in reverse"));
        stopWinch();
      }
    }
  }
}

/*void guardOff() {
  guardEnabled = false;
  }

  void calibrateWinchIn() {
  currentPosition = 0;
  guardEnabled = true;
  sendTransmission(55, 200, String(currentPosition));
  }*/

