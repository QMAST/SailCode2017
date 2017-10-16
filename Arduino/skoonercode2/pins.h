#ifndef _PINS_H
#define _PINS_H

#include <SoftwareSerial.h>
#include <math.h>

// Airmar
#include <bstrlib.h>
#include <anmea.h>
#include <TinyGPS++.h>
#define AIRMAR_NMEA_STRING_BUFFER_SIZE 80

// Remote
#include <radiocontrol.h>

// Motors
#include <pololu_champ.h>

// Autosail
typedef struct Waypoint {
  int32_t lon;
  int32_t lat;
} Waypoint;
Waypoint waypoint[20];
int target_wp = 0;
int start_wp_index = 0;
int finish_wp_index = 0;

// LED Strip
#include <PololuLedStrip.h>
rgb_color colorOff;
rgb_color colorRed;
rgb_color colorGreen;

// Pixy
#include <SPI.h>
#include <Pixy.h>
Pixy pixy;
#endif
