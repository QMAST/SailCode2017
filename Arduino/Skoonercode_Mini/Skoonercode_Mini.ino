// Servo
#define RUDDER_PIN_RIGHT 3
#define RUDDER_PIN_LEFT 2
#include <Servo.h>
Servo rudderRight;
Servo rudderLeft;

// RC Radio
#include <radiocontrol.h>
#define MIN_RC_WAIT 50
#define RC_RSX_PIN 22 // Channel 1
#define RC_RSY_PIN 23 // Channel 2
#define RC_LSY_PIN 24 // Channel 3
#define RC_LSX_PIN 25  // Channel 4
#define RC_GEAR_PIN 26 // Channel 5
uint32_t rcLastUpdateMillis;
uint16_t rudderPosition = 0;
uint16_t doubleCheckRudder = 0;
rc_mast_controller radio_controller = {
  { RC_RSX_PIN, 1900, 1100 },
  { RC_RSY_PIN, 1900, 1100 },
  { RC_LSX_PIN, 1900, 1100},
  { RC_LSY_PIN, 1900, 1100 },
  { RC_GEAR_PIN, 1900, 1100}
};

void setup() {
  // put your setup code here, to run once:
  rudderRight.attach(RUDDER_PIN_RIGHT);
  rudderLeft.attach(RUDDER_PIN_LEFT);
}

void loop() {
  // Update RC every little bit
  if ( (millis() - rcLastUpdateMillis) > MIN_RC_WAIT ) {
    rcLastUpdateMillis = millis();
    updateRadioRudder(&radio_controller);
  }
}

void updateRadioRudder(rc_mast_controller* rc) {
  int16_t rcInput = rc_get_mapped_analog( rc->lsx, 90, -90 );
  if (abs(doubleCheckRudder - rcInput) < 5) {
    // Filter out RC noise
    doubleCheckRudder = rcInput;
    if (abs(rcInput) < 10) {
      // RC zero/"dead" zone
      rcInput = 0;
    } else if (abs(rcInput) > 150) {
      // RC might be off? Reset rudder position regardless
      rcInput = 0;
    }

    if (abs(rcInput - rudderPosition) > 5) {
      // If the new RC value differs from the current servo position by at least 5 degrees, move to new position
      rudderRight.write(rcInput);
      rudderLeft.write(rcInput);
      rudderPosition = rcInput;
    }
  }
}
