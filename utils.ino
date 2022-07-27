

String appendBuffer(String str, int length, const uint8_t *data) {
  String result = str;
  for (int i=0; i<length-4; i+=4) {
    unsigned long octet = parseULong(data, i);
    char buf[10];
    snprintf(buf, 10, "%08X", (unsigned long)octet);
    result = result + "\n" + buf;
  }
  return result;
}

String packetString(const unsigned long data) {
  char buf[1000];
  snprintf(buf, 1000, "status: %u, channel: %u, number: %u, value: %u", parsePacketStatus(data), parsePacketChannel(data), parsePacketNumber(data), parsePacketValue(data));
  return buf; 
}

String appendLong(String str, const unsigned long data) {
  char buf[100];
  snprintf(buf, 100, "%08X(%u)", data, data);
  return str + buf; 
}

String appendLongDecimal(String str, const unsigned long data) {
  char buf[10];
  snprintf(buf, 10, "%u", data);
  return str + buf; 
}

String appendDouble(String str, const double data) {
  char buf[1000];
  snprintf(buf, 1000, "%.3f", data);
  return str + buf; 
}

String appendByte(String str, const byte data) {
  char buf[20];
  snprintf(buf, 20, "%02X(%u)", data, data);
  return str + buf; 
}

String appendUint8(String str, const uint8_t data) {
  char buf[20];
  snprintf(buf, 20, "%02X(%u)", data, data);
  return str + buf; 
}

String appendInt(String str, const int data) {
  char buf[20];
  snprintf(buf, 20, "%04X(%u)", data, data);
  return str + buf; 
}

String appendIntDecimal(String str, const int data) {
  char buf[10];
  snprintf(buf, 10, "%u", data);
  return str + buf; 
}

unsigned long parseULong(const uint8_t *programArray, int idx) {
  unsigned long d0 = (unsigned long)programArray[idx];
  unsigned long d1 = (unsigned long)programArray[idx + 1];
  unsigned long d2 = (unsigned long)programArray[idx + 2];
  unsigned long d3 = (unsigned long)programArray[idx + 3];  
  return (d3 << 24) + (d2 << 16) + (d1 << 8) + d0;
}

uint8_t parsePacketStatus(const int data) {
  return (uint8_t)(data >> 24);
}

uint8_t parsePacketChannel(const int data) {
  return (uint8_t)((data & 0xffffff) >> 16);
}

uint8_t parsePacketNumber(const int data) {
  return (uint8_t)((data & 0xffff) >> 8);
}

uint8_t parsePacketValue(const int data) {
  return (uint8_t)(data & 0xff);
}
