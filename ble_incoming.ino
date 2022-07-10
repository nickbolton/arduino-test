
const char SERVICE_UUID[37] = "3E66DA80-68F7-4BCC-BA12-71D986031662";
const char INCOMING_MIDI_UUID[37] = "698FFA77-1050-409A-8EC9-07377079432E";
const char INCOMING_CHANNEL_UUID[37] = "653FA3B4-8DA0-4261-89A9-E35B39156B54";
const char OUTGOING_MIDI_UUID[37] = "E143E11E-D44C-4C83-8195-D0041EBF09A1";
const char LOGGING_UUID[37] = "AA369159-8F26-4FB5-A66F-ADF7D2D63008";

const byte BANK_STATUS = 0x10;
const byte PC_STATUS = 0xc0;
const byte CC_STATUS = 0xb0;

const int MAX_PROGRAM_SIZE = 10000;

bool isIncomingConnected = false;
bool hasSentLoggingConnectionMessage = false;

BLEService midiService(SERVICE_UUID); // BLE Service

BLECharacteristic songProgram(INCOMING_MIDI_UUID, BLEWrite, MAX_PROGRAM_SIZE);
BLEUnsignedLongCharacteristic outgoingMidi(OUTGOING_MIDI_UUID, BLERead | BLENotify);
BLEByteCharacteristic incomingChannel(INCOMING_CHANNEL_UUID, BLEWrite);
BLECharacteristic logging(LOGGING_UUID, BLERead | BLENotify, 1024);

bool isConnected() {
  BLEDevice central = BLE.central();
  return central && central.connected(); 
}

void setupIncomingBLE() {
  BLE.setLocalName("Setlist Gateway");

  // add the characteristics to the service
  midiService.addCharacteristic(songProgram);
  midiService.addCharacteristic(outgoingMidi);
  midiService.addCharacteristic(incomingChannel);
  midiService.addCharacteristic(logging);

  outgoingMidi.writeValue(0l);

  sendRemoteLogging("Hello\n");

  // add service
  BLE.addService(midiService);

  BLE.setAdvertisedService(midiService);

  // start advertising
  BLE.advertise();

  Serial.println("BLE Incoming initialized");
}

void logConnectionChange() {
  BLEDevice central = BLE.central();
  if (central && central.connected()) {
    if (!isIncomingConnected) {
      sendRemoteLogging("Connected to BLE central\n");
    }
    isIncomingConnected = true;
    setConnectedStatus();
  } else {
    if (isIncomingConnected) {
      sendRemoteLogging("Disconnected from BLE central\n");
    }
    isIncomingConnected = false;
    setDisconnectedStatus();
  }
}

void handleBLEIncomingConnections() {
  logConnectionChange();
  if (!isConnected()) { return; }
  handleIncomingMidi();
  handleIncomingChannel();
  handleLoggingInitialization();
}

void handleIncomingMidi() {
  if (!songProgram.written()) { return; }
  sendRemoteLogging(appendLong("ble rx song program length: ", (unsigned long)songProgram.valueLength()) + "\n");
  if (songProgram.valueLength() <= 0) { return; }
  parseSongProgram(songProgram.value());
}

void handleIncomingChannel() {
  if (!incomingChannel.written()) { return; }
  updateIncomingMidiChannel(incomingChannel.value());
}

void sendOutgoingMidi(const byte status, const byte channel, const byte number, const byte value) {
  sendRemoteLogging("sendOutgoingMidi\n");  
  unsigned long packet = leftShift(status, 24) + leftShift(channel, 16) + leftShift(number, 8) + value;
  sendRemoteLogging(appendLong("ble Sending midi packet to mobile device: ", packet) + "\n");
  outgoingMidi.writeValue(packet);
  outgoingMidi.writeValue(0l);
}

void processPacket(const unsigned long data) {
  byte status = data >> 24;
  byte channel = (data & 0xffffff) >> 16;
  byte number =  (data & 0xffff) >> 8;
  byte value = data & 0xff;
  if (status == BANK_STATUS) {
    String channelString = appendByte("ble sending BANK channel: ", channel);
    String numberString = appendByte(" number: ", number);
    String valueString = appendByte(" pc: ", value);
    sendRemoteLogging(channelString + numberString + valueString + "\n");
    sendControlChange(channel, 0, 0);
    sendControlChange(channel, 0x20, number);
    sendProgramChange(channel, value);
  } else if (status == PC_STATUS) {
    String channelString = appendByte("ble sending PC channel: ", channel);
    String numberString = appendByte(" number: ", number);
    sendRemoteLogging(channelString + numberString + "\n");
    sendProgramChange(channel, number);
  } else if (status == CC_STATUS) {
    String channelString = appendByte("ble sending CC channel: ", channel);
    String numberString = appendByte(" number: ", number);
    String valueString = appendByte(" value: ", value);
    sendRemoteLogging(channelString + numberString + valueString + "\n");
    sendControlChange(channel, number, value);
  }
}

// Remote Logging

String appendBuffer(String str, int length, const uint8_t *data) {
  String result = str;
  for (int i=0; i<length-4; i+=4) {
    unsigned long octet = parseULong(data, i);
    char buf[10];
    snprintf(buf, 10, "%08X", (unsigned long)octet);
    result = result + "\n" + buf;
  }
  return result;
}

String appendLong(String str, const unsigned long data) {
  char buf[10];
  snprintf(buf, 10, "%08X", data);
  return str + buf; 
}

String appendDouble(String str, const double data) {
  char buf[1000];
  snprintf(buf, 1000, "%.3f", data);
  return str + buf; 
}

String appendByte(String str, const byte data) {
  char buf[4];
  snprintf(buf, 4, "%02X", data);
  return str + buf; 
}

String appendUint8(String str, const uint8_t data) {
  char buf[4];
  snprintf(buf, 4, "%02X", data);
  return str + buf; 
}

String appendInt(String str, const int data) {
  char buf[6];
  snprintf(buf, 6, "%04X", data);
  return str + buf; 
}

unsigned long parseULong(const uint8_t *programArray, int idx) {
  unsigned long d0 = (unsigned long)programArray[idx];
  unsigned long d1 = (unsigned long)programArray[idx + 1];
  unsigned long d2 = (unsigned long)programArray[idx + 2];
  unsigned long d3 = (unsigned long)programArray[idx + 3];  
  return (d3 << 24) + (d2 << 16) + (d1 << 8) + d0;
}

void sendRemoteLogging(String message) {
  Serial.print(message);
  int length = message.length() + 1;
  byte messageBytes[length];
  message.getBytes(messageBytes, length);
  logging.writeValue(messageBytes, length);
}

void sendRemoteLogging(byte b) {
  char s[4];
  snprintf(s, 4, "%d", b);
  sendRemoteLogging(s);
}

void handleLoggingInitialization() {
  if (hasSentLoggingConnectionMessage) { return; }
  sendRemoteLogging("Remote logger connected\n");
  hasSentLoggingConnectionMessage = true;
}

bool doesLoggingCharacteristicExist() {
  BLEDevice peripheral = BLE.available();
  if (!peripheral || !peripheral.connected()) { 
    return false; 
  }

  return peripheral.characteristic(LOGGING_UUID);
}
