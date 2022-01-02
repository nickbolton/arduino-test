struct Ramp {
  unsigned long source;
  unsigned long start;
  unsigned long duration;
  uint8_t startValue;
  uint8_t endValue;
  uint8_t repeating;
  uint8_t shape;
  int8_t currentValue;
};

unsigned long programEvents[MAX_EVENTS_SIZE];
unsigned long eventDelays[MAX_EVENTS_SIZE];
Ramp ramps[MAX_EVENTS_SIZE];

int rampCount = 0;

int programIndex = 0;
int programEventCount = 0;
unsigned long programStartTime = 0;

void parseSongProgram(const uint8_t *programArray) {

  programStartTime = millis();

//  for (int i=0; i<songProgram.valueLength(); i++) {
//    sendRemoteLogging(appendByte("byte: ", programArray[i]) + "\n");
//  }
  
  unsigned long count = parseULong(programArray, 0);
  int idx = 4;
  rampCount = 0;
  programIndex = 0;
  programEventCount = 0;

  sendRemoteLogging(appendInt("song program count: ", count) + "\n");

  while (idx < songProgram.valueLength()) {    
    unsigned long packet = parseULong(programArray, idx);
    idx += 4;
    unsigned long delay = parseULong(programArray, idx);
    idx += 4;
    unsigned long rampSource = parseULong(programArray, idx);
    idx += 4;

    sendRemoteLogging(appendLong("packet: ", packet) + ", " + appendLong("delay: ", delay) + "\n");
    programEvents[programEventCount] = packet;
    eventDelays[programEventCount++] = delay;

    if (rampSource != 0) {
      Ramp ramp;
      ramp.currentValue = -1;
      ramp.source = rampSource;
      ramp.start = parseULong(programArray, idx) + programStartTime;            
      idx += 4;
      ramp.duration = parseULong(programArray, idx);            
      idx += 4;   
      ramp.startValue = programArray[idx++];
      ramp.endValue = programArray[idx++];
      ramp.repeating = programArray[idx++];
      ramp.shape = programArray[idx++];
      ramps[rampCount++] = ramp;

      sendRemoteLogging(appendLong("ramp: ", rampSource) + appendLong(", start: ", ramp.start) + appendLong(", duration: ", ramp.duration) + appendByte(", startValue: ", ramp.startValue) + appendByte(", endValue: ", ramp.endValue) + appendByte(", repeating: ", ramp.repeating) + appendByte(", shape: ", ramp.shape) + "\n");
    }
  }

  sendRemoteLogging(appendLong("packet processing took: ", millis() - programStartTime) + " ms\n");

  processProgramEvents();
}

unsigned long parseULong(const uint8_t *programArray, int idx) {
  unsigned long d0 = (unsigned long)programArray[idx];
  unsigned long d1 = (unsigned long)programArray[idx + 1];
  unsigned long d2 = (unsigned long)programArray[idx + 2];
  unsigned long d3 = (unsigned long)programArray[idx + 3];  
  return (d3 << 24) + (d2 << 16) + (d1 << 8) + d0;
}

void performRamp(int index, double progress) {
  Ramp ramp = ramps[index];
  int8_t value = ramp.startValue + (int8_t)((double)(ramp.endValue - ramp.startValue) * progress);
  if (value == ramp.currentValue) {
    return;
  }
  ramps[index].currentValue = value;
  unsigned long basePacket =  ramp.source & 0xFFFFFF00;
  unsigned long packet = basePacket + (uint8_t)value;
  sendRemoteLogging(appendLong("ramping packet: ", ramp.source) + appendLong(" base packet: ", basePacket) + appendLong(" value ", value) + appendLong(" updated: ", packet) + "\n");
  processPacket(packet);  
}

void processRampingEvents() {
  unsigned long now = millis();
  
  for (int i=0; i<rampCount; i++) { 
    Ramp ramp = ramps[i];
    unsigned long endTime = ramp.start + ramp.duration;
    double progress = ((double)now - (double)ramp.start) / (double)ramp.duration;
    if (now >= ramp.start) {
      if (now <= endTime || ramp.currentValue != ramp.endValue) {
//        sendRemoteLogging(appendLong("ramping packet now: ", now) + appendLong(" start ", ramp.start) + appendLong(" end ", ramp.start + ramp.duration) + appendLong(" progress: ", (unsigned long)(progress * 100.0)) + "\n");
        performRamp(i, min(max(progress, 0.0), 1.0));      
      }
    }
  }
}

void processProgramEvents() {
  if (programIndex >= programEventCount) {
    return;
  }

  unsigned long elapsedTime = millis() - programStartTime;

  for (int i=programIndex; i<programEventCount; i++) {
    if (eventDelays[i] > elapsedTime) {
      return;
    }
    sendRemoteLogging(appendInt("processing packet: ", i) + appendInt(" of ", programEventCount) + appendLong(" ", programEvents[i]) + "\n");
    processPacket(programEvents[i]);
    programIndex++;
  }
}
