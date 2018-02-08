#include <SoftwareSerial.h>

// Declare globals for xbee
String transmissionBuffer = "";         // a string to hold incoming data

// Include LED Strip library and declare globals
#include <PololuLedStrip.h>
PololuLedStrip<5> ledStrip;
#define LED_COUNT 60
rgb_color colors[LED_COUNT];
rgb_color colorOff;
rgb_color colorRed;
rgb_color colorGreen;

// Include Pololu servo controller library and declare globals
#include <PololuMaestro.h>
// Pin 11 is not usable due to interference with the pixy
SoftwareSerial maestroSerial(12, 13);
MicroMaestro maestro(maestroSerial);
#define SERVO_MAX_RIGHT 2304
#define SERVO_MAX_LEFT 64

// Include Pixy library and declare globals
#include <SPI.h>
#include <Pixy.h>
Pixy pixy;

bool objectDetected = true;
int object_position = 0;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial1.print("10,200,Initializing...;");
  // Initialize Colors
  colorOff.red = 0;
  colorOff.green = 0;
  colorOff.blue = 0;
  colorRed.red = 255;
  colorRed.green = 0;
  colorRed.blue = 0;
  colorGreen.red = 0;
  colorGreen.green = 255;
  colorGreen.blue = 0;

  pixy.init();
  // Set the pololu serial baud rate.
  transmissionBuffer.reserve(200); // reserve 200 bytes for the inputString:
  maestroSerial.begin(57600);
  Serial1.print("10,200,Everything is OK;");
}

void loop() {
  uint16_t blocks;
  // Poll the pixy
  blocks = pixy.getBlocks();
  uint16_t position = maestro.getPosition(0);
  Serial1.print("60,200,");
  Serial1.print(map((position / 4), SERVO_MAX_LEFT, SERVO_MAX_RIGHT, -90, 90));
  Serial1.print(";");
  if (blocks && (pixy.blocks[0].width * pixy.blocks[0].height) > 400) {
    // Something was detected over a threshold size of 400 pixels
    objectDetected = true;
    Serial1.print("61,200,");
    Serial1.print(getObjectLocation((position / 4), pixy.blocks[0].x));
    Serial1.print(";");
    if (pixy.blocks[0].x <= 63) {
      // Detected object in very left of FOV, move servo to left to get a better look
      //maestro.setSpeed(0, 60);
      maestro.setTarget(0, SERVO_MAX_LEFT * 4);

      if (object_position != 1) {
        resetColors();
        setColors(49, 60, colorRed);
        ledStrip.write(colors, LED_COUNT);
        object_position = 1;
      }
    } else if (pixy.blocks[0].x <= 127) {
      // Detected object in left of FOV, move servo to left to get a better look
      maestro.setSpeed(0, 20);
      maestro.setTarget(0, SERVO_MAX_LEFT * 4);

      if (object_position != 2) {
        resetColors();
        setColors(37, 48, colorRed);
        object_position = 2;
        ledStrip.write(colors, LED_COUNT);
      }
    } else if (pixy.blocks[0].x <= 191) {
      // Detected object in centre of FOV, don't move servo
      maestro.setTarget(0, maestro.getPosition(0));

      if (object_position != 3) {
        if (pixy.blocks[0].width * pixy.blocks[0].height > 20000) {
          resetColors();
          ledSuccess();
        } else {
          resetColors();
          setColors(31, 36, colorRed);
          setColors(25, 30, colorGreen);
          object_position = 3;
          ledStrip.write(colors, LED_COUNT);
        }
      }
    } else if (pixy.blocks[0].x <= 255) {
      // Detected object in right of FOV, move servo to right to get a better look
      maestro.setSpeed(0, 20);
      maestro.setTarget(0, SERVO_MAX_RIGHT * 4);

      if (object_position != 4) {
        resetColors();
        setColors(13, 24, colorGreen);
        object_position = 4;
        ledStrip.write(colors, LED_COUNT);
      }
    } else {
      // Detected object in very right of FOV, move servo to right to get a better look
      //maestro.setSpeed(0, 60);
      maestro.setTarget(0, SERVO_MAX_RIGHT * 4);

      if (object_position != 5) {
        resetColors();
        setColors(1, 12, colorGreen);
        object_position = 5;
        ledStrip.write(colors, LED_COUNT);
      }
    }
  } else {
    // No object detected
    // Run the following if previous state had a detected object
    if (objectDetected) {
      // Reset LED strip
      resetColors();
      ledStrip.write(colors, LED_COUNT);
      // Reset variables
      objectDetected = false;
      object_position = 0;
      // Start servo to the right
      maestro.setSpeed(0, 20);
      maestro.setTarget(0, SERVO_MAX_RIGHT * 4);
      Serial1.print("61,204,0;");
    }
    // Use servo to scan
    if (position <= (SERVO_MAX_LEFT + 50) * 4) {
      maestro.setTarget(0, SERVO_MAX_RIGHT * 4);
    } else if (position >= (SERVO_MAX_RIGHT - 50) * 4) {
      maestro.setTarget(0, SERVO_MAX_LEFT * 4);
    }
  }

  // Xbee Code
  if (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();
    //Serial.print(inChar);
    // add it to the inputString:
    transmissionBuffer += inChar;
    // if the incoming character is a ";" do something about it:
    if (inChar == ';') {
      int iCommand = transmissionBuffer.substring(0, 2).toInt();
      String data = transmissionBuffer.substring(3, transmissionBuffer.indexOf(";"));
      switch (iCommand) {
        case 20:
          Serial1.print("20,200,added;");
          break;
        case 21:
          if (data == "0") {
            //digitalWrite(ledPin, LOW);
            ledSuccess();
            Serial1.print("21,200,0;");
          } else if (data == "1") {
            ledSuccess();
            //digitalWrite(ledPin, HIGH);
            Serial1.print("21,200,1;");
          } else {
            Serial1.print("21,501,Unknown Command:'" + data + "';");
          }
          break;
        default:
          Serial1.print("501;");
          break;
      }
      // clear the string:
      transmissionBuffer = "";
    }
  }

  // Pixy operates at 50fps so pause for 20ms
  delay(100);
}

int getObjectLocation(uint16_t servoposition, int pixyxposition) {
  // Returns estimated degree of detected buoy based on servo position and position in the pixy's field of view
  int servodegree = map(servoposition, SERVO_MAX_LEFT, SERVO_MAX_RIGHT, -90, 90);
  int pixydegree = map(pixyxposition, 0, 319, -37, 37);
  return servodegree + pixydegree;
}

void resetColors() {
  // set all colors to colorOff
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    colors[i] = colorOff;
  }
}

void setColors(int startLED, int endLED, rgb_color targetColor) {
  // Sets the LEDs from the startLED to endLED to targetColor
  for (uint16_t i = startLED - 1; i < endLED; i++) {
    colors[i] = targetColor;
  }
}

void ledSuccess() {
  // Fun success sequence, delays loop while running
  rgb_color colorWhite;
  colorWhite.red = 255;
  colorWhite.green = 255;
  colorWhite.blue = 255;
  for (int i = 0; i < LED_COUNT + 5; i++) {
    if (i < LED_COUNT) {
      colors[i] = colorRed;
    }
    if (i - 5 >= 0) {
      colors[(i - 5)] = colorOff;
    }
    ledStrip.write(colors, LED_COUNT);
    delay(10);
  }
  for (int i = LED_COUNT - 1; i >= -5; i--) {
    if (i >= 0) {
      colors[i] = colorRed;
    }
    if ((i + 5) < LED_COUNT) {
      colors[(i + 5)] = colorOff;
    }
    ledStrip.write(colors, LED_COUNT);
    delay(10);
  }
  setColors(1, 60, colorWhite);
  ledStrip.write(colors, LED_COUNT);
  delay(50);
  setColors(1, 60, colorOff);
  ledStrip.write(colors, LED_COUNT);
  delay(50);
  setColors(1, 60, colorWhite);
  ledStrip.write(colors, LED_COUNT);
  delay(50);
  setColors(1, 60, colorOff);
  ledStrip.write(colors, LED_COUNT);
  delay(50);
}

