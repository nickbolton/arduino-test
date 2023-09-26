#include <MIDI.h>
#include <ArduinoBLE.h>

bool isDebugLogging = false;
bool isTraceLogging = false;

void setup() {
  Serial.begin(9600);
//  while (!Serial);
  delayMicroseconds(2000000);
  Serial.println("Hey look, the Serial Monitor is working just fine!");
  setupMIDI();
  setupBLE();
  setupStatus();
}

void loop() {
  handleBLEConnections();
  processProgramEvents();
  processRampingEvents();
  processIncomingMIDI();
  processStatus();
}
