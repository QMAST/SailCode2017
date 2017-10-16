#include "pins.h"
#include <math.h>

#define IRONS_THRESHOLD_DEGREES 20
#define TACK_THRESHOLD_KNOTS 0
#define OUT_OF_IRONS_WAIT 5000
#define WP_ARRIVAL_THRESHOLD 100
#define TACK_TIMEOUT 12000

int iCurrentWPTarget = 0; // The index of the current waypoint in the array of waypoints
long lastInIronsMillis = 0;
long lastCheckMillis = 0;
long stationKeepStart = 0;
bool justInIrons = false;
bool tacking = false;
bool tackStartMillis = 0;
bool objectDetected = false;
bool rudderTakeover = false;
int object_position = 0;

void autonomous_main() {
  //Serial.println("Autosail invoked");

  update_airmar_tags();

  // Initialize variables
  int iToSetRudder = 0;
  int iToSetWinch = 0;

  // Check which type of autosail to use
  if (autosail_type == 0) {
    // Simple route, navigate between the waypoints specified by start_wp_index and finish_wp_index
    //Serial.println("Simple Route");
  } else if (autosail_type == 1) {
    pixyMode();
    delay(10);
  } else if (autosail_type == 2) {
    // Stationkeeping
    stationKeep();
  }

  if (millis() - lastCheckMillis > 2500) {
    lastCheckMillis = millis();

    // Updates waypoint progress and procedes to the next waypoint if needed
    isWeThere();
    // Calculate the relative angle to the next waypoint
    int relAngleToNextWP = getRelAngleToCurrentWP();
    int relWind = (wind_tag.wind_angle / (double) 10);

    sendTransmission(44, 200, String(relAngleToNextWP));

    sendTransmissionStart(46, 200);
    SERIAL_PORT_XBEE.print(waypoint[target_wp].lat);
    SERIAL_PORT_XBEE.print(" ");
    SERIAL_PORT_XBEE.print(waypoint[target_wp].lon);
    sendTransmissionEnd();

    if (relWind > 180) relWind = relWind - 360;
    // Check if we are in irons
    if (abs(relWind) < IRONS_THRESHOLD_DEGREES) {
      if (!tacking || ((millis() - tackStartMillis) > TACK_TIMEOUT)) {
        tacking = false;
        // If we are in irons, focus on trying to move out
        if (relWind < (IRONS_THRESHOLD_DEGREES - 10) && relWind > (-IRONS_THRESHOLD_DEGREES + 10)) {
          // If we are deep into irons, set the rudders agressively to exit
          sendTransmission(45, 200, F("In irons, agressively turning"));
          iToSetRudder = map(relWind, -1, 1, 80, -80);
        } else {
          sendTransmission(45, 200, F("In irons, not agressively turning"));
          if (relWind < 0) {
            iToSetRudder = IRONS_THRESHOLD_DEGREES + relWind;
          } else {
            iToSetRudder = - (IRONS_THRESHOLD_DEGREES - relWind);
          }
        }
        // Save state in case the boat comes out of irons between now and next loop
        justInIrons = true;
        lastInIronsMillis = millis();
      }
    } else {
      // Wait for 5 seconds to build up speed before trying to tack
      if (millis() - lastInIronsMillis < OUT_OF_IRONS_WAIT) {
        sendTransmission(45, 200, F("Just in irons, waiting to build speed"));
        iToSetRudder = -80;
      } else {
        // Steer to waypoint
        // Figure out a "bad zone", an area the boat cannot try tacking into because it will enter irons
        int iBadZoneBound1, iBadZoneBound2; // Bad zone 2 is clockwise of bad zone 1
        if (relWind <= -180 + IRONS_THRESHOLD_DEGREES) {
          // The bad zone thresholds cross the rear median of the boat from the left
          iBadZoneBound1 = relWind - IRONS_THRESHOLD_DEGREES + 360;
          iBadZoneBound2 = relWind + IRONS_THRESHOLD_DEGREES;
        } else if (relWind >= 180 - IRONS_THRESHOLD_DEGREES) {
          // The bad zone thresholds cross the rear median of the boat from the right
          iBadZoneBound1 =  relWind - IRONS_THRESHOLD_DEGREES;
          iBadZoneBound2 = relWind + IRONS_THRESHOLD_DEGREES - 360;
        } else {
          // The bad zone is to the right or left of the boat
          iBadZoneBound1 =  relWind - IRONS_THRESHOLD_DEGREES;
          iBadZoneBound2 = relWind + IRONS_THRESHOLD_DEGREES;
        }

        Serial.print(F("The bad zone is from "));
        Serial.print(iBadZoneBound1);
        Serial.print(F(" to "));
        Serial.print(iBadZoneBound2);

        // Checks if our current waypoint is in the "bad zone"
        bool bad = false;
        if (iBadZoneBound2 > 0 && iBadZoneBound1 > 0 && iBadZoneBound1 < relAngleToNextWP &&  iBadZoneBound2 > relAngleToNextWP) {
          bad = true;
        } else if (iBadZoneBound2 < 0 && iBadZoneBound1 < 0 && iBadZoneBound1 < relAngleToNextWP &&  iBadZoneBound2 > relAngleToNextWP) {
          bad = true;
        } else if (iBadZoneBound1 > 0 && iBadZoneBound2 < 0) {
          if (relAngleToNextWP > 0 && iBadZoneBound1 < relAngleToNextWP) bad = true;
          if (relAngleToNextWP < 0 && relAngleToNextWP < iBadZoneBound2) bad = true;
        } else if (iBadZoneBound1 < 0 && iBadZoneBound2 > 0) {
          if (relAngleToNextWP < 0 && iBadZoneBound1 < relAngleToNextWP) bad = true;
          if (relAngleToNextWP > 0 && relAngleToNextWP < iBadZoneBound2) bad = true;
        }

        if (bad) {
          // The current waypoint is in the "bad zone"
          iToSetRudder = map(relWind, -90, 90, -80, 80);
          sendTransmission(45, 200, F("The current waypoint is in the bad zone"));
        } else {
          // We know our current waypoint is not in the "bad zone", check if the bad zone is on the right or left of the boat
          // This way we can turn directly to the waypoint or decide whether or not to tack
          // The logic used depends on whether the "bad zone" is to the right or left of the boat
          if (relWind < 0) {
            if (relAngleToNextWP > iBadZoneBound2) {
              // Turn directly
              iToSetRudder = map(relAngleToNextWP, -90, 90, -80, 80);
              sendTransmission(45, 200, F("Direct turn to WP"));
            } else {
              // Decide whether to tack or gybe
              if ((wind_tag.wind_speed / (double) 10) < TACK_THRESHOLD_KNOTS) {
                // Wind is slow, gybe!
                iToSetRudder = 80;
                sendTransmission(45, 200, F("Gybing to WP"));
              } else {
                // Wind is fast, tack!
                iToSetRudder = -80;
                tacking = true;
                tackStartMillis = millis();
                sendTransmission(45, 200, F("Tacking to WP"));
              }
            }
          } else {
            if (relAngleToNextWP < iBadZoneBound1) {
              // Turn directly
              iToSetRudder = map(relAngleToNextWP, -90, 90, -80, 80);
              sendTransmission(45, 200, F("Direct turn to WP"));
            } else {
              // Decide whether or not to tack
              if ((wind_tag.wind_speed / (double) 10) < TACK_THRESHOLD_KNOTS) {
                // Wind is slow, gybe!
                iToSetRudder = -80;
                sendTransmission(45, 200, F("Gybing to WP"));
              } else {
                // Wind is fast, tack!
                iToSetRudder = 80;
                tacking = true;
                tackStartMillis = millis();
                sendTransmission(45, 200, F("Tacking to WP"));
              }
            }
          }
        }
      }
    }
    if (rudderTakeover == false) {
      setServoRudder(iToSetRudder);
      Serial.print(F("Setting rudders to "));
      Serial.println(iToSetRudder);
    }
    if (abs(relWind) <= IRONS_THRESHOLD_DEGREES) {
      iToSetWinch = 50;
    } else {
      iToSetWinch = map(abs(relWind), 55, 180, 0, 1000 );
    }
    Serial.print(F("Setting winches to "));
    Serial.println(iToSetWinch);
    setWinchTarget(iToSetWinch);
  }
}

// Returns the angle between the boat's position and the current target waypoint
int getAbsAngleToCurrentWP() {
  // Calculate the X and Y offsets
  double iLatOffset = waypoint[target_wp].lat - ((double)gps_tag.latitude * (int)gps_tag.sign_lat);
  double iLonOffset = waypoint[target_wp].lon - ((double)gps_tag.longitude * (int)gps_tag.sign_lon);

  // Find the angle of the boat relative to the boat (from -180 to 180)
  int angleToWP = atan2(iLonOffset, iLatOffset) * (180 / PI);

  // If the angle is under 0, make it positive (ie. represent it as a value between 0 and 360)
  if (angleToWP < 0) {
    angleToWP = 360 + angleToWP;
  }
  return angleToWP;
}

// Returns the angle to the current waypoint relative to the boat's heading
int getRelAngleToCurrentWP() {
  // Relative heading = boat heading - waypoint absolute heading
  int iBoatHeading = ((head_tag.mag_angle_deg / 10) % 360); // Between 0 and 360
  int iAbsAngleToWP = getAbsAngleToCurrentWP(); // Between 0 and 360

  int relAngleToWP = iAbsAngleToWP + (360 - iBoatHeading); // Make a paper boat and logic it out!

  // Always represent relAngleToWP as a value between -180 and 180
  if (relAngleToWP > 180) {
    relAngleToWP = relAngleToWP - 360;
  }

  return relAngleToWP;
}

void stationKeep() {
  if (stationKeep == 0) {
    // First time entering stationKeepingMode
    stationKeepStart = millis();
  } else if (millis() - stationKeepStart < 270000) {
    // Stay within box for 5 minutes
    sendTransmissionStart(45, 200);
    SERIAL_PORT_XBEE.print(F("Exiting box in "));
    SERIAL_PORT_XBEE.print(270000 - (millis() - stationKeepStart));
    SERIAL_PORT_XBEE.print(F("seconds"));
    sendTransmissionEnd();
  } else {
    // Exit box quickly
    setServoRudder(0);
    return;
  }
}

// Checks if we are at the next waypoint and advances to the next waypoint if arrived
void isWeThere() {
  // Calculate the X and Y offsets
  double iLatOffset = waypoint[target_wp].lat - ((double)gps_tag.latitude * (int)gps_tag.sign_lat);
  double iLonOffset = waypoint[target_wp].lon - ((double)gps_tag.longitude * (int)gps_tag.sign_lon);

  sendTransmissionStart(10, 200);
  /*SERIAL_PORT_XBEE.print(F("Lat: "));
  SERIAL_PORT_XBEE.print((double)gps_tag.latitude * (int)gps_tag.sign_lat);
  SERIAL_PORT_XBEE.print(F(" Lon: "));
  SERIAL_PORT_XBEE.print((double)gps_tag.longitude * (int)gps_tag.sign_lon);
  SERIAL_PORT_XBEE.print(F(" WayLat: "));
  SERIAL_PORT_XBEE.print(waypoint[target_wp].lat);
  SERIAL_PORT_XBEE.print(F(" WayLon: "));
  SERIAL_PORT_XBEE.print(waypoint[target_wp].lon);*/
  SERIAL_PORT_XBEE.print(F(" Lat offset: "));
  SERIAL_PORT_XBEE.print(iLatOffset);
  SERIAL_PORT_XBEE.print(F(" Lon offset: "));
  SERIAL_PORT_XBEE.print(iLonOffset);
  SERIAL_PORT_XBEE.print(F(" Waypoint offset: "));
  SERIAL_PORT_XBEE.print(sqrt((iLatOffset*iLatOffset) + (iLonOffset*iLonOffset)));
  sendTransmissionEnd();

  if (sqrt((iLatOffset*iLatOffset) + (iLonOffset*iLonOffset)) < WP_ARRIVAL_THRESHOLD) {
    target_wp++;
    if (target_wp > finish_wp_index) {
      sendTransmission(10, 201, F("Final destination reached! Looping to start."));
      target_wp = start_wp_index;
    }
    Serial.print("Waypoint reached, navigating to wp ");
    Serial.println(target_wp);
  }
}

int getObjectLocation(uint16_t servoposition, int pixyxposition) {
  // Returns estimated degree of detected buoy based on servo position and position in the pixy's field of view
  int servodegree = map(servoposition, PIXY_SERVO_MAX_LEFT, PIXY_SERVO_MAX_RIGHT, -90, 90);
  int pixydegree = map(pixyxposition, 0, 319, -37, 37);
  return servodegree + pixydegree;
}

void pixyMode() {
  // When the pixy discovers an object, take over the rudders and start going in that direction
  uint16_t blocks;
  // Poll the pixy
  Serial.println("Polling pixy");
  blocks = pixy.getBlocks();
  Serial.println("Polling servo");
  uint16_t position = 0;

  Serial.println("Hello");
  if (blocks && (pixy.blocks[0].width * pixy.blocks[0].height) > 400) {
    // Something was detected over a threshold size of 400 pixels
    Serial.println("I see something");
    objectDetected = true;
    rudderTakeover = true;

    // Send object angle
    sendTransmissionStart(61, 200);
    SERIAL_PORT_XBEE.print(getObjectLocation((position / 4), pixy.blocks[0].x));
    SERIAL_PORT_XBEE.print(";");

    // Start sailing in that direction
    setServoRudderDeg(getObjectLocation((position / 4), pixy.blocks[0].x));

    // Arrived at buoy
    if (pixy.blocks[0].width * pixy.blocks[0].height > 20000) {
      sendTransmission(10, 201, F("Boat has arrived at the buoy!"));
      start_wp_index = 0;
      finish_wp_index = 0;
      target_wp = 0;
      autosail_type = 0;
      resetColors();
      ledSuccess();
    }

    // Move motor to track object
    if (pixy.blocks[0].x <= 63) {
      // Detected object in very left of FOV, move servo to left to get a better look
      //maestro.setSpeed(0, 60);
      setServoPixyDeg( -90);

      if (object_position != 1) {
        resetColors();
        setColors(49, 60, colorRed);
        updateLEDs();
        object_position = 1;
      }
    } else if (pixy.blocks[0].x <= 127) {
      // Detected object in left of FOV, move servo to left to get a better look
      setServoSpeed(PIXY_SERVO_CHANNEL, 20);
      setServoPixyDeg( -90);

      if (object_position != 2) {
        resetColors();
        setColors(37, 48, colorRed);
        object_position = 2;
        updateLEDs();
      }
    } else if (pixy.blocks[0].x <= 191) {
      // Detected object in centre of FOV, don't move servo
      setServoPosition(PIXY_SERVO_CHANNEL, getServoPosition(PIXY_SERVO_CHANNEL));

      if (object_position != 3) {
        resetColors();
        setColors(31, 36, colorRed);
        setColors(25, 30, colorGreen);
        object_position = 3;
        updateLEDs();
      }
    } else if (pixy.blocks[0].x <= 255) {
      // Detected object in right of FOV, move servo to right to get a better look
      setServoSpeed(PIXY_SERVO_CHANNEL, 20);
      setServoPixyDeg( 90);

      if (object_position != 4) {
        resetColors();
        setColors(13, 24, colorGreen);
        object_position = 4;
        updateLEDs();
      }
    } else {
      // Detected object in very right of FOV, move servo to right to get a better look
      //maestro.setSpeed(0, 60);
      setServoPixyDeg( 90);

      if (object_position != 5) {
        resetColors();
        setColors(1, 12, colorGreen);
        object_position = 5;
        updateLEDs();
      }
    }
  } else {
    Serial.println("I don't see anything");
    // No object detected
    // Run the following if previous state had a detected object
    if (objectDetected) {
      // Reset LED strip
      resetColors();
      updateLEDs();
      // Reset variables
      objectDetected = false;
      rudderTakeover = false;
      object_position = 0;
      // Start servo to the right
      setServoSpeed(PIXY_SERVO_CHANNEL, 20);
      setServoPixyDeg(90);
      sendTransmission(61, 204, F("0"));
    }
    // Use servo to scan
    if (position <= (PIXY_SERVO_MAX_LEFT + 50) * 4) {
      setServoPixyDeg(90);
    } else if (position >= (PIXY_SERVO_MAX_RIGHT - 50) * 4) {
      setServoPixyDeg(-90);
    }
  }
}
