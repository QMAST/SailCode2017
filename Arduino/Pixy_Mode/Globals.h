// General libraries
#include <SoftwareSerial.h>

// Pixy libraries and globals
#include <SPI.h>
#include <Pixy.h>

// LED libraries and globals
#include <PololuLedStrip.h>
#define LED_COUNT 60
#define LED_PIN 5

// Pololu libraries and globals
#include <PololuMaestro.h>
#define PIXY_SERVO_RX 12
#define PIXY_SERVO_TX 13
#define PIXY_SERVO_CHANNEL 0
#define PIXY_SERVO_MAX_RIGHT 2280
#define PIXY_SERVO_MAX_LEFT 64

// Xbee globals
#define SERIAL_PORT_XBEE  Serial1

// Time refresh intervals
#define PIXY_REFRESH_INTERVAL 20
