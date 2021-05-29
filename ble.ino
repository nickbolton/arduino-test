
void setupBLE() {
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");

    while (1);
  }
  setupIncomingBLE();
//  setupOutgoingBLE();
}

void handleBLEConnections() {
  handleBLEIncomingConnections();
//  handleBLEOutgoingConnections();
}

unsigned long leftShift(const byte b, const byte bits) {
  unsigned long value = (unsigned long)b;
  return value << bits;
}

void printData(const unsigned long data) {

    if (data < 0x10) {
      Serial.print("0");
    }
    if (data < 0x100) {
      Serial.print("0");
    }
    if (data < 0x1000) {
      Serial.print("0");
    }
    if (data < 0x10000) {
      Serial.print("0");
    }
    if (data < 0x100000) {
      Serial.print("0");
    }
    if (data < 0x1000000) {
      Serial.print("0");
    }
    if (data < 0x1000000) {
      Serial.print("0");
    }

    Serial.println(data, HEX);
}
