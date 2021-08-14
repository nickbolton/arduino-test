
byte lastOutgoingChannel = 0;
byte lastOutgoingNumber = 0;
byte lastOutgoingValue = 0;

void sendControlChange(byte channel, byte number, byte value) {
  lastOutgoingChannel = channel;
  lastOutgoingNumber = number;
  lastOutgoingValue = value;

  String channelString = appendByte("tx cc channel: ", channel);  
  String numberString = appendByte(", number: ", number);
  String valueString = appendByte(", value: ", value);
  sendRemoteLogging(channelString + numberString + valueString + "\n");

  MIDI.sendControlChange(number, value, channel);
}

void sendProgramChange(byte channel, byte number) {
  lastOutgoingChannel = channel;
  lastOutgoingNumber = number;
  lastOutgoingValue = 0;

  String channelString = appendByte("tx pc channel: ", channel);  
  String numberString = appendByte(", number: ", number);  
  sendRemoteLogging(channelString + numberString + "\n");

  MIDI.sendProgramChange(number, channel);
}
