/*
  led-music-pulse
  Copyright (C) 2020, Jeff St. Jean

  This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

*/

#include "arduinoFFT.h" // library for real-time frequency analysis
#include <Adafruit_NeoPixel.h> // library for communicating with light-strip
#include <microsmooth.h> // library for smoothing the input signal

//FFT Variables
#define SAMPLES 32             // must be a power of 2
#define SAMPLING_FREQUENCY 2000 // Hz, must be less than 10000 due to ADC
unsigned int sampling_period_us;
unsigned long microseconds;
arduinoFFT FFT = arduinoFFT();
int musicPin = A0; // audio in pin
uint16_t *ptr; // memory of recent bass values to pass to microsmooth.h
double vReal[SAMPLES]; //   real    array of FFT
double vImag[SAMPLES]; // imaginary array of FFT

// for smoothing of LED brightness
#define ledValueHistoryLength 3
int ledValueHistory[ledValueHistoryLength];

//Light Strip Variables
#define LED_PIN 9 // led pin
int NUM_LEDS = 60; // number of LEDs on light strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

//Program Control Variables
int ledValue; // value to set the led to (0-255)
bool highBassFlag = false; //
int mode = 0; // which colour the leds should be set to
int maximum = 0;

bool reactive_lights_active = false;
const int LIGHT_REACTIVE_ON_DELAY = 100;
const int LIGHT_REACTIVE_OFF_DELAY = 3000;
bool checking_for_change_in_state = false;
long last_change_in_state = 0;
int red = 0, green = 0, blue = 0, brightness = 0;
int wheel_pos = 0;
long last_wheel_update = 0;
const int PASSIVE_SAMPLE_RATE = 500;

void setup() {
  Serial.begin(115200); // for debugging

  strip.begin(); // prepares the light-strip
  strip.show(); // initialize all pixels to 'off'

  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY)); // configure sampling period

  ptr = ms_init(RDP); // sets microsmooth to smoothing mode RDP
  if (ptr == NULL) {
    Serial.println("Not enough memory"); // not enough memory for the array - should never happen
    while (true) ;
  }

  randomSeed(analogRead(A5)); // fetches a random number seed
  mode = random(0, 6); // randomly generates a number to set the mode to
  for (int i = 0; i < ledValueHistoryLength; i++) ledValueHistory[i] = 0;
  delay(100);
}

void loop() {
  // sampling the audio
  for (int i = 0; i < SAMPLES; i++) {
    microseconds = micros();    // overflows after around 70 minutes!

    vReal[i] = analogRead(musicPin);
    vImag[i] = 0;

    while (micros() < (microseconds + sampling_period_us)) { }
  }

  // FFT doing its thing
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

  double bass = 0; // holds the bass level

  for (int i = 2; i < 15; i++) {
    if (vReal[i] > bass) bass = vReal[i]; // finds the highest frequency within the 'bass' frequency window (2-15)
  }

  if (bass < 100) bass = 0;

  double filteredBass = rdp_filter(bass, ptr); // smooths the input


  if (filteredBass < 185) filteredBass = 170; // sets a floor to avoid random flickering when music is not playing
  if (filteredBass > maximum) maximum = filteredBass;

  int constrainedBass = constrain(filteredBass, 170, 700); // constrain the bass to these values

  if (reactive_lights_active == true) {

    ledValue = map(constrainedBass, 170, 700, 10, 255); // map the bass to analog numbers (10-255) - anything less than 10 looks ba5

    if (ledValue > 130) { // if this value is above the 'bass hit' value...
      if (!highBassFlag) { // if not still apart of a previous bass hit...
        highBassFlag = true; // say that it is a new bass hit
        mode++; // switch to the next mode (colour)
        if (mode == 7) mode = 0; // if overflow, wrap back around
      }
    }
    else {
      if (highBassFlag) highBassFlag = false; // if not a 'bass hit,' set the flag to false
    }

    int avgLedValue = 0;

    for (int i = ledValueHistoryLength - 1; i > 0; i--) {
      ledValueHistory[i] = ledValueHistory[i - 1];
      avgLedValue += ledValueHistory[i];
    }
    avgLedValue += ledValue;
    ledValueHistory[0] = ledValue;
    avgLedValue = round(avgLedValue / (float)ledValueHistoryLength);

    color_switch();

    brightness = avgLedValue;
  }
  else {
    if(millis() - last_wheel_update >= PASSIVE_SAMPLE_RATE) {
      wheel(wheel_pos);
      wheel_pos++;
      if(wheel_pos == 255) wheel_pos = 0;
      last_wheel_update = millis();
      brightness = 20;
    }
  }

  strip.setBrightness(brightness);
  colorSet(red, green, blue);
  strip.show(); // show the strip
  delay(1);
  if (reactive_lights_active == true) {
    if (filteredBass < 185 && !checking_for_change_in_state) {
      // Serial.println("Checking if music stopped");
      checking_for_change_in_state = true;
      last_change_in_state = millis();
    }
    if (checking_for_change_in_state) {
      if (filteredBass >= 185) {
        // Serial.println("No music is still playing");
        checking_for_change_in_state = false;
      }
      if (millis() - last_change_in_state >= LIGHT_REACTIVE_OFF_DELAY) {
        // Serial.println("Music has been off for long enough, switching to passive");
        reactive_lights_active = false;
        checking_for_change_in_state = false;
      }
    }
  }
  else {
    if (filteredBass >= 185 && !checking_for_change_in_state) {
      // Serial.println("Checking if music started");
      checking_for_change_in_state = true;
      last_change_in_state = millis();
    }
    if (checking_for_change_in_state) {
      if (filteredBass < 185) {
        // Serial.println("No music is still stopped");
        checking_for_change_in_state = false;
      }
      if (millis() - last_change_in_state >= LIGHT_REACTIVE_ON_DELAY) {
        // Serial.println("Music has been on for long enough, switching to active");
        reactive_lights_active = true;
        checking_for_change_in_state = false;
      }
    }
  }
}


// sets the colour of the strip - must use strip.show() to actually see the update in real life
void colorSet(int red, int blue, int green) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, red, green, blue);
  }
}

void color_switch() {
  switch (mode) {
    case 0: // purple
      red   = 60;
      blue  = 0;
      green = 255;
      break;
    case 1: // cyan
      red   = 30;
      blue  = 200;
      green = 255;
      break;
    case 2: // yellow
      red   = 100;
      blue  = 100;
      green = 0;
      break;
    case 3: // light blue
      red   = 20;
      blue  = 50;
      green = 255;
      break;
    case 4: // lime green
      red   = 0;
      blue  = 255;
      green = 30;
      break;
    case 5: // red
      red   = 200;
      blue  = 255;
      green = 30;
      break;
    case 6: // orange
      red   = 100;
      blue  = 200;
      green = 30;
      break;
  }
}


// Input values 0 to 255 to get color values that transition R->G->B. 0 and 255
// are the same color. This is based on Adafruit's Wheel() function, which used
// a linear map that resulted in brightness peaks at 0, 85 and 170. This version
// uses a quadratic map to make values approach 255 faster while leaving full
// red or green or blue untouched. For example, Wheel(42) is halfway between
// red and green. The linear function yielded (126, 129, 0), but this one yields
// (219, 221, 0). This function is based on the equation the circle centered at
// (255,0) with radius 255:  (x-255)^2 + (y-0)^2 = r^2
void wheel(byte position) {
  if (position < 85) {
    red = sqrt32((1530 - 9 * position) * position);
    green = sqrt32(65025 - 9 * position * position);
  }
  else if (position < 170) {
    position -= 85;
    red = sqrt32(65025 - 9 * position * position);
    blue = sqrt32((1530 - 9 * position) * position);
  }
  else {
    position -= 170;
    green = sqrt32((1530 - 9 * position) * position);
    blue = sqrt32(65025 - 9 * position * position);
  }
}



// Adapted from https://www.stm32duino.com/viewtopic.php?t=56#p8160
unsigned int sqrt32(unsigned long n) {
  unsigned int c = 0x8000;
  unsigned int g = 0x8000;
  while (true) {
    if (g * g > n) {
      g ^= c;
    }
    c >>= 1;
    if (c == 0) {
      return g;
    }
    g |= c;
  }
}
