#define WINCH_HIGH_SPEED 800
#define WINCH_MED_SPEED 500
#define WINCH_LOW_SPEED 200

#define WINCH_HIGH_THRESH 2000
#define WINCH_MED_THRESH 500
#define WINCH_TARGET_THRESH 50

int32_t winch_current_position;
int32_t winch_target;

const uint32_t MIN_RC_WAIT = 10; // (msec) Minimum time before updating
const uint32_t PCHAMP_REQ_WAIT = 5; // (msec) Time to wait for response

//between -1000 to 1000 max
void motor_set_rudder( int target ){
			
	char buf[40];       // buffer for printing debug messages
    uint16_t rvar = 0;  // hold result of remote device status (pololu controller)
			
    target = constrain( target, -1000, 1000 );
    int target0 = map( target,
            -1000, 1000,
            POLOLU_SERVO_0_RUD_MIN,
            POLOLU_SERVO_0_RUD_MAX );
	int target2 = map( target,
            -1000, 1000,
            POLOLU_SERVO_2_RUD_MIN,
            POLOLU_SERVO_2_RUD_MAX );
    /*snprintf_P( buf, sizeof(buf), PSTR("Call rudder: %d\n"), rc_output );*/
    /*Serial.print( buf );*/
	//Rudder 0
    pchamp_servo_set_position( &(rudder_servo[0]), target0 );
    rvar = pchamp_servo_request_value( &(rudder_servo[0]), PCHAMP_SERVO_VAR_ERROR );
    delay(PCHAMP_REQ_WAIT);
    if( rvar != 0 ) {
        snprintf_P( buf, sizeof(buf), PSTR("SERVERR0: 0x%02X\n"), rvar );
        Serial.print(buf);
    }
	
	//Rudder 1
    pchamp_servo_set_position( &(rudder_servo[1]), target2 );
    rvar = pchamp_servo_request_value( &(rudder_servo[1]), PCHAMP_SERVO_VAR_ERROR );
    delay(PCHAMP_REQ_WAIT);
    if( rvar != 0 ) {
        snprintf_P( buf, sizeof(buf), PSTR("SERVERR1: 0x%02X\n"), rvar );
        Serial.print(buf);
    }
}

//between -1000 to 1000
uint8_t winch_current_direction;
void motor_set_winch( int target ){
	
	char buf[40];       // buffer for printing debug messages
    uint16_t rvar = 0;  // hold result of remote device status (pololu controller)
	uint16_t rvar_serial = 0;  // hold result of remote device status (pololu controller)

    target = constrain( target, -1000, 1000);
    winch_current_direction = target > 0 ? PCHAMP_DC_FORWARD : PCHAMP_DC_REVERSE;
    target = map( abs(target), 0, 1000, 0, 3200 );
	target = target/3;
	//winch 0
    //pchamp_request_safe_start( &(winch_control[0]) );
    pchamp_set_target_speed( &(winch_control[0]), target, winch_current_direction );
    delay(PCHAMP_REQ_WAIT);

	//Check for error:
    rvar = pchamp_request_value( &(winch_control[0]), PCHAMP_DC_VAR_ERROR );	//General Error Request
    if( rvar != 0 ) {
		rvar_serial = pchamp_request_value( &(winch_control[0]), PCHAMP_DC_VAR_ERROR_SERIAL );	//Serial Error Request
		
		if(rvar_serial != 0){
			snprintf_P( buf, sizeof(buf), PSTR("W0ERR_SERIAL: 0x%02x\n"), rvar_serial );
			pchamp_request_safe_start( &(winch_control[0]) );	//If there is a serial error, ignore it and immediately restart
		}
		else
			snprintf_P( buf, sizeof(buf), PSTR("W0ERR: 0x%02x\n"), rvar );
        Serial.print(buf);
    }

    //winch 1
	/*
    pchamp_request_safe_start( &(mast[1]) );
    pchamp_set_target_speed( &(mast[1]), target, winch_current_direction; );
    delay(PCHAMP_REQ_WAIT);

    rvar = pchamp_request_value( &(mast[1]), PCHAMP_DC_VAR_ERROR );
    if( rvar != 0 ) {
        snprintf_P( buf, sizeof(buf), PSTR("W1ERR: 0x%02x\n"), rvar );
        Serial.print(buf);
    }*/
	
}

//Locks all motors
// - Servos are set to 0
// - PDC is locked and turned off
void motor_lock(){
	pchamp_set_target_speed(
        &(winch_control[0]), 0, PCHAMP_DC_MOTOR_FORWARD );
    pchamp_request_safe_start( &(winch_control[0]), false );
    //pchamp_set_target_speed(
    //    &(winch_control[1]), 0, PCHAMP_DC_MOTOR_FORWARD );
    //pchamp_request_safe_start( &(winch_control[1]), false );

    // Servos (disengage)
    pchamp_servo_set_position( &(rudder_servo[0]), 0 );
    pchamp_servo_set_position( &(rudder_servo[1]), 0 );
}

//Unlocks PDC winch.
void motor_unlock(){
	pchamp_request_safe_start( &(winch_control[0]) );
    //pchamp_request_safe_start( &(winch_control[1]) );
    
}


void motor_winch_abs(int32_t target_abs){
	//Clear current ticks;
	uint16_t ticks = barn_getandclr_w1_ticks();
	if(winch_current_direction == PCHAMP_DC_REVERSE)
		winch_current_position -= ticks;
	else
		winch_current_position += ticks;
	
	//Set new target:
	winch_target = target_abs;
	Serial.print("Target set to: ");
	Serial.println(target_abs);
	
	motor_winch_update();
}

void motor_winch_rel(int32_t target_rel){
	//Clear current ticks;
	uint16_t ticks = barn_getandclr_w1_ticks();
	if(winch_current_direction == PCHAMP_DC_REVERSE)
		winch_current_position -= ticks;
	else
		winch_current_position += ticks;
	
	//Set new target:
	winch_target = winch_current_position + target_rel;
	
	motor_winch_update();
}

//Motor update function used for absolute positioning, call as frequently as possible
//Returns 1 if target has been reached, 0 otherwise
int motor_winch_update(){
	char buf[500];
	static int16_t winch_velocity = 0;
	static int16_t last_winch_velocity;
	uint8_t winch_target_reached = 0;
		
	//Update current position tracker
	uint16_t ticks = barn_getandclr_w1_ticks();
	if(winch_current_direction == PCHAMP_DC_REVERSE)
		winch_current_position -= (int32_t)ticks;
	else
		winch_current_position += (int32_t)ticks;
	
	int32_t offset = (int32_t) winch_target - winch_current_position;
	
	//Get velocity magnitude:
	if(abs(offset)>		 WINCH_HIGH_THRESH)
		winch_velocity = WINCH_HIGH_SPEED;
	else if(abs(offset)> WINCH_MED_THRESH)
		winch_velocity = WINCH_MED_SPEED;
	else if(abs(offset)> WINCH_TARGET_THRESH)
		winch_velocity = WINCH_LOW_SPEED;
	else{
		winch_velocity = 0;
		winch_target_reached = 1;
	}
		
	
	//Get velocity direction
	if(offset < 0)
		winch_velocity = abs(winch_velocity) * -1;
	
	//Exit if no change in vel
	//Perhaps do not use to avoid interference errors (ie. something else set motor speed)
	//if(winch_velocity == last_winch_velocity)
		//return winch_target_reached;
	
	motor_set_winch(winch_velocity);
	
	/*Serial.print("Speed: ");
	Serial.print(winch_velocity);
	Serial.print("\tPos: ");
	Serial.print(winch_current_position);
	Serial.print("\tOff: ");
	Serial.println(offset);*/
	
	return winch_target_reached;
}

void motor_cal_winch(){
	winch_current_position = 0;
}