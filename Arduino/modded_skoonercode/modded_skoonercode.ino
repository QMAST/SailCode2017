
/** Skoonercode
 *
 * Supercedes ferrycode, I thought the name looked cooler than Schoonercode
 *
 * Uses a simple polling loop with timeouts enforced by the functions
 * themselves. Each function is passed the amount of time it is allowed to take
 * for execution, and either exits as soon as possible, or attempts to perform
 * its function for the entire time until it realises it needs to exit.
 *
 * Main interface, command line functions. No function may lock the polling
 * loop into a single function. Certain polling functions may be
 * enabled/disabled on the fly.
 */
#ifndef _SKOONERCODE_INO
#define _SKOONERCODE_INO

#include "pins.h"
#include <SoftwareSerial.h>

#include <avr/pgmspace.h>
#include <inttypes.h>

#include <WSWire.h>
#include <DS3232RTC.h>
#include <Time.h>
#include "barnacle_client.h"

#include <bstrlib.h>
#include <constable.h>
#include <conshell.h>

#include <memoryget.h>
#include <anmea.h>
#include <TinyGPS++.h>

#include <pololu_champ.h>

#include <radiocontrol.h>
#include <latlong.h>

#include <winch_control.h>

/** Global Variable instances
 ******************************************************************************
 */
 
typedef struct Waypoint{
	int32_t lon;
	int32_t lat;
}Waypoint;

int auto_mode = 0;

Waypoint waypoint[11];


// Instances necessary for command line usage
cons_line cli;
cmdlist functions;

cons_line xbee;
int rc_grease = 0;

// AIRMAR NMEA String buffer
//char airmar_buffer_char[80];
anmea_buffer_t airmar_buffer;
//anmea_tag_wiwmv_t airmar_nmea_wimwv_tag;

// Motor object definitions
pchamp_controller rudder_control; // Rudder
pchamp_servo rudder_servo[2];

pchamp_controller winch_control[2]; // Drum winches


// Global to track current mode of operation
uint16_t gaelforce = MODE_COMMAND_LINE;

// Software serial instances
SoftwareSerial* Serial4;
SoftwareSerial XBEE_SERIAL_PORT(51,52);

int incomingByte;

/// Initialise pin numbers and related calibration values, most values should
//be overwritten by eeprom during setup()
rc_mast_controller radio_controller = {
    { MAST_RC_RSX_PIN, 1900, 1100 },
    { MAST_RC_RSY_PIN, 1900, 1100 },
    { MAST_RC_LSX_PIN, 1900, 1100},
    { MAST_RC_LSY_PIN, 1900, 1100 },
    { MAST_RC_GEAR_PIN, 1900, 1100}
};

//Turn counter for the airmar tags.
int airmar_turn_counter;
const char *AIRMAR_TAGS[3] = {"$HCHDG","$WIMWV","$GPGGA"};
uint8_t NUMBER_OF_TAGS = 3;


//AIRMAR tags for holding data.
anmea_tag_hchdg_t head_tag;
anmea_tag_wiwmv_t wind_tag;
anmea_tag_gpgll_t gps_tag;


//Waypoint
int target_wp = 0;

//Gps update
TinyGPSPlus way_gps;
uint32_t way_gps_time;
int new_wp = 0;
int num_of_wps = 9;
/******************************************************************************
 */
 
void diagnostics( cons_line* cli );
void rmode_update_motors(
        rc_mast_controller* rc,
        pchamp_controller mast[2],
        pchamp_servo rudder[2] );

void setup() {
    //STANDARD WAYPOINTS
    waypoint[0].lat = 44227151;
    waypoint[0].lon = -76489489;
    waypoint[1].lat = 44226744;
    waypoint[1].lon = -76489163;
    waypoint[2].lat = 44226617;                   
    waypoint[2].lon = -76489664;
    waypoint[3].lat = 44227095;
    waypoint[3].lon = -76490488;
    waypoint[4].lat = 44227095;
    waypoint[4].lon = -76490488;
    waypoint[5].lat = 44227095;
    waypoint[5].lon = -76490488;
	waypoint[6].lat = 44227095;
    waypoint[6].lon = -76490488;
	waypoint[7].lat = 44227095;
    waypoint[7].lon = -76490488;
	waypoint[8].lat = 44227095;
    waypoint[8].lon = -76490488;
	//STATIONKEEP WAYPOINTS
	waypoint[9].lat = 44227095;
    waypoint[9].lon = -76490488;
	waypoint[10].lat = 44227095;
    waypoint[10].lon = -76490488;

    
    // Set initial config for all serial ports4
    SERIAL_PORT_CONSOLE.begin(SERIAL_BAUD_CONSOLE);
    delay(100);
    SERIAL_PORT_CONSOLE.println(F("Gaelforce starting up!"));
    SERIAL_PORT_CONSOLE.println(F("----------------------"));
    SERIAL_PORT_CONSOLE.print(F("Init all serial ports..."));
    // Set up the rest of the ports
    SERIAL_PORT_POLOLU.begin(SERIAL_BAUD_POLOLU);
    SERIAL_PORT_AIRMAR.begin(SERIAL_BAUD_AIRMAR);
    SERIAL_PORT_AIS.begin(SERIAL_BAUD_AIS);
    SERIAL_PORT_CONSOLE.println(F("OKAY!"));
	
    // NOTE: AUUUUUGGGHHH THIS IS DISGUSTING  
    SERIAL_PORT_BARN = new SoftwareSerial( SERIAL_SW4_RXPIN, SERIAL_SW4_TXPIN );
    SERIAL_PORT_BARN->begin( SERIAL_BAUD_BARNACLE_SW );
    barnacle_port = Serial4;
    // Its a global pointer defined in another library that needs to be set
    // even though it isn't obvious. Fix the library as soon as members are
    // able to!
    
    SERIAL_PORT_CONSOLE.print(F("Registering cli funcs..."));
    // Register all command line functions
    cons_cmdlist_init( &functions );
    cons_reg_cmd( &functions, "help", (void*) cabout );
    cons_reg_cmd( &functions, "dia", (void*) cdiagnostic_report );
    cons_reg_cmd( &functions, "mon", (void*) cmon );
    cons_reg_cmd( &functions, "lltest", (void*) latlongtest);
    cons_reg_cmd( &functions, "calrc", (void*) calrc );
    cons_reg_cmd( &functions, "mode", (void*) csetmode );
    cons_reg_cmd( &functions, "ee", (void*) ceeprom );
    cons_reg_cmd( &functions, "rc", (void*) crcd );
    cons_reg_cmd( &functions, "mot", (void*) cmot );
    cons_reg_cmd( &functions, "now", (void*) cnow );
    cons_reg_cmd( &functions, "res", (void*) cres );
	cons_reg_cmd( &functions, "airmar", (void*) cairmar );

    // Last step in the cli initialisation, command line ready
    //cons_init_line( &cli, &SERIAL_PORT_CONSOLE );

	cons_init_line( &cli, &SERIAL_PORT_CONSOLE);
    SERIAL_PORT_CONSOLE.println(F("OKAY!"));

    SERIAL_PORT_CONSOLE.print(F("Setting motor information..."));
    // Initialise servo motor information
    rudder_control.id = 11;
    rudder_control.line = &SERIAL_PORT_POLOLU;

    rudder_servo[0].channel_id = 0;
    rudder_servo[0].controller = &rudder_control;

    rudder_servo[1].channel_id = 2;
    rudder_servo[1].controller = &rudder_control;

    // Initialise dc motor information
    winch_control[0].id = 12;
    winch_control[0].line = &SERIAL_PORT_POLOLU;

    winch_control[1].id = 13;
    winch_control[1].line = &SERIAL_PORT_POLOLU;

    SERIAL_PORT_CONSOLE.println(F("OKAY!"));

    pinMode( BARNACLE_RESET_PIN, OUTPUT );

    SERIAL_PORT_CONSOLE.print(F("Barnacle reboot (1)..."));
    reset_barnacle();
    SERIAL_PORT_CONSOLE.println(F("OKAY!"));

    SERIAL_PORT_CONSOLE.print(F("Init wire network..."));
    Wire.begin();
    SERIAL_PORT_CONSOLE.println(F("OKAY!"));

    SERIAL_PORT_CONSOLE.print(F("Setting system time..."));
    // Time set
    // the function to get the time from the RTC
    delay(100);
    setSyncProvider(RTC.get);
    if(timeStatus() != timeSet) {
        cli.port->println(F("Unable to sync with the RTC"));
    } else {
        cli.port->println(F("RTC has set the system time"));
    }
    display_time( cli.port );
    SERIAL_PORT_CONSOLE.println(F("OKAY!"));

    // Initialize the airmar buffer state
	SERIAL_PORT_AIRMAR.println("$PAMTC,EN,ALL,0");	//Disable all
	SERIAL_PORT_AIRMAR.println("$PAMTC,EN,GGA,1,10");	//tag, enable, target tag, [0 (dis) / 1 (en)], period (1s/10)
	SERIAL_PORT_AIRMAR.println("$PAMTC,EN,HDG,1,10");
	SERIAL_PORT_AIRMAR.println("$PAMTC,EN,MWVR,1,10");	//Use relative ie. apparent wind
	//SERIAL_PORT_AIRMAR.println("$PAMTC,EN,S");		//Save to eeprom
    airmar_buffer.state = ANMEA_BUF_SEARCHING;
    airmar_buffer.data  = bfromcstralloc( AIRMAR_NMEA_STRING_BUFFER_SIZE, "" );

    SERIAL_PORT_CONSOLE.print(F("Resetting barnacle tick counters..."));
    // Reset encoder counters to 0
    delay(100);
    barn_clr_w1_ticks();
    barn_clr_w2_ticks();
    SERIAL_PORT_CONSOLE.println(F("OKAY!"));

    SERIAL_PORT_CONSOLE.print(F("Barnacle reboot (2)..."));
    reset_barnacle();
    SERIAL_PORT_CONSOLE.println(F("OKAY!"));
	
	rc_read_calibration_eeprom( 0x08, &radio_controller );
	Serial.print(F("RC Settings Loaded - "));
	rc_print_calibration( &Serial , &radio_controller);

    // Yeah!
    cli.port->print(F("Initialisation complete, awaiting commands"));

    // Print the prefix to the command line, the return code from the previous
    // function doesn't exist, so default to zero
    print_cli_prefix( &cli, 0 );

		
	XBEE_SERIAL_PORT.begin(SERIAL_BAUD_XBEE);
	delay(100);

	
	delay(100);

}

/** Polling loop definition
 ******************************************************************************
 */
void loop() {
    static int res;
	static int res_xbee;
	blist list;

	//XBEE commands, used to wirelessly communicate with the boat
	if (XBEE_SERIAL_PORT.available() >0){
		// read the oldest byte in the serial buffer:
		incomingByte = XBEE_SERIAL_PORT.read() - 64;
		//For some bullshit reason, this works
		//incomingByte = incomingByte - 64;
		//XBEE_SERIAL_PORT.println(F("QUE?!"));
		//Lock motor
		XBEE_SERIAL_PORT.print("Incoming Byte: ");
		XBEE_SERIAL_PORT.println(incomingByte);
		if (incomingByte == 'L') {
			motor_lock();
		    XBEE_SERIAL_PORT.println(F("MOTOR LOCKED!"));
		}
		//Unlock motor
		else if(incomingByte == 'U'){
			motor_unlock();
			XBEE_SERIAL_PORT.println(F("MOTOR UNLOCKED!"));
		}
		//Calibrate RC, 3 stages
		else if(incomingByte == 'R'){
			XBEE_SERIAL_PORT.println(F("Set RC and press X!"));
			while(incomingByte != 'X'){
				incomingByte = XBEE_SERIAL_PORT.read() -64;
			}
			// Up
			radio_controller.rsy.high = rc_get_raw_analog( radio_controller.rsy );
			radio_controller.lsy.high = rc_get_raw_analog( radio_controller.lsy );
			
			XBEE_SERIAL_PORT.println(F("Set RC and press Z!"));
			while(incomingByte != 'Z'){
				incomingByte = XBEE_SERIAL_PORT.read() -64;
			}
			// Down
			radio_controller.rsy.low = rc_get_raw_analog( radio_controller.rsy );
			radio_controller.lsy.low = rc_get_raw_analog( radio_controller.lsy );

			XBEE_SERIAL_PORT.println(F("Calibration values set"));
		}
		//RC mode
		else if(incomingByte == 'C'){
			gaelforce = MODE_RC_CONTROL;
			XBEE_SERIAL_PORT.println(F("MODE SET RC!"));
		}
		//Command line mode
		else if(incomingByte == 'M'){
			gaelforce = MODE_COMMAND_LINE;
			XBEE_SERIAL_PORT.println(F("MODE SET CLI!"));
		}
		else if(incomingByte == 'Q')
		{
			display_waypoints();
		}
		//Autosail mode
		else if(incomingByte == 'A'){
			gaelforce = MODE_AUTOSAIL;
			XBEE_SERIAL_PORT.println(F("MODE SET AUTO!"));
		}
		//Tries to add a gps point
		else if(incomingByte == 'W'){
			set_waypoint();
		}
		else if(incomingByte == 'P'){
			way_plus();
		}		
		else if(incomingByte == 'O'){
			way_minus();
		}
		else if(incomingByte == '+'){
			target_plus();
		}	
		else if(incomingByte == '-'){
			target_minus();
		}
		else if(incomingByte == 'K'){
			num_of_wps++;
			num_of_wps%9;
			XBEE_SERIAL_PORT.print("Num of wps: ");
			XBEE_SERIAL_PORT.println(num_of_wps);
		}
		else if(incomingByte == 'V'){
			auto_mode++;
			auto_mode%3;
			XBEE_SERIAL_PORT.print("Auto mode: ");
			XBEE_SERIAL_PORT.println(auto_mode);
		}
	}

    if( gaelforce & MODE_COMMAND_LINE ) {
        res = cons_poll_line( &cli, CONSHELL_CLI_TIMEOUT );
        if( res == CONSHELL_LINE_END ) {
            res = cons_search_exec( &cli, &functions );
            print_cli_prefix( &cli, res );
            cons_clear_line( &cli );
        }
    }

    if( gaelforce & MODE_DIAGNOSTICS_OUTPUT ) {
        static uint32_t diagnostics_last = millis();
        if( (millis() - diagnostics_last) > 1000 ) {
            diagnostics_last = millis();
            diagnostics( &cli );
            print_cli_prefix( &cli, res );
            cli.port->print( (char*) cli.line->data );
        }
    }

    if( gaelforce & MODE_RC_CONTROL ) {
		rmode_update_motors(
				&radio_controller,
				winch_control,
				rudder_servo
			);
		/*if((millis() - rc_timer) > 9000){
			gps_encode();
			rc_timer = millis();
		}*/
    }
    
  
    if( gaelforce & MODE_AUTOSAIL ) {
 
		  autosail_main();
    }

}
/******************************************************************************
 */

/** Command line prefix generation command
 ******************************************************************************
 * Call this every time a command completes
 */
void print_cli_prefix( cons_line* cli, int res ) {
    Stream* line = cli->port;

    line->println();
    line->print('[');
    line->print( res == CONSHELL_FUNCTION_NOT_FOUND ? '0' : '1' );
    line->print('|');

    if( gaelforce & MODE_RC_CONTROL ) {
        line->print(F("RC|"));
    }

    if( gaelforce & MODE_AUTOSAIL ) {
        line->print(F("AUTO|"));
    }

    if( gaelforce & MODE_DIAGNOSTICS_OUTPUT ) {
        line->print(F("DIA|"));
    }

    line->print( getAvailableMemory() );
    line->print(F("]> "));

}
/** Waypoint functions
 ******************************************************************************
 */
 
void set_waypoint(){
	way_gps_time = millis();
	while(way_gps.location.isUpdated() == 0 && ( millis() - way_gps_time < 6000)){
		if (Serial3.available()){
			way_gps.encode(Serial3.read());
		}
	}
	if(millis() - way_gps_time < 6000){
		waypoint[new_wp].lat = way_gps.location.lat() * 1000000;
		waypoint[new_wp].lon = way_gps.location.lng() * 1000000;
		XBEE_SERIAL_PORT.println(F("WAYPOINT ADDED!"));
		XBEE_SERIAL_PORT.println(waypoint[new_wp].lat);
		XBEE_SERIAL_PORT.println(waypoint[new_wp].lon);
	}
	else{
		XBEE_SERIAL_PORT.println(F("WAYPOINT NOT ADDED!"));
	}
}
void target_plus(){
	if(target_wp < 6){
		target_wp++;
	}
	XBEE_SERIAL_PORT.println(F("TARGET WP + 1"));
	XBEE_SERIAL_PORT.println(target_wp);
}
void target_minus(){
	if(target_wp != 0){
		target_wp--;
	}
	XBEE_SERIAL_PORT.println(F("TARGET WP - 1"));
	XBEE_SERIAL_PORT.println(target_wp);
}
void way_plus(){
	if(new_wp < 6){
		new_wp++;
	}
	XBEE_SERIAL_PORT.println(F("WP + 1"));
	XBEE_SERIAL_PORT.println(new_wp);
}
void way_minus(){
	if(new_wp != 0){
		new_wp--;
	}
	XBEE_SERIAL_PORT.println(F("WP - 1"));
	XBEE_SERIAL_PORT.println(new_wp);
}

/** System diagnostics
 ******************************************************************************
 */
void
diagnostics( cons_line* cli )
{
    Stream* con = cli->port; // Short for con(sole)
    char buf[120] = { '\0' };
    uint32_t uptime[2];
    uint16_t req_value; //Requested value


    // Print the current time
    con->println();
    display_time( cli->port );

    // Check connection to pololus
	//Motor 0 disattached
	
    req_value =
        pchamp_request_value( &(winch_control[0]), PCHAMP_DC_VAR_TIME_LOW );
    uptime[0] = req_value & 0xFFFF;
    req_value =
        pchamp_request_value( &(winch_control[0]), PCHAMP_DC_VAR_TIME_HIGH );
    uptime[0] += req_value * 65536ULL;
	
    /*req_value =
        pchamp_request_value( &(winch_control[1]), PCHAMP_DC_VAR_TIME_LOW );
    uptime[1] = req_value & 0xFFFF;
    req_value =
        pchamp_request_value( &(winch_control[1]), PCHAMP_DC_VAR_TIME_HIGH );
    uptime[1] += req_value * 65536ULL;*/

    // Report for controller 0
    snprintf_P( buf,
            sizeof(buf),
            PSTR(
                "Pololu controllers\n"
                "M0: Uptime -> %lu msec\n"
                "    Voltag -> %u mV\n"
                "    TEMP   -> %u\n"
                "    ERRORS -> 0x%02x\n"
            ),
            uptime[0],
            pchamp_request_value( &(winch_control[0]), PCHAMP_DC_VAR_VOLTAGE ),
            pchamp_get_temperature( &(winch_control[0]) ),
            pchamp_request_value( &(winch_control[0]), PCHAMP_DC_VAR_ERROR )
        );
    con->print( buf );
    delay(100);

    // Report for controller 1
    /*snprintf_P( buf,
            sizeof(buf),
            PSTR(
                "M1: Uptime -> %lu msec\n"
                "    Voltag -> %u mV\n"
                "    TEMP   -> %u\n"
                "    ERRORS -> 0x%02x\n"
            ),
            uptime[1],
            pchamp_request_value( &(winch_control[1]), PCHAMP_DC_VAR_VOLTAGE ),
            pchamp_get_temperature( &(winch_control[0]) ),
            pchamp_request_value( &(winch_control[1]), PCHAMP_DC_VAR_ERROR )
        );
    con->print( buf );*/

    // Report for rudder controller

    // Check values from attopilot units
    con->println(F("Power box"));
    snprintf_P( buf,
            sizeof(buf),
            PSTR(
                "BATTERY: Voltage -> %u\n"
                "         Current -> %u\n"
            ),
            barn_get_battery_voltage(),
            barn_get_battery_current()
        );
    con->print( buf );

    snprintf_P( buf,
            sizeof(buf),
            PSTR(
                "CHARGER: Voltage -> %u\n"
                "         Current -> %u\n"
            ),
            barn_get_charger_voltage(),
            barn_get_charger_current()
        );
    con->print( buf );

    snprintf_P( buf, sizeof(buf),
            PSTR("ENC: W1: %u T1: %u\n"
                 "     W2: %u T2: %u\n"),
            barn_get_w1_ticks(),
            0,
            barn_get_w2_ticks(),
            0
        );
    con->print( buf );

    snprintf_P( buf, sizeof(buf),
            PSTR("BARNACLE_LATENCY: %lu\n"),
            barn_check_latency()
            );
    con->print(buf);
}
/******************************************************************************
 */

 
void display_waypoints(){
	XBEE_SERIAL_PORT.print(waypoint[0].lat);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[0].lon);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[1].lat);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[1].lon);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[2].lat);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[2].lon);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[3].lat);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[3].lon);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[4].lat);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[4].lon);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[5].lat);
	XBEE_SERIAL_PORT.println('!');
	XBEE_SERIAL_PORT.print(waypoint[5].lon);
	XBEE_SERIAL_PORT.println('!');
}
void display_time( Stream* com )
{
    char buf[20];
    snprintf_P( buf, sizeof(buf),
            PSTR(
                "%d %d %d %02d:%02d:%02d\n"
            ),
            year(),
            month(),
            day(),
            hour(),
            minute(),
            second()
        );
    com->print( buf );
}

void reset_barnacle() {
    digitalWrite( BARNACLE_RESET_PIN, LOW );
    delay(250);
    digitalWrite( BARNACLE_RESET_PIN, HIGH );
    delay(250);
}
#endif // Include guard
// vim:ft=c:


/******************************************************************************
 */
