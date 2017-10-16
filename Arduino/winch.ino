#define OUT_BOUND 500
#define IN_BOUND 665

long targetPosition;
boolean target_enabled = false;

boolean moving = false;
uint8_t movingDirection;
long movingCheckMillis;
long potWaitMillis;

boolean guardEnabled = true;

void moveWinch(uint8_t goDirection) {
  if (guardEnabled) {
    if (goDirection == PCHAMP_DC_REVERSE && encoderMean < OUT_BOUND) {
      Serial.println("Reached out bound, cannot start");
    } else if (goDirection == PCHAMP_DC_FORWARD && encoderMean > IN_BOUND) {
      Serial.println("Reached in bound, cannot start");
    } else {
      if (moving && goDirection != movingDirection) {
        stopWinch();
      }
      pchamp_set_target_speed( &(winch_control[0]), 1600, goDirection );
      movingCheckMillis = millis();
      movingDirection = goDirection;
      moving = true;
    }
  } else {
    pchamp_set_target_speed( &(winch_control[0]), 1600, goDirection );
    movingCheckMillis = millis();
    movingDirection = goDirection;
    moving = true;
  }
}

void stopWinch() {
  Serial.print("Stopping winch");
  if (moving) {
    pchamp_set_target_speed( &(winch_control[0]), 0, PCHAMP_DC_FORWARD );
    moving = false;
    target_enabled = false;
    Serial.println(encoderMean);
  }
}

void setWinchTarget(int target) {
  // Move winch to a position from 0 to 5000
  //if (moving) stopWinch();
  target = map(target, 0, 100, IN_BOUND, OUT_BOUND);
  Serial.print("Winch targeting ");
  Serial.println(target);
  if (target != encoderMean) {
    uint8_t targetDirection = (target - encoderMean) > 0 ? PCHAMP_DC_FORWARD : PCHAMP_DC_REVERSE;
    target_enabled = true;
    targetPosition = target;
    moveWinch(targetDirection);
  }
}

void updateWinchPosition() {
  if (target_enabled == true) {
    if (movingDirection == PCHAMP_DC_REVERSE && encoderMean < targetPosition) {
      stopWinch();
      target_enabled = false;
    } else if (movingDirection == PCHAMP_DC_FORWARD && encoderMean > targetPosition) {
      stopWinch();
      target_enabled = false;
    }
  }
  
  if (moving && (millis() - movingCheckMillis) > 500) {
    /*if (movingDirection == PCHAMP_DC_FORWARD) {
      Serial.println("Forward");
      currentPosition = currentPosition + map((nowMillis - movingCheckMillis), 0, 4500, 0, 5500);
      } else if (movingDirection == PCHAMP_DC_REVERSE) {
      Serial.println("Reverse");
      currentPosition = currentPosition - (nowMillis - movingCheckMillis);
      }*/
    
    stopWinch();
    potWaitMillis = millis();
    

    Serial.println(encoderMean);
    sendTransmission(55, 200, String(encoderMean));
    if (guardEnabled) {
      if (movingDirection == PCHAMP_DC_REVERSE && encoderMean < OUT_BOUND) {
        Serial.println("Reached outbound");
        stopWinch();
      } else if (movingDirection == PCHAMP_DC_FORWARD && encoderMean > IN_BOUND) {
        Serial.println("Reached inbound");
        stopWinch();
      }
    }
  }

  if(millis - potWaitMillis > 500){
    if (guardEnabled) {
      if (movingDirection == PCHAMP_DC_REVERSE && analogRead(1) < OUT_BOUND) {
        Serial.println("Reached outbound");
        stopWinch();
      } else if (movingDirection == PCHAMP_DC_FORWARD && analogRead(1) > IN_BOUND) {
        Serial.println("Reached inbound");
        stopWinch();
      }else{
        
      }
    }
  }

}

void guardOff() {
  guardEnabled = false;
}

/*void calibrateWinchIn() {
  //currentPosition = 0;
  guardEnabled = true;
  sendTransmission(55, 200, String(encoderMean));
}*/

