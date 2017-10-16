#ifndef _PINS_H
#define _PINS_H

#include <inttypes.h>

#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
#define _MEGA
#endif

#define DEBUG

// This function uses a lot of ram for something used rarely
#define RC_CALIBRATION

// Amount of memory to reserve for NMEA string build
//prev 80
#define AIRMAR_NMEA_STRING_BUFFER_SIZE 80

#define SERIAL_PORT_CONSOLE Serial
#define SERIAL_PORT_POLOLU  Serial1
#define SERIAL_PORT_AIS     Serial2
#define SERIAL_PORT_AIRMAR  Serial3
#define SERIAL_PORT_BARN    Serial4
#define SERIAL_PORT_LCD_SW  Serial5
//#define SERIAL_PORT_XBEE    Serial6

#define SERIAL_BAUD_XBEE 115200
#define SERIAL_BAUD_CONSOLE 57600
#define SERIAL_BAUD_AIRMAR  4800
#define SERIAL_BAUD_AIRMAR_BOOST 38400
#define SERIAL_BAUD_AIS     38400
#define SERIAL_BAUD_POLOLU  9600
#define SERIAL_BAUD_BARNACLE_SW 19200

#define SERIAL_SW4_RXPIN    10
#define SERIAL_SW4_TXPIN    11

// Max and min usec values that can be sent to rudder servo motors
#define POLOLU_SERVO_0_RUD_MIN 1000
#define POLOLU_SERVO_0_RUD_MAX 1600
#define POLOLU_SERVO_2_RUD_MIN 1050
#define POLOLU_SERVO_2_RUD_MAX 1650

//Mid 0 = 1003 mid 2 = 1640
#define BARNACLE_RESET_PIN 7

/*Default Pin Values*/
// Plug each channel from the receiver in to each of these arduino pins
#define MAST_RC_RSX_PIN 22 // Channel 1
#define MAST_RC_RSY_PIN 23 // Channel 2
#define MAST_RC_LSY_PIN 24 // Channel 3
#define MAST_RC_LSX_PIN 25  // Channel 4

#define MAST_RC_GEAR_PIN 26 // Channel 5

#define XBEE_RX 51
#define XBEE_TX 52



/** Mode of operation definitions
 ******************************************************************************
 */
typedef enum {
    MODE_COMMAND_LINE = 0x1,
    MODE_MOTOR_TEST = 0x2,
    MODE_RC_CONTROL = 0x4,
    MODE_AUTOSAIL = 0x8,
    MODE_DIAGNOSTICS_OUTPUT = 0x10,
	MODE_WAYPOINT = 0x20
} gaelforce_mode_t;
/******************************************************************************
 */

/** EEPROM Memory mapping
 ******************************************************************************
 */
#define EE_RC_SETTINGS  0x08
#define EE_TEST_LOC     EE_RC_SETTINGS + sizeof( rc_mast_controller ) + 1
/******************************************************************************
 */
#endif
