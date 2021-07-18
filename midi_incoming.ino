        ///////////////////////////////
byte incomingMidiChannel = 16;

void setupIncomingMIDI() {
  Serial.println("setting incoming midi");
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleProgramChange(handleProgramChange);
  MIDI.setHandleControlChange(handleControlChange);
}

void updateIncomingMidiChannel(byte channel) {
  incomingMidiChannel = channel;
}
 
void processIncomingMIDI() {
  MIDI.read();
}

void handleProgramChange(byte channel, byte number) {
  if (channel != incomingMidiChannel) {
    Serial.print("Dropping pc message on channel: ");  
    Serial.print(channel);  
    Serial.print(". Controller is configured for channel: ");  
    Serial.println(incomingMidiChannel);  
    return;
  }
  Serial.print("rx pc channel: ");
  Serial.print(channel);  
  Serial.print(", number: ");
  Serial.println(number);  
}

void handleControlChange(byte channel, byte number, byte value) {
  if (channel != incomingMidiChannel) {
    Serial.print("Dropping cc message on channel: ");  
    Serial.print(channel);  
    Serial.print(". Controller is configured for channel: ");  
    Serial.println(incomingMidiChannel);  
    return;
  }
  Serial.print("rx cc channel22: ");
  Serial.print(channel);  
  Serial.print(", number: ");
  Serial.print(number);  
  Serial.print(", value: ");
  Serial.println(value);  
  sendOutgoingMidi(0xb0, channel, number, value);
}
