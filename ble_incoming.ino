
const char SERVICE_UUID[37] = "3E66DA80-68F7-4BCC-BA12-71D986031662";
const char INCOMING_MIDI_UUID[37] = "698FFA77-1050-409A-8EC9-07377079432E";
const char INCOMING_CHANNEL_UUID[37] = "653FA3B4-8DA0-4261-89A9-E35B39156B54";
const char OUTGOING_MIDI_UUID[37] = "E143E11E-D44C-4C83-8195-D0041EBF09A1";

const byte PC_STATUS = 0xc0;
const byte CC_STATUS = 0xb0;

bool isIncomingConnected = false;

BLEService midiService(SERVICE_UUID); // BLE Service

BLEUnsignedLongCharacteristic incomingMidi(INCOMING_MIDI_UUID, BLEWrite);
BLEUnsignedLongCharacteristic outgoingMidi(OUTGOING_MIDI_UUID, BLERead | BLENotify);
BLEByteCharacteristic incomingChannel(INCOMING_CHANNEL_UUID, BLEWrite);

void setupIncomingBLE() {
  BLE.setLocalName("Setlist Gateway");

  // add the characteristics to the service
  midiService.addCharacteristic(incomingMidi);
  midiService.addCharacteristic(outgoingMidi);
  midiService.addCharacteristic(incomingChannel);

  outgoingMidi.writeValue(0l);

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
      Serial.print("Connected to central: ");
      Serial.println(central.address());  
    }
    isIncomingConnected = true;
  } else {
    if (isIncomingConnected) {
      Serial.print(F("Disconnected from central: "));
      Serial.println(central.address());
    }
    isIncomingConnected = false;
  }
}

void handleBLEIncomingConnections() {
  logConnectionChange();
  BLEDevice central = BLE.central();
  if (!central || !central.connected()) { return; }

  handleIncomingMidi();
  handleIncomingChannel();
}

void handleIncomingMidi() {
  if (!incomingMidi.written()) { return; }
  Serial.print("packet length: ");
  Serial.println(incomingMidi.valueLength());

  if (incomingMidi.valueLength() != 4) { return; }
  long packet = incomingMidi.value();
  Serial.print("packet: 0x");
  printData(packet);
  Serial.println();
  processPacket(packet);
}

void handleIncomingChannel() {
  if (!incomingChannel.written()) { return; }
  updateIncomingMidiChannel(incomingChannel.value());
}

void sendOutgoingMidi(const byte status, const byte number, const byte value) {
  BLEDevice central = BLE.central();
  if (!central || !central.connected()) { return; }
  unsigned long packet = leftShift(status, 24) + leftShift(number, 8) + value;
  Serial.print("Sending midi packet to mobile device: ");
  printData(packet);
  outgoingMidi.writeValue(packet);
  outgoingMidi.writeValue(0l);
}

void processPacket(const unsigned long data) {
  byte status = data >> 24;
  byte channel = (data & 0xffffff) >> 16;
  byte number =  (data & 0xffff) >> 8;
  byte value = data & 0xff;
  if (status == PC_STATUS) {
    sendProgramChange(channel, number);
  } else if (status == CC_STATUS) {
    sendControlChange(channel, number, value);
  }
}
