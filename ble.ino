
int bleStatus = 0;

void setupBLE() {
  // begin initialization

   bleStatus = BLE.begin();
  if (bleStatus) {
    setupIncomingBLE();
    Serial.println("BLE started successfull");
  } else {
    Serial.println("starting BLE failed!");
  }
}

void handleBLEConnections() {
  if (bleStatus) {
    handleBLEIncomingConnections();
  }
}
