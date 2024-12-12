/*********************************************************
This is a library for the MPR121 12-channel Capacitive touch sensor

Designed specifically to work with the MPR121 Breakout in the Adafruit shop 
  ----> https://www.adafruit.com/products/

These sensors use I2C communicate, at least 2 pins are required 
to interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada for Adafruit Industries.  
BSD license, all text above must be included in any redistribution
**********************************************************/

void startup() {
  //play a lil startup
  delay(10);
  Serial.println(100);
  Serial.println(-15);
  Serial.println(-18); Serial.println(-19); Serial.println(-21);
  delay(250);
  Serial.println(-7);
  delay(1000);
  Serial.println(0);
  delay(1000);
}

#include <Wire.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

//From Smoothing.ino
// Define the number of samples to keep track of. The higher the number, the
// more the readings will be smoothed, but the slower the output will respond to
// the input. Using a constant rather than a normal variable lets us use this
// value to determine the size of the readings array.
const int numReadings = 5;

int readings[numReadings];  // the readings from the analog input
int readIndex = 0;          // the index of the current reading
int total = 0;              // the running total
int average = 0;            // the average
int oldAverage = 0;

#define inputPin A0

int serialvalue; // value for serial input
int started = 0; // flag for whether we've received serial yet

void setup() {
  Serial.begin(19200);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  //smoothing
  // initialize all the readings to 0:
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  while (!Serial) { // needed to keep leonardo/micro from starting too fast!
    delay(10);
  }
  
  Serial.println("Adafruit MPR121 Capacitive Touch sensor test"); 
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");
}

void loop() {
  if(Serial.available() && started == 0) // check to see if there's serial data in the buffer
  {
    serialvalue = Serial.read(); // read a byte of serial data
    started = 1; // set the started flag to on
    /*
    for (int i=500; i>0; --i) {
      Serial.println(i);
      delay(1);
    }
    */
    startup();
    digitalWrite(13, started);
  }

  while(started) // loop once serial data has been received
  {
    // Get the currently touched pads
    currtouched = cap.touched();
    
    for (uint8_t i=0; i<12; i++) {
      // it if *is* touched and *wasnt* touched before, alert!
      if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
        Serial.println(-(i+12));
      }
      // if it *was* touched and now *isnt*, alert!
      if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
        Serial.println(-(i+1)); 
      }
    }

    // reset our state
    lasttouched = currtouched;

    //Smoothing
      // subtract the last reading:
    total = total - readings[readIndex];
    // read from the sensor:
    readings[readIndex] = analogRead(inputPin);
    // add the reading to the total:
    total = total + readings[readIndex];
    // advance to the next position in the array:
    readIndex = readIndex + 1;

    // if we're at the end of the array...
    if (readIndex >= numReadings) {
      // ...wrap around to the beginning:
      readIndex = 0;
    }

    // calculate the average:
    average = map(total / numReadings, 200, 800, 0, 127);
      // print the results to the Serial Monitor:
    if (abs(average - oldAverage) > average/40 && average >= 0) { //I've found abt 200 to be the threshold btwn playing and not
      Serial.println(average); // makes it that as you are increasing, the 
      oldAverage = average;
    }

    // comment out this line for detailed data from the cap sensor!
    return;
    
    // debugging info, what
    Serial.print("\t\t\t\t\t\t\t\t\t\t\t\t\t 0x"); Serial.println(cap.touched(), HEX);
    Serial.print("Filt: ");
    for (uint8_t i=0; i<12; i++) {
      Serial.print(cap.filteredData(i)); Serial.print("\t");
    }
    Serial.println();
    Serial.print("Base: ");
    for (uint8_t i=0; i<12; i++) {
      Serial.print(cap.baselineData(i)); Serial.print("\t");
    }
    Serial.println();
    
    // put a delay so it isn't overwhelming
    delay(10);
  }
}
