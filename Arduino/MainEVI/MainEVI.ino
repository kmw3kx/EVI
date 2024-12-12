// Bob Kammauff (kmw3kx)
// Arduino Uno EVI
//12-7-24

/*
Goal: Build a USB Wind-Controller using an Arduino Uno

I want to use some object-oriented programming to make this cleaner

And also separate function definition files

Nah that doesn't seem to want to work for me here
*/



// For MPR121 Capacitive
#include <Wire.h>
#include "Adafruit_MPR121.h"

bool MPR_PinStates[12];

#ifndef _BV 
#define _BV(n) (1 << (n)) // sets nth bit to one
#endif



// Capacitive Touch Pin Assignments
// 11 10 9 8  7 6 5 4  3 2 1 0
#define VALVE_PINS {6, 7, 8, 9} //pins 6-9, in numerical order (1234)
#define MOUTHTPIECE_PIN 12 // unused as of now
#define OCTAVE_PINS {0, 1, 2, 3, 4, 5} //pins 0-5, from lowest to highest
bool valveChange = false;

int getOctaveShift(uint16_t currState, int OctavePins[6] = OCTAVE_PINS) { //Returns the Octave Shift 0-5
  for (int i=5; i>0; i--) {
    pin = OctavePins[i];
    if (currState & _BV(pin)) {
      return i;
      break;
    }
  }
}

bool getValvePos(bool valvePos[5], uint16_t currState, uint16_t pastState) {
  // gets the position of the valves (1-4), and if the player is engaging the mouthpiece (0)
  bool change = false;
  for (uint8_t i=0; i<4; i++) { // Pins need to be sequential
    // it if *is* touched and *wasnt* touched before, alert!
    if ((currState & _BV(i)) && !(pastState & _BV(i)) ) {
      //Serial.print(i); Serial.println(" touched");
      valvePos[i] = true;
      change = true;
    }
    // if it *was* touched and now *isnt*, alert!
    if (!(currState & _BV(i)) && (pastState & _BV(i)) ) {
      //Serial.print(i); Serial.println(" released");
      valvePos[i] = false;
      change = true;
    }
    return change == true;
  }
}

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap = Adafruit_MPR121();

// Keeps track of the last pins touched
// so we know when buttons are 'released'
uint16_t lasttouched = 0;
uint16_t currtouched = 0;

// For USBMIDI
#include <midi_serialization.h>
#include <usbmidi.h>

// for channel 0
#define NOTE_ON 0x90 
#define NOTE_OFF 0x80

// See midictrl.png in the example folder for the wiring diagram,
// as well as README.md.

void sendCC(uint8_t channel, uint8_t control, uint8_t value) {
	Serial.write(0xB0 | (channel & 0xf));
	Serial.write(control & 0x7f);
	Serial.write(value & 0x7f);
}

void sendNote(uint8_t channel, uint8_t note, uint8_t velocity) {
	Serial.write((velocity != 0 ? 0x90 : 0x80) | (channel & 0xf));
	Serial.write(note & 0x7f);
	Serial.write(velocity &0x7f);
}

// Instrument Math

#define BREATH_PIN A0
#define FUNDAMENTAL_MIDI 34 // Low Bb on Tuba
#define breathThres 1
bool valvePos[5] = {false, false, false, false, false};
int octShift = 0;
int octOld = 0;
int valveShift = 0;
uint8_t midiNote = 0;
uint8_t velocity = 96;
int currBreath;
int pastBreath;

int getBreath() {
  // Get Breath Reading
  return analogRead(BREATH_PIN) >> 3; // converts from 10 bit to 7 bit
}

//#define INSTRUMENT_KEY -2 // For a Bb Instrument // Only necessary if I want a transposing instrument

int getValvePitchShift(bool valvePos[5]) {
  // Calculates how many semitones down the valve combination creates
  return -(valvePos[1]*2 + valvePos[2]*1 + valvePos[3]*3 + valvePos[4]*5);
}

int getMidiNote(bool valvePos[5], int octShift) {
  // calculate desired note value
  return FUNDAMENTAL_MIDI -(valvePos[1]*2 + valvePos[2]*1 + valvePos[3]*3 + valvePos[4]*5) + 12*octShift;
}

void setup() {
  // put your setup code here, to run once:
  
  //Initiate Serial Monitor
  Serial.begin(9600);
  //set up i2c for OLED display & Capacitive Touch
  
  //Serial.println("Setting up MPR121 Capacitive Touch Sensor..."); 
  
  // Default address is 0x5A, if tied to 3.3V its 0x5B
  // If tied to SDA its 0x5C and if SCL then 0x5D
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  //Serial.println("MPR121 found!");
  //Get initial values for sensors
  const int breathInit = getBreath();
  currBreath = breathInit;
  pastBreath = breathInit;
}

void loop() {
  // put your main code here, to run repeatedly:

  // Get the currently touched pads
  currtouched = cap.touched();
  currBreath = getBreath();
  octShift = getOctaveShift(currtouched);
  //Get Valve Positions and calculate note
  valveChange = getValvePos(valvePos, currtouched, lasttouched); // written to valvePos[5]
  if (valveChange || octShift != octOld) {
    //Send Midi Off for previous note
    sendNote(0, midiNote, 0);
    valveShift = getValvePitchShift(valvePos);
    midiNote = getMidiNote(valveShift, octShift);
    //Send Midi On for new note
    //sendNote(0, midiNote, velocity);
    //if sending to max
    Serial.println(midiNote);
  }
  // Check CC change
  if (currBreath != pastBreath) {
    //sendCC(0, 0x0B, currBreath);
    //if sending to Max
    Serial.println(currBreath);
  }
  // reset our state
  lasttouched = currtouched;
  pastBreath = currBreath;
  delay(10);
  // 
}
