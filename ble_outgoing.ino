
const char PERIPHERAL_UUID[37] = "5BF5937B-C8F4-4A82-82CF-3442EC3890CB";
const char CHARACTERISTIC_UUID[37] = "E143E11E-D44C-4C83-8195-D0041EBF09A1";

bool isOutgoingConnected = false;
bool hasPrintedNotConnectedMessage = false; 

bool needsToSendValue = false;
unsigned long packet = 0l;

void setupOutgoingBLE() {
  Serial.println("Scanning for peripherals…");
//  Serial.println(PERIPHERAL_UUID);
  BLE.scan(); //ForUuid(PERIPHERAL_UUID);
}

void handleBLEOutgoingConnections() {
//  Serial.println("handleBLEOutgoingConnections…");
  handleConnectionChanges();
  BLEDevice peripheral = BLE.available();
  if (!peripheral || !peripheral.connected()) { 
//    Serial.print("Outgoing peripheral not available: ");
//    Serial.println(PERIPHERAL_UUID);
    return; 
  }

//  Serial.print("Found peripheral: ");
//  Serial.println(PERIPHERAL_UUID);

  needsToSendValue = true;
  packet = 0xB0010101;

  if (!needsToSendValue) { return; }

  needsToSendValue = false;
  
  BLECharacteristic characteristic = peripheral.characteristic(CHARACTERISTIC_UUID);
  if (!characteristic) { 
//    Serial.print("Could not find characteristic: ");
//    Serial.println(CHARACTERISTIC_UUID);
    return; 
  }

//  Serial.print("Found characteristic: ");
//  Serial.println(CHARACTERISTIC_UUID);

  Serial.print("Sending characteristic packet: ");
  printLong(packet);
  Serial.println();
  
  characteristic.writeValue(packet);

  delay(2000);
}

void handleConnectionChanges() {
  BLEDevice peripheral = BLE.available();
  if (isOutgoingConnected) {
    if (!peripheral || !peripheral.connected()) {
      Serial.print(F("Disconnected from peripheral: "));
      Serial.println(peripheral.address());
      isOutgoingConnected = false;
      Serial.println("\nStarting scan…");
      BLE.scan();
    }
  } else {
    if (!peripheral) {
      if (!hasPrintedNotConnectedMessage) {
        Serial.println("BLE Outgoing Not connected");
        hasPrintedNotConnectedMessage = true;
      }
      return;
    }

    Serial.println("");
    Serial.print("Address: ");
    Serial.println(peripheral.address());

    if (peripheral.hasAdvertisedServiceUuid()) {
      Serial.print("Service UUIDs: ");
      for (int i = 0; i < peripheral.advertisedServiceUuidCount(); i++) {
        Serial.print(peripheral.advertisedServiceUuid(i));
        Serial.print(" ");
      }
    }

    if (peripheral.hasLocalName()) {
      Serial.print(" ");
      Serial.print(peripheral.localName());
      Serial.print(" ");
    }

    Serial.println("");
    Serial.print("RSSI: ");
    Serial.println(peripheral.rssi());

    if (!peripheral.connect()) {
      Serial.println("Failed to connect");        
      return;
    }

    if (!peripheral.hasService(PERIPHERAL_UUID)) {
      Serial.println("No service");    
      peripheral.disconnect();    
      return;
    }

    Serial.print(" uuid: ");
    Serial.print(peripheral.service(PERIPHERAL_UUID).uuid());

    hasPrintedNotConnectedMessage = false;
    
    Serial.print("Connected to midi peripheral: ");
    Serial.println(peripheral.address());  
  
    BLECharacteristic characteristic = peripheral.characteristic(CHARACTERISTIC_UUID);
    if (!characteristic) {
      Serial.println("Peripheral does not have characteristic!");
      peripheral.disconnect();
      return;
    }

    Serial.println("\nStopping scan…");
    BLE.stopScan();
    isOutgoingConnected = true;
  }
}
