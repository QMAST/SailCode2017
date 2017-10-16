// michaelspiering2@gmail.com

#include "pins.h"
#define AUTOSAIL_TIMEOUT 1000
#define AUTOSAIL_ALGORITHM_TICKS 100
#define AUTOSAIL_MIN_DELAY 20
#define NAV_MIN_DELAY 2000
#define devCount 360
#define CHDEG 35 //Closest degree offset from wind direction that boat can sail
#define DIRCONST 15 //Offset of green area from dirDeg (plus or minus)
#define IRON_CONST 15
#define TACK_TIMEOUT 5000
uint32_t autosail_start_time = 0;
uint32_t stationkeep_start_time = 0;
uint32_t time_since_lastNav;
uint32_t last_degree;

//int timer_station = 0;
//uint32_t last_change = 0;


uint32_t head_check = 0;
uint32_t rud_update = 0;
int32_t angle_to_wp = 0;
int degree = 0;
int8_t turn_flag = 2;

//const int winDeg = 0;

typedef struct DegScore {
  int degree;
  int score;
} DegScore;

int32_t tack_time = 0;
int target = 0;


DegScore navScore[devCount];

bool objectDetected = true;
int object_position = 0;

void autosail_main() {

  Serial.println("Autosail invoked");
  // Some sort of if for autosail modes, searching, stationkeep, race.

  if (millis() - autosail_start_time < AUTOSAIL_MIN_DELAY)
    return;

  autosail_start_time = millis();
  //update_airmar_tags();

  //Serial.println(autosail_start_time);

  if ((millis() - autosail_start_time) > AUTOSAIL_TIMEOUT) {
    Serial.print(F("Autosail Timeout"));
    return;
  }

  int rudder_takeover = 0;
  //Serial.println(autosail_type);
  if (autosail_type == 0) {
    // Simple route, navigate between the waypoints specified by start_wp_index and finish_wp_index
    Serial.println("Simple Route");

  } else if (autosail_type == 1) {
    // Computer vision tracking
    // Sail between waypoints specified by start_wp_index and finish_wp_index (search pattern) until pixy discovers an object
    // When the pixy discovers an object, take over the rudders and start going in that direction
    Serial.println("Computer Vision");
    uint16_t blocks;
    // Poll the pixy
    Serial.println("Polling pixy");
    blocks = pixy.getBlocks();
    Serial.println("Polling servo");
    uint16_t position = getServoPosition(PIXY_SERVO_CHANNEL);

    Serial.println("Hello");
    if (blocks && (pixy.blocks[0].width * pixy.blocks[0].height) > 400) {
      // Something was detected over a threshold size of 400 pixels
      Serial.println("I see something");
      objectDetected = true;
      rudder_takeover = 1;

      // Send object angle
      sendTransmissionStart(61, 200);
      SERIAL_PORT_XBEE.print(getObjectLocation((position / 4), pixy.blocks[0].x));
      SERIAL_PORT_XBEE.print(";");

      // Start sailing in that direction
      degree = calcNavScore(wind_tag.wind_angle, getObjectLocation((position / 4), pixy.blocks[0].x));
      changeDir(degree);

      // Arrived at buoy
      if (pixy.blocks[0].width * pixy.blocks[0].height > 20000) {
        sendTransmission(10, 201, F("Destination Reached! Boat will now stay at this location."));
        set_waypoint();
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
  } else if (autosail_type == 2) {
    // Stationkeeping
    stationKeep();
  }

  // Send current autosail data every second or so
  if (((millis() - head_check) > 1000)) {
    angle_to_wp = (720 + (gpsDirect() - (head_tag.mag_angle_deg / 10)) + 12) % 360;

    sendTransmissionStart(44, 200);
    SERIAL_PORT_XBEE.print(angle_to_wp);
    sendTransmissionEnd();

    sendTransmissionStart(46, 200);
    SERIAL_PORT_XBEE.print(waypoint[target_wp].lat);
    SERIAL_PORT_XBEE.print(F(" "));
    SERIAL_PORT_XBEE.print(waypoint[target_wp].lon);
    sendTransmissionEnd();

    head_check = millis();
  }

  update_airmar_tags();

  if (((millis() - rud_update) > 500) && (rudder_takeover != 1)) {
    degree = calcNavScore(wind_tag.wind_angle, angle_to_wp);
    sendTransmissionStart(45, 200);
    SERIAL_PORT_XBEE.print(F("Proceeding at "));
    SERIAL_PORT_XBEE.print(degree);
    SERIAL_PORT_XBEE.print(F(" degrees"));
    sendTransmissionEnd();
    
    changeDir(degree);
    rud_update = millis();
  }

  /*const int UPangle = 45;
    int winch_update_time;
    int winch_wind = wind_tag.wind_angle;
    //Winch update, using 45 degree threshold for in
    if ((millis() - winch_update_time) < 5000) {
    winch_update_time = millis();
    if (winch_wind > 180) {
      winch_wind = 360 - winch_wind;
    }
    //All in if close hauled
    if (winch_wind < UPangle) {
      motor_winch_abs(-1000);
    }
    //Map to point of sail
    else {
      motor_winch_abs(map(winch_wind, 45, 180, -1000, 1000));
    }
    }*/
}

int getObjectLocation(uint16_t servoposition, int pixyxposition) {
  // Returns estimated degree of detected buoy based on servo position and position in the pixy's field of view
  int servodegree = map(servoposition, PIXY_SERVO_MAX_LEFT, PIXY_SERVO_MAX_RIGHT, -90, 90);
  int pixydegree = map(pixyxposition, 0, 319, -37, 37);
  return servodegree + pixydegree;
}

/**
  Changes the rudders based on a given degree from bow
**/
void changeDir(int degree) {
  //String data = "Rudder degree: " + String(degree);
  //sendTransmission(10, 202, data);

  //	SERIAL_PORT_XBEE.print("Target: ");
  //	SERIAL_PORT_XBEE.print(target);

  if (degree > 5 && degree <= 180) {
    sendTransmission(10, 202, F("Turning Right"));

    if (target > 0) {
      target = target - 150;
      tack_time = millis();
    }
    else if (target > -1000) {
      target = target - 75;
      tack_time = millis();
    }
    else {
      if (millis() - tack_time > TACK_TIMEOUT) {
        target = 1000;
        tack_time = millis();
      }
    }
  }
  else if (degree > 180 && degree < 355) {
    sendTransmission(10, 202, F("Turning Left"));

    if (target < 0) {
      target = target + 150;
      tack_time = millis();
    }
    else if (target < 1000) {
      target = target + 75;
      tack_time = millis();
    }
    else {
      if (millis() - tack_time > TACK_TIMEOUT) {
        target = -1000;
        tack_time = millis();
      }
    }
  }
  else {
    sendTransmission(10, 202, F("Sailing straight"));
    target = 0;
  }
  setServoRudder(target);
}



void autosail_print_tags() {
  anmea_print_wiwmv(&wind_tag, &Serial);
  anmea_print_hchdg(&head_tag, &Serial);
  anmea_print_gpgll(&gps_tag, &Serial);
  Serial.print("\n");
}
void auto_update_winch(
  rc_mast_controller* rc,
  pchamp_controller mast[2])
{
  int16_t rc_input;

  char buf[40];       // buffer for printing debug messages
  uint16_t rvar = 0;
  static uint16_t old_winch = 0;
  //Winch
  rc_input = rc_get_mapped_analog( rc->rsy, -1000, 1000 );
  sendTransmissionStart(64, 200);
  if (abs(rc_input) < 200) {
    SERIAL_PORT_XBEE.print(rc_input);
    SERIAL_PORT_XBEE.print(" (Zeroed)");
    //motor_set_winch(0);
    old_winch = 0;
  } else if (rc_input < -2000) {
    SERIAL_PORT_XBEE.print(F("Remote not connected"));
  } else {
    SERIAL_PORT_XBEE.print(rc_input);
    if (rc_input > 0 && old_winch != 1) {
      //motor_set_winch(250);
      old_winch = 1;
    } else if (rc_input < 0 && old_winch != -1) {
      //motor_set_winch(-250);
      old_winch = -1;
    }
  }
  sendTransmissionEnd();

}

//Calculates best navscore, and returns it's degree
int calcNavScore(int windDeg, int dirDeg) {
  //wind deg is degree of wind (clockwise) with respect to boat's bow (deg = 0).
  // dirDeg is degree of waypoint (clockwise) with respect to boat's bow (deg = 0).
  int best_score = 1000;
  int best_degree;
  for (int i = 0; i < devCount; i++) {
    navScore[i].degree = 360 * i / devCount;
    navScore[i].score = calcWindScore(windDeg, navScore[i].degree) + calcDirScore(dirDeg, navScore[i].degree);
    if (navScore[i].score < best_score) {
      best_score = navScore[i].score;
      best_degree = navScore[i].degree;
    }
  }
  if (abs(windDeg - dirDeg) > CHDEG && abs(360 - windDeg + dirDeg) > CHDEG && abs(360 + windDeg - dirDeg) > CHDEG) {
    //check if waypoint is down wind from close hauled limit
    best_degree = dirDeg;
  }
  return (best_degree);
}

int calcWindScore(int windDeg, int scoreDeg) {
  //calculate red area
  if (abs(windDeg - scoreDeg) < CHDEG) //check if facing upwind
    return 360; //return high number for undesirable direction (upwind)

  int pointA = (windDeg + 180) % 360; //Point directly down wind from boatA
  int pointB = (windDeg + CHDEG) % 360; //Maximum face-up angle (clockwise from wind)
  int pointC = (windDeg - CHDEG) % 360; //Maximum face-up angle (counter-clockwise from wind)

  if (abs(windDeg - scoreDeg)  <= CHDEG ||
      abs(360 - windDeg + scoreDeg) <= CHDEG ||
      abs(360 + windDeg - scoreDeg) <= CHDEG) //check if facing upwind
    return 170;
  //Calculate green area if wind is to port (left) of bow
  else if (windDeg == 0)
    return 10;
  else if (windDeg > 180 && (scoreDeg <= pointA || scoreDeg >= pointB))
    return 10; //arbitrary
  //Calculate yellow area if wind is to port (left) of bow
  else if (windDeg > 180 && (scoreDeg >= pointA && scoreDeg <= pointC))
    return 100;//arbitrary
  //Calculate green area if wind is to starboard (right) of bow
  else if (windDeg <= 180 && (scoreDeg >= pointA || scoreDeg <= pointC))
    return 10;//arbitrary
  //Calculate yellow area if wind is to starboard (right) of bow
  else if (windDeg <= 180 && (scoreDeg <= pointA && scoreDeg >= pointB))
    return 100;//arbitrary
}

int calcDirScore(int dirDeg, int scoreDeg) {
  //green area
  if (abs(dirDeg - scoreDeg) < DIRCONST)
    return 2;//arbitrary
  //yellow area
  if (abs(dirDeg - scoreDeg) <= 90)
    return 5;//arbitrary
  //red area
  return 10;

}

void stationKeep() {
  if (stationkeep_start_time == 0) {
    // First time entering stationKeepingMode
    stationkeep_start_time = millis();
  } else if (millis() - stationkeep_start_time < 300000) {
    // Stay within box for 5 minutes
    sendTransmissionStart(45, 200);
    SERIAL_PORT_XBEE.print(F("Exiting box in "));
    SERIAL_PORT_XBEE.print(300000 - (millis() - stationkeep_start_time));
    SERIAL_PORT_XBEE.print(F("seconds"));
    sendTransmissionEnd();
  } else {
    // Exit box quickly
  }
}

int gpsDirect() {
  int lat_vect = waypoint[target_wp].lat - gps_tag.latitude;
  int lon_vect = waypoint[target_wp].lon + gps_tag.longitude;
  double gps_vect = (double)(lat_vect) / (double)(lon_vect);

  if (abs(lat_vect) < 50 && abs(lon_vect) < 50 && lat_vect != 0 && lon_vect != 0) {
    target_wp++;
    if (target_wp > finish_wp_index) {
      sendTransmission(10, 201, F("Final destination reached! Looping to start. "));
      target_wp = start_wp_index;
    }
    lat_vect = waypoint[target_wp].lat - gps_tag.latitude;
    lon_vect = waypoint[target_wp].lon + gps_tag.longitude;
    gps_vect = (double)(lat_vect) / (double)(lon_vect);
  }

  // Send direction data
  sendTransmissionStart(44, 200);
  SERIAL_PORT_XBEE.print((atan(gps_vect) * (180 / PI)));
  SERIAL_PORT_XBEE.print(" ");
  SERIAL_PORT_XBEE.print(lat_vect);
  SERIAL_PORT_XBEE.print(" ");
  SERIAL_PORT_XBEE.print(lon_vect);
  sendTransmissionEnd();

  if (lat_vect < 0 && lon_vect < 0) {
    return (270 - (atan (lat_vect / lon_vect) * (180 / PI)));
  }
  if (lat_vect < 0 && lon_vect > 0) {
    return (90 + abs(atan (lat_vect / lon_vect) * (180 / PI)));
  }
  if (lat_vect > 0 && lon_vect < 0) {
    return (270 + abs (atan (lat_vect / lon_vect) * (180 / PI)));
  }
  if (lat_vect > 0 && lon_vect > 0) {
    return (90 - (atan (lat_vect / lon_vect) * (180 / PI)));
  }
}
