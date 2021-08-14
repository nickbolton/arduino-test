
void setupBLE() {
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");

    while (1);
  }
  setupIncomingBLE();
}

void handleBLEConnections() {
  handleBLEIncomingConnections();
}

unsigned long leftShift(const byte b, const byte bits) {
  unsigned long value = (unsigned long)b;
  return value << bits;
}
