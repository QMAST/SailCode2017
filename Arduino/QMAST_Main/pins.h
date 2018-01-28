#ifndef _PINS_H
#define _PINS_H

// Console
#define SERIAL_PORT_CONSOLE         Serial
#define SERIAL_BAUD_CONSOLE         57600

// XBee
#define SERIAL_PORT_XBEE            Serial1
#define SERIAL_BAUD_XBEE            57600

// Raspberry Pi
#define SERIAL_PORT_RPI             Serial2
#define SERIAL_BAUD_RPI             9600

// GPS
#define SERIAL_PORT_GPS             Serial3
#define SERIAL_BAUD_GPS             9600

#define PORT_RPI                    1
#define PORT_XBEE                   2

// Remote control (Spektrum)
#define PIN_RC_CH1                  22
#define PIN_RC_CH2                  23
#define PIN_RC_CH3                  24
#define PIN_RC_CH4                  25
#define PIN_RC_CH5                  26
#define PIN_RC_CH6                  27
#define PIN_RC_CH7                  28
#define PIN_RC_CH8                  29

// Servo connections
#define PIN_SERVO_1                 8
#define PIN_SERVO_2                 9
#define PIN_SERVO_3                 10
#define PIN_SERVO_WINCH             7

// Wind Vane (Analog)
#define APIN_WINDVANE               1

// Compass (CMPS11) 
#define COMPASS_ADDRESS             0xC0

#endif
