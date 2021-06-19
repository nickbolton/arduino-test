#include <MIDI.h>
#include <ArduinoBLE.h>

void setup() {
  Serial.begin(9600);
//  while (!Serial);
  Serial.println("Hey look, the Serial Monitor is working just fine!");
  setupMIDI();
  setupBLE();
}

void loop() {
  processIncomingMIDI();
  handleBLEConnections();
}
