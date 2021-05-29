
void sendControlChange(byte channel, byte number, byte value) {
  MIDI.sendControlChange(number, value, channel);
}

void sendProgramChange(byte channel, byte number) {
  MIDI.sendProgramChange(number, channel);
}
