#include <MIDI.h>
#include <ArduinoBLE.h>

void setup() {
  Serial.begin(57600);
  while (!Serial);
  setupMIDI();
  setupBLE();
}

void loop() {
  processIncomingMIDI();
  handleBLEConnections();
}
