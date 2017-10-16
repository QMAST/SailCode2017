#include "Globals.h"
Pixy pixy;

bool prevObjDetect, newServoPosition;
bool servoScanning = true;
int servoPosition = 0; // Saves the current servo target as a degree between -90 and 90.
unsigned long previousPixyMillis = 0;

void setup() {
  Serial.begin(9600);
  // Initialize each component
  initXbee();
  sendTransmission(10, 200, "Initializing...");
  initLEDStrip();
  initServo();
  pixy.init();
  sendTransmission(10, 200, "Everything is OK");
  sendTransmission(60, 200, "Null");
}

void loop() {
  // Checks if the Xbee has recieved anything and let it do its thing
  updateAndExecute();

  // Updates internal timekeeping mechanism
  unsigned long currentMillis = millis();

  // Send position of servo
  sendTransmission(60, 200, String(getServoDeg(PIXY_SERVO_CHANNEL)));

  // Refresh Pixy if the interval has passed
  if (currentMillis - previousPixyMillis >= PIXY_REFRESH_INTERVAL) {
    previousPixyMillis = currentMillis;
    uint16_t blocks = pixy.getBlocks();
    // Check if objects are seen
    if (blocks && (pixy.blocks[0].width * pixy.blocks[0].height) > 400) {
      // Object detected
      int pixyobjdeg = map(pixy.blocks[0].x, 0, 319, -37, 37); // gets where the object is in the pixy's FOV as a degree between -37 and 37 (the Pixy has a 75 degree FOV), 0 is the centre
      // Send estimated object location
      sendTransmission(61, 200, String(getServoDeg(PIXY_SERVO_CHANNEL) + pixyobjdeg) + " degrees (" + String(pixyobjdeg) + " degrees for the Pixy)");
      if (pixyobjdeg > 10 || pixyobjdeg < -10) {
        Serial.println("The buoy is not in the centre of the Pixy's FOV. It is "+String(pixyobjdeg)+" degrees from the centre. The servo is at " +String(getServoDeg(PIXY_SERVO_CHANNEL))+ " degrees");
        // If the buoy is not close to the centre of the pixy's field of view, move the servo to the predicted centre
        servoPosition = getServoDeg(PIXY_SERVO_CHANNEL) + pixyobjdeg;
        newServoPosition = true;
      }else{
        Serial.println("The buoy is in the centre of the Pixy's FOV");
      }
      if (prevObjDetect != true) {
        servoScanning = false;
        prevObjDetect = true;
      }
    } else {
      // Object not detected
      if (prevObjDetect != false) {
        sendTransmission(60, 200, "0");
        servoScanning = true;
        setServoDeg(PIXY_SERVO_CHANNEL, 90);
        prevObjDetect = false;
      }
    }
  }

  // If servos are supposed to be scanning, tell them to scan now
  if (servoScanning) {
    int currServoDeg = getServoDeg(PIXY_SERVO_CHANNEL);
    // Use servo to scan
    if (currServoDeg <= -90) {
      Serial.println("Scanning to 90...");
      setServoDeg(PIXY_SERVO_CHANNEL, 90);
    } else if (currServoDeg >= 90) {
      Serial.println("Scanning to -90...");
      setServoDeg(PIXY_SERVO_CHANNEL, -90);
    }
  } else if (newServoPosition) {
    // If a new position has been defined for servos, set it now
    setServoDeg(PIXY_SERVO_CHANNEL, servoPosition);
    Serial.println("The next predicted servo position is " + String(servoPosition));
    bool reaching = true;
    while (reaching) {
      delay(100);
      Serial.println("Moving to predicted location. Waiting for " + String(servoPosition) + ", currently at " + getServoDeg(PIXY_SERVO_CHANNEL));
      if (getServoDeg(PIXY_SERVO_CHANNEL) < (servoPosition + 2) && getServoDeg(PIXY_SERVO_CHANNEL) > (servoPosition - 2)) reaching = false;
      }
  }
  delay(100);
}
