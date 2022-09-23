
AD5242 AD01(0x2C);  // AD0 & AD1 == GND

void setupVolume()
{
  Serial.println();
  Serial.println(__FILE__);
  Serial.println();
  Serial.println(AD524X_LIB_VERSION);

  Wire.begin();
  Wire.setClock(400000);

  Serial.println("****************");
  
  bool b = AD01.begin();
  Serial.print("OK: ");
  Serial.println(b ? "true" : "false");
  Serial.print("Connected: ");
  Serial.println(AD01.isConnected());
  Serial.print("pot count: ");
  Serial.println(AD01.pmCount());
  Serial.println("****************");
}

void adjustVolume(uint8_t value) {
  
}

void processVolume() {
  Serial.println(0);
  AD01.write(0, 0);
  delay(5000);
  Serial.println(127);
  AD01.write(0, 127);
  delay(5000);
  Serial.println(255);
  AD01.write(0, 255);
  delay(5000);
  Serial.println(127);
  AD01.write(0, 127);
  delay(5000);
//  for (int val = 0; val < 255; val++) {
//    AD01.write(1, val);
//    if (val == 200) {
//      AD01.write(1, val, HIGH, LOW);
//    }
//    if (val == 0) {
//      AD01.write(1, val, LOW, LOW);
//    }
//    Serial.println(val);
//    delay(500);
//  }
}
