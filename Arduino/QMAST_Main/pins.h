/*
  pins.h
  Header file mapping to the 2017-2018 boat

  Created in December 2017, QMAST
*/


#define DEBUG // Comment this line out to disable debug printing to USB serial

#ifdef DEBUG
 #define DEBUG_PRINTLN(x)  SERIAL_PORT_CONSOLE.println (x)
 #define DEBUG_PRINT(x)  SERIAL_PORT_CONSOLE.print (x)
#else
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINT(x)
#endif

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
#define RC_STD_TIMEOUT 50000 //RC Pulse standard timeout, recommend at least 20000
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
