#define WINCH_HIGH_SPEED 800
#define WINCH_MED_SPEED 500
#define WINCH_LOW_SPEED 200

#define WINCH_HIGH_THRESH 2000
#define WINCH_MED_THRESH 500
#define WINCH_TARGET_THRESH 50

long winch_current_position = 0;
int32_t winch_target;
boolean approach_target = false;
int last_set_starget;

const uint32_t MIN_RC_WAIT = 10; // (msec) Minimum time before updating
const uint32_t PCHAMP_REQ_WAIT = 5; // (msec) Time to wait for response

//between -1000 to 1000
uint8_t winch_current_direction;

void motor_set_winch( int target ) {
  if (target != last_set_starget) {
    last_set_starget = target;
    char buf[40];       // buffer for printing debug messages
    uint16_t rvar = 0;  // hold result of remote device status (pololu controller)
    uint16_t rvar_serial = 0;  // hold result of remote device status (pololu controller)

    target = constrain( target, -1000, 1000);
    Serial.print("Going at ");
    Serial.println(target);
    winch_current_direction = target > 0 ? PCHAMP_DC_FORWARD : PCHAMP_DC_REVERSE;
    target = map( abs(target), 0, 1000, 0, 3200 );
    /*if (winch_current_direction == PCHAMP_DC_FORWARD && winch_current_position >= 20000) {
      Serial.println("Winch cannot move forward past this point");
      } else if (winch_current_direction == PCHAMP_DC_REVERSE && winch_current_position <= 0) {
      Serial.println("Winch cannot move backwards past this point");
      } else {*/
    //winch 0
    //pchamp_request_safe_start( &(winch_control[0]) );
    pchamp_set_target_speed( &(winch_control[0]), target, winch_current_direction );
    delay(PCHAMP_REQ_WAIT);

    //Check for error:
    rvar = pchamp_request_value( &(winch_control[0]), PCHAMP_DC_VAR_ERROR );  //General Error Request
    if ( rvar != 0 ) {
      rvar_serial = pchamp_request_value( &(winch_control[0]), PCHAMP_DC_VAR_ERROR_SERIAL );  //Serial Error Request

      if (rvar_serial != 0) {
        snprintf_P( buf, sizeof(buf), PSTR("W0ERR_SERIAL: 0x%02x\n"), rvar_serial );
        pchamp_request_safe_start( &(winch_control[0]) ); //If there is a serial error, ignore it and immediately restart
      }
      else
        snprintf_P( buf, sizeof(buf), PSTR("W0ERR: 0x%02x\n"), rvar );
      Serial.print(buf);
    }
    //}
  }
}

//Locks all motors
// - Servos are set to 0
// - PDC is locked and turned off
void motor_lock() {
  Serial.println("Locking Winch");
  pchamp_set_target_speed(
    &(winch_control[0]), 0, PCHAMP_DC_MOTOR_FORWARD );
  pchamp_request_safe_start( &(winch_control[0]), false );
  //pchamp_set_target_speed(
  //    &(winch_control[1]), 0, PCHAMP_DC_MOTOR_FORWARD );
  //pchamp_request_safe_start( &(winch_control[1]), false );

  // Servos (disengage)
  //pchamp_servo_set_position( &(rudder_servo[0]), 0 );
  //pchamp_servo_set_position( &(rudder_servo[1]), 0 );
}

//Unlocks PDC winch.
void motor_unlock() {
  Serial.println("Unlocking Winch");
  pchamp_request_safe_start( &(winch_control[0]) );
  //pchamp_request_safe_start( &(winch_control[1]) );

}


void motor_winch_abs(int32_t target_abs) {
  //Clear current ticks;
  uint16_t ticks = barnacle_client->barn_getandclr_w1_ticks();

  if (winch_current_direction == PCHAMP_DC_REVERSE)
    winch_current_position -= ticks;
  else
    winch_current_position += ticks;

  //Set new target:
  winch_target = target_abs;
  Serial.print("Target set to: ");
  Serial.println(target_abs);
  approach_target = true;
  motor_winch_update();
}

void motor_winch_rel(int32_t target_rel) {
  //Clear current ticks;
  uint16_t ticks = barnacle_client->barn_getandclr_w1_ticks();

  if (winch_current_direction == PCHAMP_DC_REVERSE)
    winch_current_position -= ticks;
  else
    winch_current_position += ticks;

  //Set new target:
  winch_target = winch_current_position + target_rel;
  approach_target = true;
  motor_winch_update();
}

//Motor update function used for absolute positioning, call as frequently as possible
//Returns 1 if target has been reached, 0 otherwise
int motor_winch_update() {
  char buf[500];
  static int16_t winch_velocity = 0;
  uint8_t winch_target_reached = 0;

  //Update current position tracker
  uint16_t ticks = barnacle_client->barn_getandclr_w1_ticks();

int scaledPosition;
  if (winch_current_direction == PCHAMP_DC_REVERSE) {
    Serial.print("Reversing ");
    scaledPosition = map(ticks, 0, 61130, 0, 20000);
    winch_current_position -= scaledPosition;
  } else {
    Serial.print("Going forward ");
    scaledPosition = map(ticks, 0, 27839, 0, 20000);
    winch_current_position += scaledPosition;
  }

  Serial.print("Currently at ");
  Serial.println(winch_current_position);
  if (approach_target == true) {
    int32_t offset = (int32_t) winch_target - winch_current_position;

    //Get velocity magnitude:
    if (abs(offset) >    50) {
      winch_velocity = WINCH_MED_SPEED;
    } else {
      winch_velocity = 0;
      winch_target_reached = 1;
      approach_target = false;
    }

    //Get velocity direction
    if (offset < 0)
      winch_velocity = abs(winch_velocity) * -1;

    motor_set_winch(winch_velocity);

    /*Serial.print("Speed: ");
      Serial.print(winch_velocity);
      Serial.print("\tPos: ");
      Serial.print(winch_current_position);
      Serial.print("\tOff: ");
      Serial.println(offset);*/
  }
  delay(50);
  return winch_target_reached;
}

void motor_cal_winch() {
  winch_current_position = 0;
}
