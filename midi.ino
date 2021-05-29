
MIDI_CREATE_INSTANCE(HardwareSerial,Serial1, MIDI); // create a MIDI object

void setupMIDI() {
  setupIncomingMIDI();
  Serial1.begin(31250); // setup serial for MIDI
}
