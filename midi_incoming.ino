///////////////////////////////

byte incomingMidiChannel = 1;
byte resetDeviceControlNumber = 99;

extern byte lastOutgoingChannel;
extern byte lastOutgoingNumber;
extern byte lastOutgoingValue;

void(*reset) (void) = 0;

void setupIncomingMIDI() {
  Serial.println("setting incoming midi");
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
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
//    String channelString = appendByte("Dropping pc message on channel: ", channel);
//    String configuredString = appendByte(" (", incomingMidiChannel);
//    sendRemoteLogging(channelString + configuredString + ")\n");
    return;
  }

  String channelString = appendByte("rx pc channel: ", channel);  
  String numberString = appendByte(", number: ", number);  
  sendRemoteLogging(channelString + numberString + "\n");
}

void handleControlChange(byte channel, byte number, byte value) {
  if (channel != incomingMidiChannel) {
//    String channelString = appendByte("Dropping cc message on channel: ", channel);
//    String configuredString = appendByte(" (", incomingMidiChannel);
//    sendRemoteLogging(channelString + configuredString + ")\n");
    return;
  }

  if (channel == lastOutgoingChannel && number == lastOutgoingNumber && value == lastOutgoingValue) {
    return;
  }

  if (number == resetDeviceControlNumber && value > 0) {
    reset();
    return;
  }

  String channelString = appendByte("rx cc channel: ", channel);  
  String incomingChannelString = appendByte(", incomingMidiChannel: ", incomingMidiChannel);  
  String numberString = appendByte(", number: ", number);
  String valueString = appendByte(", value: ", value);
  sendRemoteLogging(channelString + incomingChannelString + numberString + valueString + "\n");
  
  sendOutgoingMidi(0xb0, channel, number, value);
}
