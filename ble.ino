
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
