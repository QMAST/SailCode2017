#include "pins.h"

PololuLedStrip<LED_PIN> ledStrip;
rgb_color colors[LED_COUNT];

void initLEDStrip() {
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
}

void resetColors() {
  // set all colors to colorOff
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    colors[i] = colorOff;
  }
}

void setColors(int startLED, int endLED, rgb_color targetColor) {
  // Sets the LEDs from the startLED to endLED to targetColor
  endLED = constrain( endLED, 1, 60 );
  for (uint16_t i = startLED - 1; i < endLED; i++) {
    colors[i] = targetColor;
  }
}

void updateLEDs() {
  ledStrip.write(colors, LED_COUNT);
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
