
int getOctaveShift(uint16_t currState, int OctavePins[6] = OCTAVE_PINS) { //Returns the Octave Shift 0-5
  for (int i=5; i>0; i--) {
    pin = OctavePins[i];
    if (currState & _BV(pin)) {
      return i;
      break;
    }
  }
}

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
