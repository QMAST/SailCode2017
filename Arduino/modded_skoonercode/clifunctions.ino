/** Locally defined command line accessible functions
 ******************************************************************************
 */
/** Utility function
 * Checks to see if the bstring matches a given constant string without making
 * a copy of the bstrings contents.
 *
 * There is no guarantee that the char* in the bstring is valid or that the
 * given constant string will not contain anything weird
 *
 * Returns:
 * 1 If strings match
 * 0 Otherwise
 */
uint8_t
arg_matches( bstring arg, const char* ref )
{
    uint8_t match;

    match = strncasecmp(
            (char*) arg->data,
            ref,
            min( arg->slen, strlen(ref) )
        );

    // Return one if the string matches
    return match == 0 ? 1 : 0;
}

int cabout( blist list )
{
    Serial.println("GAELFORCE Skoonercode edition");
    Serial.println("Compiled " __DATE__ " " __TIME__ ".");
#ifdef DEBUG
    Serial.println(F("DEBUGGING BUILD"));
#endif

#ifndef _MEGA
    Serial.println(F("UNO BUILD: FUNCTIONS LIMITED"));
#endif

    char* buf;

    for( uint8_t i = 0; i < functions.index; i++ ) {
        Serial.print( (uint16_t) functions.cmds[i].func, HEX );
        Serial.print(F("-> "));

        buf = bstr2cstr( functions.cmds[i].name, '\0' );
        Serial.println( buf );
        free(buf);
    }
}

int cdiagnostic_report( blist list )
{
    diagnostics( &cli );

    return 0;
}


/*** Monitor relay a serial connection back to serial 0
 *
 *   Assuming Serial is the connection to the user terminal, it takes all
 *   characters received on the specified serial connection and prints them.
 *
 *   Not tested for communication from user to device
 *
 *   Pressing q terminates monitor
 */
#ifdef _MEGA
int cmon( blist list )
{
    char* buf; // For parsing input number
    uint8_t target_port_number;
    Stream* target_port;

    char input = '\0'; // For mirroring data between ports

    if( !list || list->qty <= 1 ) return CONSHELL_FUNCTION_BAD_ARGS;

    buf = bstr2cstr( list->entry[1], '\0' );
    target_port_number = strtoul( buf, NULL, 10 );
    free(buf);

#ifdef DEBUG
    Serial.print("Using port ");
    Serial.println( target_port_number );
#endif

    if( target_port_number == 1 ) target_port = &Serial1;
    else if( target_port_number == 2 ) target_port = &Serial2;
    else if( target_port_number == 3 ) target_port = &Serial3;

    while( input != 'q' ) {
        if( target_port->available() >= 1 ) {
            Serial.print( (char) target_port->read() );
        }

        if( Serial.available() >= 1 ) {
            target_port->write( (input = Serial.read()) );
        }
    }
}
#endif

/*** Get the values of each channel as neutral value
 *
 * Sets the value of the global rc structure such that each new value read from
 * the current position of the sticks is saved as the neutral position for that
 * stick
 */
#ifdef RC_CALIBRATION
int calrc( blist list )
{
    cli.port->println(F(
                "Please setup controller by: \n"
                "   - Set all sticks to up position (including gear)\n"
                "Press enter to continue."
                ));
    while( cli.port->available() == 0 )
        ;
    if( cli.port->read() == 'q' ) return -1;

    // Up
    radio_controller.rsy.high = rc_get_raw_analog( radio_controller.rsy );
    radio_controller.lsy.high = rc_get_raw_analog( radio_controller.lsy );
	
	cli.port->println(F(
                "Please setup controller by: \n"
                "   - Set all sticks to low position (including gear)\n"
                "Press enter to continue."
                ));
    while( cli.port->available() == 0 )
        ;
    if( cli.port->read() == 'q' ) return -1;

    // Low
    radio_controller.rsy.low = rc_get_raw_analog( radio_controller.rsy );
    radio_controller.lsy.low = rc_get_raw_analog( radio_controller.lsy );

    // Write the values to eeprom
    //rc_write_calibration_eeprom( 0x08, &radio_controller );

    cli.port->println(F("Calibration values set"));
}
#endif // RC_CALIBRATION


/** Given a +/- and a mode name, apply or remove that mode bit
 *
 *  Valid mode names are
 *  -   RC      <- RC Control mode
 *  -   AUTO 	<- Autosail mode
 *
 *  There must be a space between the +/- and modename
 *  NOTE - command is mode set auto for example not +/-
 */
int csetmode( blist list )
{
    unsigned int mode_bit = 0;
    char buf[50];

    if( list->qty <= 2 ) {
        return -1;
    }

    // Get the mode bit
    if(         arg_matches(list->entry[2], "auto") ) {
        mode_bit = MODE_AUTOSAIL;
    } else if(  arg_matches(list->entry[2], "rc") ) {
        mode_bit = MODE_RC_CONTROL; 
   } else if(  arg_matches(list->entry[2], "dia") ) {
        mode_bit = MODE_DIAGNOSTICS_OUTPUT;
    } else {
        // Do nothing and return
        return -2;
    }

    // Clear or set the bit
    if( arg_matches(list->entry[1], "set") ) {
        gaelforce |= mode_bit;
    } else {
        gaelforce &= ~(mode_bit);
    }

    snprintf_P( buf, sizeof(buf),
            PSTR( "MODE BIT %s: %d\n" ),
            arg_matches(list->entry[1], "set") ? "SET" : "UNSET",
            mode_bit );
    cli.port->print( buf );
    return 0;
}

/** Read write predetermined structures to and from eeprom
 *
 *  Uses two arguments, first is
 *      - r : for read
 *      - w : for write
 *
 *  Second is the item to be read/written, these structures are hardcoded in
 *  the sense that their eeprom address is written here. Available modules
 *
 *      - rc : RC calibration settings
 *      - ...
 *
 *      Returns -1 in case of error 0 otherwise
 */
int ceeprom( blist list )
{
    uint8_t rw = 0; // 0 is read-only
    bstring arg;

    if( list->qty <= 2 ) return -1;

    // Check the first argument
    arg = list->entry[1];
    if( arg_matches(arg, "w") ) {
        rw = 1;
    } else if( arg_matches(arg, "r") ) {
        rw = 0;
    } else {
#ifdef DEBUG
        Serial.print(F("USE EITHER r OR w"));
#endif
        return -1;
    }

    // Get the module name from the second argument
    arg = list->entry[2];
    if( arg_matches(arg, "rc") ) {
#ifdef DEBUG
        Serial.println(F("RC SETTINGS"));
#endif
        if( rw == 0 ) {
            rc_read_calibration_eeprom( 0x08, &radio_controller );
			Serial.print(F("Settings Loaded - "));
			rc_print_calibration( &Serial , &radio_controller);
        } else if( rw == 1 ) {
            rc_write_calibration_eeprom( 0x08, &radio_controller );
			Serial.print(F("Settings Saved - "));
			rc_print_calibration( &Serial , &radio_controller);
        }


        return 0;
    } else {
#ifdef DEBUG
        Serial.print(F("NOT A MODULE"));
#endif
    }

    return -1;
}
//Prints the adjusted rc value. Append "raw" to get raw analog pulses.
int crcd( blist list )
{
    if(arg_matches(list->entry[1], "raw") && list->qty != 1){

		while(      Serial.available() <= 0 &&  Serial.read() != 'q' ){
			rc_print_controller_raw( &Serial, &radio_controller );
			delay(100);
		}
    }
	
	else{
		while( Serial.available() <= 0 &&  Serial.read() != 'q' ){
			rc_print_controller_mapped( &Serial, &radio_controller );
			delay(100);
		}
	}
	
	return -1;
}


/** Primary motor control at command line
 *
 * Unlock the winch motors                          > mot u
 * Set the speed of a winch 0 to 2300 of 3200 units > mot g 0 2300
 * Set speed of winch in opposite direction         > mot g 0 -2300
 * Set rudder servo 0 to 1500us                     > mot r 0 1500
 * Halt winch motors and lock them                  > mot s
 *
 *mot w [-1000 to 1000] for winch
 *mot k [-1000 to 1000] for rudder
 *
 * Current event based commands disabled
 */
int cmot( blist list )
{
    bstring arg;

    // Check if no actual arguments are given
    if( list->qty <= 1 ) {
        cli.port->print(F(
            "s - stop and lock all motors\n"
            "u - request unlock of both motors\n"
            "r - moves rudder to target position [-1000 1000]\n"
			"wv - moves winch to target speed [-1000 1000]\n"
			"wa - moves winch to absolute position, number of encoder ticks."
			"wr - moves winch to relative position, number of encoder ticks."
			"wcal - calibrate winch to 0 position."
            ));
        return -1;
    }

    // Check arguments
    arg = list->entry[1];
    if( arg_matches( arg, "s" ) ) {
        motor_lock();
		cli.port->println(F("MOTORS LOCKED"));
    } 
    else if( arg_matches( arg, "u" ) ) {
        motor_unlock();
		cli.port->println(F("MOTORS UNLOCKED"));
    } 
	
	else if( arg_matches( arg, "r" ) ){
		int16_t mot_pos =
            strtol( (char*) list->entry[2]->data, NULL, 10 );
        mot_pos = constrain( mot_pos, -1000, 1000 );
		
		motor_set_rudder( mot_pos );
	}
	
	else if( arg_matches( arg, "wa" ) ){
		int32_t target =
            strtol( (char*) list->entry[2]->data, NULL, 10 );
			
		motor_winch_abs(target);
		uint8_t target_reached = 0;
		
		while(target_reached == 0){
			if(Serial.available() && Serial.read() == 'q') {
				cli.port->print(F("Pressed Q!"));
				break;
			}
			
			target_reached = motor_winch_update();
		}
	}
	
	else if( arg_matches( arg, "wr" ) ){
		int32_t target =
            strtol( (char*) list->entry[2]->data, NULL, 10 );
			
		motor_winch_rel(target);
		uint8_t target_reached = 0;
		
		while(target_reached == 0){
			if(Serial.available() && Serial.read() == 'q') {
				cli.port->print(F("Pressed Q!"));
				break;
			}
			
			target_reached = motor_winch_update();
		}
	}
	
	else if( arg_matches( arg, "wv" ) ){
		int16_t mot_pos =
            strtol( (char*) list->entry[2]->data, NULL, 10 );
        mot_pos = constrain( mot_pos, -1000, 1000 );
		
		motor_set_winch( mot_pos );
	}
	
	else if( arg_matches( arg, "wcal" ) ){
		motor_cal_winch();
		Serial.print("Winch position set to zero!\n");
	}
}

/** To get the time from the real time clock to set linux time
 *
 * Gets the number of seconds for the epoch, which can be used to set the time
 * on the clock of the raspberry pi.
 */
int cnow( blist list )
{
    cli.port->println(now());
}

int cres( blist list )
{
    reset_barnacle();
}

/*
* airmar poll - reads raw data from airmar stream.
* airmar en [head, wind, gps, all] - enables the specified tag on the airmar
* airmar dis [head, wind, gps, all] - disables specified tags on the airmar
*Note all tags are set to run every 5/10s of a second. This can be changed below with "5".
*/

int cairmar(blist list){
	bstring arg = list->entry[1];
	if( arg_matches( arg, "poll" ) ){
		while(		Serial.available() <= 0
            &&  Serial.read() != 'q' ){
				if(SERIAL_PORT_AIRMAR.available())
					cli.port->print((char)SERIAL_PORT_AIRMAR.read());
			}
	}

	else if( arg_matches( arg, "greasepoll" ) ){
		char wind_buf[50];
		char grease_buf;
		int i = 0;
		while(		Serial.available() <= 0
            &&  Serial.read() != 'q'){
				if(SERIAL_PORT_AIRMAR.available()){
					wind_buf[i] = SERIAL_PORT_AIRMAR.read();
					i++;
					if(i > 49){
						cli.port->print(wind_buf[0]);
						Serial.print(F("\n"));
						i = 0;
					}
				}
			}
	}
	else if( arg_matches( arg, "en" ) ){
		arg = list->entry[2];
		if(arg_matches( arg, "wind" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,MWVR,1,10");
		else if(arg_matches( arg, "all" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,ALL,1,10");
		else if(arg_matches( arg, "head" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,HDG,1,10");
		else if(arg_matches( arg, "gps" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,GGA,1,10");
		cli.port->print("Sent to AIRMAR!");
		cli.port->println();
	}
	
	else if( arg_matches( arg, "dis" ) ){
		arg = list->entry[2];
		if(arg_matches( arg, "wind" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,MWVR,0,5");
		else if(arg_matches( arg, "all" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,ALL,0,5");
		else if(arg_matches( arg, "head" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,HDG,0,5");
		else if(arg_matches( arg, "gps" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,GGA,0,5");
		cli.port->print("Sent to AIRMAR!");
		cli.port->println();
	}
	
}
/******************************************************************************
 * END OF COMMAND LINE FUNCTIONS */
// vim:ft=c:
