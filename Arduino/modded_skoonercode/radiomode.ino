// vim:ft=c:
/** Radio control mode
 *
 * Grab values from the rc controller and set motor values based on them
 *
 * TODO: Implement timeout functionality and the ability to continue part way
 * through if interrupted
 *
 * TODO: Don't send a new command if values haven't changed
 *
 * TODO: Change to relational movement, stick doesn't correspond directly to
 * the position or speed of a motor. Simply changes the desired position state
 * as its being held in a particular direction
 */
 
 
void
rmode_update_motors(
        rc_mast_controller* rc,
        pchamp_controller mast[2],
        pchamp_servo rudder[2] )
{


    int16_t rc_input;

    char buf[40];       // buffer for printing debug messages
    uint16_t rvar = 0;  // hold result of remote device status (pololu controller)

    // Track positions from last call, prevent repeat calls to the same
    // position.
    static uint16_t old_rudder = 0;
    static uint16_t old_winch = 0;

    static uint32_t time_last_updated;
    static uint32_t time_rudder_last_updated = 0;

    if( (millis() - time_last_updated) < MIN_RC_WAIT ) {
        return;
    }
    time_last_updated = millis();

	//Rudder
	rc_input = rc_get_mapped_analog( rc->rsy, -1000, 1000 );
	if(abs(rc_input) <= 50){
		motor_set_rudder(0);
	}
	else if(abs(old_rudder - rc_input)>=25){
		motor_set_rudder(
                rc_input
            );
		old_rudder = rc_input;
	}

	
    
	//Winch
	rc_input = rc_get_mapped_analog( rc->lsy, -1200, 1200 );
	if(abs(rc_input)<400)
		rc_input = 0;
	rc_input = map(rc_input, -1200, 1200, -1000, 1000);
		
	
	if(abs(old_winch - rc_input)>=25){
		motor_set_winch(
                rc_input
            );
		old_winch = rc_input;
	}

}

