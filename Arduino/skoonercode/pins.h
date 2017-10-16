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
#define AIRMAR_NMEA_STRING_BUFFER_SIZE 80

#define SERIAL_PORT_CONSOLE         Serial
#define SERIAL_PORT_XBEE            Serial3
#define SERIAL_PORT_POLOLU_HPMC     Serial1
#define SERIAL_PORT_AIRMAR          Serial2
//#define SERIAL_PORT_LCD_SW  Serial5

#define SERIAL_BAUD_XBEE 57600
#define SERIAL_BAUD_CONSOLE 57600
#define SERIAL_BAUD_AIRMAR  4800
#define SERIAL_BAUD_AIRMAR_BOOST 38400
#define SERIAL_BAUD_AIS     38400
#define SERIAL_BAUD_BARNACLE_SW 19200
#define SERIAL_BAUD_POLOLU_MAESTRO  9600
#define SERIAL_BAUD_POLOLU_HPMC  19200


#define BARNACLE_RX_PIN    10
#define BARNACLE_TX_PIN    11

//Mid 0 = 1003 mid 2 = 1640
#define BARNACLE_RESET_PIN 7

#define WINCH_ENCODER_A_PIN 2
#define WINCH_ENCODER_B_PIN 3

/*Default Pin Values*/
// Plug each channel from the receiver in to each of these arduino pins
#define MAST_RC_RSX_PIN 22 // Channel 1
#define MAST_RC_RSY_PIN 23 // Channel 2
#define MAST_RC_LSY_PIN 24 // Channel 3
#define MAST_RC_LSX_PIN 25  // Channel 4

#define MAST_RC_GEAR_PIN 26 // Channel 5

// Pixy
#include <SPI.h>
#include <Pixy.h>
#define PIXY_REFRESH_INTERVAL 20

// LED Strip
#include <PololuLedStrip.h>
#define LED_COUNT 60
#define LED_PIN 5

// Pololu HPMC
#define HPMC_RX_PIN 51
#define HPMC_TX_PIN 50

// Pololu Maestro (servo)
#include <PololuMaestro.h>
#define SERVO_RX_PIN 53
#define SERVO_TX_PIN 52

#define PIXY_SERVO_CHANNEL 0
#define PIXY_SERVO_MAX_RIGHT 2280
#define PIXY_SERVO_MAX_LEFT 64

#define RUDDER_0_SERVO_CHANNEL 1
#define RUDDER_0_SERVO_MAX_RIGHT 2416
#define RUDDER_0_SERVO_MAX_LEFT 384

#define RUDDER_1_SERVO_CHANNEL 2
#define RUDDER_1_SERVO_MAX_RIGHT 2272
#define RUDDER_1_SERVO_MAX_LEFT 176

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
