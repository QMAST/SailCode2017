// vim:ft=c:
/** Radio control mode

   Grab values from the rc controller and set motor values based on them

   TODO: Implement timeout functionality and the ability to continue part way
   through if interrupted

   TODO: Don't send a new command if values haven't changed

   TODO: Change to relational movement, stick doesn't correspond directly to
   the position or speed of a motor. Simply changes the desired position state
   as its being held in a particular direction
*/

#define MIN_RC_WAIT 50 // (msec) Minimum time before updating

void
rmode_rudders(
  rc_mast_controller* rc)
{
  static uint16_t old_rudder = 0;
  static uint16_t double_check_rudder = 0;
  int16_t rc_input;
  //Rudder
  rc_input = rc_get_mapped_analog( rc->lsx, 100, -100 );
  if (rc_input > 200 || rc_input < -200) {
    setServoRudderDeg(0);
  } else if (old_rudder != rc_input && (abs(double_check_rudder - rc_input) <= 10)) {
    //Serial.print("Rudder Remote input");
    //Serial.println(rc_input);
    //sendTransmissionStart(63, 200);
    if (rc_input < -2000) {
      //SERIAL_PORT_XBEE.print(F("Remote not connected"));
    } else {
      //SERIAL_PORT_XBEE.print(rc_input);
      if (abs(rc_input) <= 5) {
        setServoRudderDeg(0);
      }
      else if (abs(old_rudder - rc_input) >= 4) {
        setServoRudderDeg(rc_input);
      }
      old_rudder = rc_input;
    }
    //sendTransmissionEnd();
  }
  double_check_rudder = rc_input;
}

void
rmode_winch(
  rc_mast_controller* rc)
{
  static uint16_t old_winch = 5;
  int16_t rc_input;
  //Winch
  rc_input = rc_get_mapped_analog( rc->rsy, -1000, 1000 );
  //Serial.print("Winch Remote input");
  //Serial.println(rc_input);
  //sendTransmissionStart(64, 200);
  if (rc_input < -2000) {
    //Serial.println("Not connected");
    //SERIAL_PORT_XBEE.print(F("Remote not connected"));
    //Serial.println(F("Remote not connected"));
  } else {
    //Serial.println(rc_input);
    //SERIAL_PORT_XBEE.print(rc_input);
    if (abs(rc_input) < 200 && old_winch != 0) {
      //SERIAL_PORT_XBEE.print(rc_input);
      //SERIAL_PORT_XBEE.print(" (Zeroed)");
      //Serial.println("Zero");
      //motor_set_winch(0);
      stopWinch();
      old_winch = 0;
    } else if (rc_input > 200 && old_winch != 1) {
      //Serial.println("Left");
      //motor_set_winch(500);
      old_winch = 1;
      moveWinch(PCHAMP_DC_REVERSE);
    } else if (rc_input < -200 && old_winch != -1) {
      //Serial.println("Right");
      //motor_set_winch(-500);
      old_winch = -1;
      moveWinch(PCHAMP_DC_FORWARD);
    }
  }
  //sendTransmissionEnd();

  /*//Winch
    rc_input = rc_get_mapped_analog( rc->rsy, -1000, 1000 );
    if (abs(rc_input) < 100)
    rc_input = 0;
    if (old_winch != rc_input ) {
    sendTransmissionStart(64, 200);
    if (rc_input < -2000) {
      SERIAL_PORT_XBEE.print(F("Remote not connected"));
    } else {
      SERIAL_PORT_XBEE.print(rc_input);
      if (abs(old_winch - rc_input) >= 25) {
        //Serial.println(F("Rmode updating winch"));
        //Serial.print("Rc_input = ");
        //Serial.println(rc_input);
        motor_set_winch(
          rc_input
        );
      }
      old_winch = rc_input;
    }
    sendTransmissionEnd();

    }*/
}

void
rmode_update_motors(
  rc_mast_controller* rc,
  pchamp_controller mast[2])
{

  // Track positions from last call, prevent repeat calls to the same
  // position.
  static uint32_t time_last_updated;

  if ( (millis() - time_last_updated) < MIN_RC_WAIT ) {
    return;
  }
  time_last_updated = millis();

  rmode_rudders(rc);
  rmode_winch(rc);
}

