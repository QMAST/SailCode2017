
/** Skoonercode

   Supercedes ferrycode, I thought the name looked cooler than Schoonercode

   Uses a simple polling loop with timeouts enforced by the functions
   themselves. Each function is passed the amount of time it is allowed to take
   for execution, and either exits as soon as possible, or attempts to perform
   its function for the entire time until it realises it needs to exit.

   Main interface, command line functions. No function may lock the polling
   loop into a single function. Certain polling functions may be
   enabled/disabled on the fly.
*/
#ifndef _SKOONERCODE_INO
#define _SKOONERCODE_INO

#include "pins.h"
#include <SoftwareSerial.h>

#include <avr/pgmspace.h>
#include <inttypes.h>

#include <WSWire.h>
#include <DS3232RTC.h>
#include <TimeLib.h>
#include <Time.h>
#include <BarnacleDriver.h>
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

typedef struct Waypoint {
  int32_t lon;
  int32_t lat;
} Waypoint;

uint16_t autosail_type = 0;

//Waypoint
#define NUMBER_OF_WAYPOINTS 20
Waypoint waypoint[20];
int target_wp = 0;
int start_wp_index = 0;
int finish_wp_index = 0;

// LED Strip Globals
rgb_color colorOff;
rgb_color colorRed;
rgb_color colorGreen;

Pixy pixy;

uint32_t gps_update_time = 0;
uint32_t update_one_second = 0;
uint32_t update_five_seconds = 0;

// Instances necessary for command line usage
cons_line cli;
cmdlist functions;

int rc_grease = 0;

// AIRMAR NMEA String buffer
//char airmar_buffer_char[80];
anmea_buffer_t airmar_buffer;
//anmea_tag_wiwmv_t airmar_nmea_wimwv_tag;

// Motor object definitions
pchamp_controller winch_control[1]; // Drum winches
int currentPosition = 0; // From 0 to 5000 depending on current position, 0 is fully in, 5 is fully out
void count(void);


//Setup barnacle serial
SoftwareSerial SERIAL_PORT_BARN(BARNACLE_RX_PIN, BARNACLE_TX_PIN);
BarnacleDriver *barnacle_client = new BarnacleDriver(SERIAL_PORT_BARN);

// Global to track current mode of operation
uint16_t gaelforce = MODE_COMMAND_LINE;


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
#define NUMBER_OF_TAGS 3
const char *AIRMAR_TAGS[NUMBER_OF_TAGS] = {"$HCHDG", "$WIMWV", "$GPGGA"};

//AIRMAR tags for holding data.
anmea_tag_hchdg_t head_tag;
anmea_tag_wiwmv_t wind_tag;
anmea_tag_gpgll_t gps_tag;
anmea_tag_gpvtg_t speed_tag;

//Gps update
TinyGPSPlus way_gps;
uint32_t way_gps_time;
int new_wp = 0;
/******************************************************************************
*/

void diagnostics( cons_line* cli );
void rmode_update_motors(
  rc_mast_controller* rc,
  pchamp_controller mast[2]);

void
rmode_winch(
  rc_mast_controller* rc);

void setup() {
  initComms();
  SERIAL_PORT_CONSOLE.begin(SERIAL_BAUD_CONSOLE);
  SERIAL_PORT_XBEE.begin(SERIAL_BAUD_XBEE);
  SERIAL_PORT_AIRMAR.begin(SERIAL_BAUD_AIRMAR);
  SERIAL_PORT_POLOLU_HPMC.begin(SERIAL_BAUD_POLOLU_HPMC);
  SERIAL_PORT_BARN.begin(SERIAL_BAUD_BARNACLE_SW);
  delay(500);

  SERIAL_PORT_XBEE.print(";");
  sendTransmissionAndPrint(10, 204, F("GO"));
  delay(100); // Give half a second so any xbee devices can show bootup state
  sendTransmissionAndPrint(10, 204, F("Powered On."));

  // Initialize LED strip
  sendTransmissionAndPrint(10, 204, F("Initializing LED strip..."));
  initLEDStrip();
  setColors(1, LED_COUNT, colorRed);
  updateLEDs();
  SERIAL_PORT_CONSOLE.println(F("OKAY!"));

  //STANDARD WAYPOINTS
  for (int i = 0; i < NUMBER_OF_WAYPOINTS; i++) {
    waypoint[i].lat = 44227151;
    waypoint[i].lon = -76489489;
  }

  target_wp = 3;
  start_wp_index = 0;
  finish_wp_index = 13;

  waypoint[0].lat = 38982630;
  waypoint[0].lon = -76476790;

  waypoint[1].lat = 38982430;
  waypoint[1].lon = -76476290;

  waypoint[2].lat = 38982130;
  waypoint[2].lon = -76475690;

  waypoint[3].lat = 38983570;
  waypoint[3].lon = -76474500;

  waypoint[4].lat = 38984000;
  waypoint[4].lon = -76474960;

  waypoint[5].lat = 38984250;
  waypoint[5].lon = -76475360;

  waypoint[6].lat = 38984600;
  waypoint[6].lon = -76475760;

  waypoint[7].lat = 38985000;
  waypoint[7].lon = -76476260;

  waypoint[8].lat = 38985300;
  waypoint[8].lon = -76476760;

  waypoint[9].lat = 38985730;
  waypoint[9].lon = -76477460;

  waypoint[10].lat = 38984030;
  waypoint[10].lon = -76479000;

  waypoint[11].lat = 38983670;
  waypoint[11].lon = -76478490;

  waypoint[12].lat = 38983330;
  waypoint[12].lon = -76477890;

  waypoint[13].lat = 38982930;
  waypoint[13].lon = -76477290;

  sendTransmissionAndPrint(10, 204, F("Initializing Pixy..."));
  pixy.init();
  SERIAL_PORT_CONSOLE.println(F("OKAY!"));

  // Register all command line functions
  sendTransmissionAndPrint(10, 204, F("Initializing cli functions..."));
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
  cons_init_line( &cli, &SERIAL_PORT_CONSOLE);
  SERIAL_PORT_CONSOLE.println(F("OKAY!"));

  // Initialize servos
  sendTransmissionAndPrint(10, 204, F("Initializing servos..."));
  initServo();
  SERIAL_PORT_CONSOLE.println(F("OKAY!"));

  // Initialize motors
  sendTransmissionAndPrint(10, 204, F("Initializing motor information..."));
  //pinMode(WINCH_ENCODER_A_PIN, INPUT);
  //pinMode(WINCH_ENCODER_B_PIN, INPUT);
  //attachInterrupt(digitalPinToInterrupt(WINCH_ENCODER_A_PIN), winchEncoderCount, RISING);

  // Initialise servo motor information
  /*rudder_control.id = 11;
    rudder_control.line = &SERIAL_PORT_POLOLU;

    rudder_servo[0].channel_id = 0;
    rudder_servo[0].controller = &rudder_control;

    rudder_servo[1].channel_id = 2;
    rudder_servo[1].controller = &rudder_control;*/

  // Initialise dc motor information
  winch_control[0].id = 13;
  winch_control[0].line = &SERIAL_PORT_POLOLU_HPMC;

  /*// Initialize secondary Arduino
    sendTransmissionAndPrint(10, 204, F("Initializing barnacle..."));
    pinMode( BARNACLE_RESET_PIN, OUTPUT );
    sendTransmissionAndPrint(10, 204, F("Rebooting barnacle (1 of 2)..."));
    reset_barnacle();
    SERIAL_PORT_CONSOLE.println(F("OKAY!"));*/

  // Initialize wire network
  // TODO: Figure out what the wire network is
  sendTransmissionAndPrint(10, 204, F("Initializing wire network..."));
  Wire.begin();
  SERIAL_PORT_CONSOLE.println(F("OKAY!"));

  // Set system time
  sendTransmissionAndPrint(10, 204, F("Setting system time..."));
  // the function to get the time from the RTC
  setSyncProvider(RTC.get);
  if (timeStatus() != timeSet) {
    sendTransmissionAndPrint(10, 204, F("Unable to sync with the RTC"));
  } else {
    sendTransmissionAndPrint(10, 204, F("RTC has set the system time"));
  }
  display_time(cli.port);
  SERIAL_PORT_CONSOLE.println(F("OKAY!"));


  /*// Reset encoder counters to 0
    sendTransmissionAndPrint(10, 204, F("Resetting barnacle tick counters..."));
    SERIAL_PORT_BARN.listen();
    barnacle_client->barn_clr_w1_ticks();
    barnacle_client->barn_clr_w2_ticks();
    SERIAL_PORT_CONSOLE.println(F("OKAY!"));

    // Reboot barnacle to reflect new encoder counters
    sendTransmissionAndPrint(10, 204, F("Rebooting barnacle (2 of 2)..."));
    reset_barnacle();
    SERIAL_PORT_CONSOLE.println(F("OKAY!"));*/

  // Load remote control
  sendTransmissionAndPrint(10, 204, F("Initializing remote control..."));
  rc_read_calibration_eeprom( 0x08, &radio_controller );
  sendTransmissionAndPrint(10, 204, F("Loaded RC settings..."));
  rc_print_calibration( &Serial , &radio_controller);
  SERIAL_PORT_CONSOLE.println(F("OKAY!"));

  // Initialize the airmar buffer state
  delay(100);
  sendTransmissionAndPrint(10, 204, F("Initializing Airmar..."));
  delay(100);
  SERIAL_PORT_AIRMAR.println("$PAMTC,EN,ALL,0");  //Disable all
  SERIAL_PORT_AIRMAR.println("$PAMTC,EN,GGA,1,10"); //tag, enable, target tag, [0 (dis) / 1 (en)], period (1s/10)
  SERIAL_PORT_AIRMAR.println("$PAMTC,EN,HDG,1,10");
  SERIAL_PORT_AIRMAR.println("$PAMTC,EN,MWVR,1,10");  //Use relative ie. apparent wind
  //SERIAL_PORT_AIRMAR.println("$PAMTC,EN,VTG,1,10");
  SERIAL_PORT_AIRMAR.println("$PAMTC,EN,S");    //Save to eeprom
  airmar_buffer.state = ANMEA_BUF_SEARCHING;
  airmar_buffer.data  = bfromcstralloc( AIRMAR_NMEA_STRING_BUFFER_SIZE, "" );
  SERIAL_PORT_CONSOLE.println(F("OKAY!"));

  // Yeah!
  sendTransmission(10, 204, F("\nReady."));
  sendTransmission(10, 200, F("Ready."));
  cli.port->print(F("Initialization complete, awaiting commands"));

  // Print the prefix to the command line, the return code from the previous
  // function doesn't exist, so default to zero
  print_cli_prefix( &cli, 0 );

  setColors(1, LED_COUNT, colorOff);
  updateLEDs();
  delay(100);

  gaelforce = MODE_RC_CONTROL;
  if ( gaelforce & MODE_COMMAND_LINE) {
    sendTransmission(21, 200, F("0"));
  } else if ( gaelforce & MODE_RC_CONTROL ) {
    sendTransmission(21, 200, F("1"));
  } else if (gaelforce & MODE_AUTOSAIL) {
    sendTransmission(21, 200, F("2"));
  }
}

/** Polling loop definition
 ******************************************************************************
*/

void loop() {
  static int res;
  blist list;

  currentPosition = analogRead(1);

  xBeeUpdateAndExecute();
  /*// Send pixy servo position
    sendTransmissionStart(60, 200);
    SERIAL_PORT_XBEE.print(getServoDeg(PIXY_SERVO_CHANNEL));
    sendTransmissionEnd();*/
  update_airmar_tags();
  updateWinchPosition();

  // update and report location/diagnostics to device
  if (millis() - update_one_second > 1000) {
    update_one_second = millis();



    //Serial.println(wind_tag.wind_speed);
    //Serial.println(wind_tag.wind_angle);
    //sendTransmission(55, 200, String(encoderValue));

    // Send heading
    sendTransmissionStart(13, 200);
    SERIAL_PORT_XBEE.print(((head_tag.mag_angle_deg / 10) % 360));
    sendTransmissionEnd();

    // Send uptime
    sendTransmission(14, 200, String(millis() / 1000));

    // Send available memory
    sendTransmission(15, 200, String(getAvailableMemory()));

    /*char buf[120] = { '\0' };
      SERIAL_PORT_BARN.listen();
      snprintf_P( buf, sizeof(buf),
                PSTR("ENC: W1: %d\n"),
                barnacle_client->barn_get_w1_ticks());
      Serial.print( buf );

      /*snprintf_P( buf, sizeof(buf),
                PSTR("BARNACLE_LATENCY: %d\n"),
                barnacle_client->barn_check_latency()
              );
      Serial.print(buf);*/
  }

  if (millis() - update_five_seconds > 5000) {
    update_five_seconds = millis();
    //sendTransmission(51, 200, String(barn_get_battery_voltage()));
    //sendTransmission(52, 200, String(barn_get_battery_current()));
    //sendTransmission(53, 200, String(barn_get_charger_voltage()));
    //sendTransmission(54, 200, String(barn_get_charger_current()));

    // Send GPS conditions
    if (gps_tag.latitude != 0 && gps_tag.longitude != 0) {
      // Report data to device
      sendTransmissionStart(11, 200);
      SERIAL_PORT_XBEE.print((double)gps_tag.latitude * (int)gps_tag.sign_lat);
      SERIAL_PORT_XBEE.print(" ");
      SERIAL_PORT_XBEE.print((double)gps_tag.longitude * (int)gps_tag.sign_lon);
      sendTransmissionEnd();
    } else {
      sendTransmission(11, 200, F(""));
    }

    // Send mode
    if ( gaelforce & MODE_COMMAND_LINE) {
      sendTransmission(21, 200, F("0"));
    } else if ( gaelforce & MODE_RC_CONTROL ) {
      sendTransmission(21, 200, F("1"));
    } else if (gaelforce & MODE_AUTOSAIL) {
      sendTransmission(21, 200, F("2"));
    }

    sendTransmissionStart(50, 200);
    SERIAL_PORT_XBEE.print(((pchamp_get_temperature( &(winch_control[0]) ) / 10.0)));
    sendTransmissionEnd();
  }

  if ( gaelforce & MODE_COMMAND_LINE ) {
    res = cons_poll_line( &cli, CONSHELL_CLI_TIMEOUT );
    if ( res == CONSHELL_LINE_END ) {
      res = cons_search_exec( &cli, &functions );
      print_cli_prefix( &cli, res );
      cons_clear_line( &cli );
    }
  }

  if ( gaelforce & MODE_DIAGNOSTICS_OUTPUT ) {
    static uint32_t diagnostics_last = millis();
    if ( (millis() - diagnostics_last) > 1000 ) {
      diagnostics_last = millis();
      diagnostics( &cli );
      print_cli_prefix( &cli, res );
      cli.port->print( (char*) cli.line->data );
    }
  }

  if ( gaelforce & MODE_RC_CONTROL ) {
    rmode_update_motors(
      &radio_controller,
      winch_control
    );
    /*if((millis() - rc_timer) > 9000){
      gps_encode();
      rc_timer = millis();
      }*/
  }


  if ( gaelforce & MODE_AUTOSAIL ) {
    update_airmar_tags();
    autonomous_main();
    xBeeUpdateAndExecute();
    rmode_winch(&radio_controller);
  }
}
/******************************************************************************
*/

/** Command line prefix generation command
 ******************************************************************************
   Call this every time a command completes
*/
void print_cli_prefix( cons_line* cli, int res ) {
  Stream* line = cli->port;

  line->println();
  line->print('[');
  line->print( res == CONSHELL_FUNCTION_NOT_FOUND ? '0' : '1' );
  line->print('|');

  if ( gaelforce & MODE_RC_CONTROL ) {
    line->print(F("RC|"));
  }

  if ( gaelforce & MODE_AUTOSAIL ) {
    line->print(F("AUTO|"));
  }

  if ( gaelforce & MODE_DIAGNOSTICS_OUTPUT ) {
    line->print(F("DIA|"));
  }

  line->print( getAvailableMemory() );
  line->print(F("]> "));

}
/** Waypoint functions
 ******************************************************************************
*/

void set_waypoint() {
  way_gps_time = millis();
  while (way_gps.location.isUpdated() == 0 && ( millis() - way_gps_time < 6000)) {
    if (Serial3.available()) {
      way_gps.encode(Serial3.read());
    }
  }
  if (millis() - way_gps_time < 6000) {
    waypoint[new_wp].lat = way_gps.location.lat() * 1000000;
    waypoint[new_wp].lon = way_gps.location.lng() * 1000000;
    sendTransmissionStart(10, 201);
    SERIAL_PORT_XBEE.print(F("Waypoint added at "));
    SERIAL_PORT_XBEE.print(waypoint[new_wp].lat);
    SERIAL_PORT_XBEE.print(",");
    SERIAL_PORT_XBEE.print(waypoint[new_wp].lon);
    sendTransmissionEnd();
  }
  else {
    sendTransmission(10, 201, F("Waypoint not added"));
  }
}
/*void target_plus() {
  if (target_wp < 6) {
    target_wp++;
  }
  XBEE_SERIAL_PORT.println(F("TARGET WP + 1"));
  XBEE_SERIAL_PORT.println(target_wp);
  }
  void target_minus() {
  if (target_wp != 0) {
    target_wp--;
  }
  XBEE_SERIAL_PORT.println(F("TARGET WP - 1"));
  XBEE_SERIAL_PORT.println(target_wp);
  }
  void way_plus() {
  if (new_wp < 6) {
    new_wp++;
  }
  XBEE_SERIAL_PORT.println(F("WP + 1"));
  XBEE_SERIAL_PORT.println(new_wp);
  }
  void way_minus() {
  if (new_wp != 0) {
    new_wp--;
  }
  XBEE_SERIAL_PORT.println(F("WP - 1"));
  XBEE_SERIAL_PORT.println(new_wp);
  }*/

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
              PSTR("BARNACLE_LATENCY: %lu\n"),
              barn_check_latency()
            );
  con->print(buf);
}
/******************************************************************************
*/

void update_airmar_tags() {
  anmea_poll_string(
    &SERIAL_PORT_AIRMAR,
    &airmar_buffer,
    AIRMAR_TAGS[airmar_turn_counter]
  );
  if ( airmar_buffer.state == ANMEA_BUF_MATCH ) {
    if (airmar_turn_counter == 0) {
      Serial.println(F("Updating heading"));
      anmea_update_hchdg(&head_tag, airmar_buffer.data);
    }

    else if (airmar_turn_counter == 1) {
      Serial.println(F("Updating wind"));
      anmea_update_wiwmv(&wind_tag, airmar_buffer.data);
      // Send wind conditions
      sendTransmissionStart(12, 200);
      SERIAL_PORT_XBEE.print(wind_tag.wind_speed / (double)10);
      SERIAL_PORT_XBEE.print(F(" "));
      SERIAL_PORT_XBEE.print(wind_tag.wind_angle / (double)10);
      sendTransmissionEnd();
    } else if (airmar_turn_counter == 2) {
      Serial.println(F("Updating GPS"));
      anmea_update_gpgll(&gps_tag, airmar_buffer.data);
    } else if (airmar_turn_counter == 3) {
      Serial.println(F("Updating Speed"));
      anmea_update_gpvtg(&speed_tag, airmar_buffer.data);
      sendTransmissionStart(16, 200);
      SERIAL_PORT_XBEE.print((speed_tag.s_knots / 10));
      sendTransmissionEnd();
    }

    airmar_turn_counter++;
    airmar_turn_counter = airmar_turn_counter % 3;

    // autosail_print_tags();

    anmea_poll_erase( &airmar_buffer );
    //Serial.print("freeMemory()=");
    //Serial.println(getAvailableMemory());
  }
}

void display_waypoints() {
  for (int i = 0; i < NUMBER_OF_WAYPOINTS; i++) {
    sendTransmissionStart(10, 202);
    SERIAL_PORT_XBEE.print(i);
    SERIAL_PORT_XBEE.print(" ");
    SERIAL_PORT_XBEE.print(waypoint[i].lat);
    SERIAL_PORT_XBEE.print(" ");
    SERIAL_PORT_XBEE.print(waypoint[i].lon);
    sendTransmissionEnd();
  }
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
  sendTransmissionAndPrint(10, 202, String(buf));
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
