/*
led-music-pulse
Copyright (C) 2018, Jeffrey St. Jean

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 3 of the License, or (at your option) any later version.

View LICENSE.md for more info.

You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
int musicPin = A5; // audio in pin
uint16_t *ptr; // memory of recent bass values to pass to microsmooth.h
double vReal[SAMPLES]; //   real    array of FFT
double vImag[SAMPLES]; // imaginary array of FFT

//Light Strip Variables
#define LED_PIN 4 // led pin
int NUM_LEDS = 60; // number of LEDs on light strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

//Program Control Variables
int ledValue; // value to set the led to (0-255)
bool highBassFlag = false; // 
int mode = 0; // which colour the leds should be set to
 
void setup() {
    Serial.begin(115200); // for debugging

    strip.begin(); // prepares the light-strip
    strip.show(); // initialize all pixels to 'off'

    sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY)); // configure sampling period

    ptr = ms_init(RDP); // sets microsmooth to smoothing mode RDP
    if(ptr == NULL) Serial.println("No memory"); // not enough memory for the array - should never happen

    randomSeed(analogRead(14)); // fetches a random number seed
    mode = random(0, 5); // randomly generates a number to set the mode to
}
 
void loop() {
    // sampling the audio
    for(int i = 0; i < SAMPLES; i++)
    {
        microseconds = micros();    // overflows after around 70 minutes!
     
        vReal[i] = analogRead(musicPin);
        vImag[i] = 0;
     
        while(micros() < (microseconds + sampling_period_us)){ }
    }
 
    // FFT doing its thing
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
    
    double bass = 0; // holds the bass level

    for(int i = 2; i<= 9; i++) {
      if(vReal[i] > bass) bass = vReal[i]; // finds the highest frequency within the 'bass' frequency window (2-9)
    }

    double filteredBass = rdp_filter(bass, ptr); // smooths the input

    
    if(filteredBass < 180) filteredBass = 170; // sets a floor to avoid random flickering when music is not playing

    int constrainedBass = constrain(filteredBass, 170, 850); // constrain the bass to these values

    ledValue = map(constrainedBass, 170, 850, 10, 255); // map the bass to analog numbers (10-255) - anything less than 10 looks bad
    //ledValue = map(v, , , 10, 255)

    Serial.println(ledValue);
    
    if(ledValue > 130) { // if this value is above the 'bass hit' value...
      if(!highBassFlag) { // if not still apart of a previous bass hit...
        highBassFlag = true; // say that it is a new bass hit
        mode++; // switch to the next mode (colour)
        if(mode == 6) mode = 0; // if overflow, wrap back around
      }
    }
    else {
        if(highBassFlag) highBassFlag = false; // if not a 'bass hit,' set the flag to false
      }

    // sets the colours to the appropriate values depending on the mode
    if(mode==0) {
      colorSet(strip.Color(round((255/255)*ledValue), round((0/255)*ledValue), round((0/255)*ledValue)));
    }
    else if(mode==1) {
      colorSet(strip.Color(round((255/255)*ledValue), round((255/255)*ledValue), round((0/255)*ledValue)));
    }
    else if(mode==2) {
      colorSet(strip.Color(round((0/255)*ledValue), round((255/255)*ledValue), round((0/255)*ledValue)));
    }
   else if(mode==3) {
      colorSet(strip.Color(round((0/255)*ledValue), round((255/255)*ledValue), round((255/255)*ledValue)));
    }
    else if(mode==4) {
      colorSet(strip.Color(round((0/255)*ledValue), round((0/255)*ledValue), round((255/255)*ledValue)));
    }
    else if(mode==5) {
      colorSet(strip.Color(round((255/255)*ledValue), round((0/255)*ledValue), round((255/255)*ledValue)));
    }
    
    strip.show(); // show the strip
    delay(1);
}


// sets the colour of the strip - must use strip.show() to actually see the update in real life
void colorSet(uint32_t c) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
  }
}
