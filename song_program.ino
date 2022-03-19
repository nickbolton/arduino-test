struct Ramp {
  unsigned long source;
  unsigned long start;
  unsigned long duration;
  unsigned long dutyCycle;
  unsigned long cycleStart;
  uint8_t startValue;
  uint8_t endValue;
  uint8_t repeating;
  uint8_t shape;
  uint8_t reversed;
  int8_t currentValue;
};

const int MAX_EVENTS_SIZE = 1024;
const uint8_t TRIANGLE_SHAPE = 0;
const uint8_t SQUARE_SHAPE = 1;
const uint8_t SINE_SHAPE = 2;

unsigned long programEvents[MAX_EVENTS_SIZE];
unsigned long eventDelays[MAX_EVENTS_SIZE];
unsigned long colors[MAX_EVENTS_SIZE];
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
    unsigned long color = parseULong(programArray, idx);
    idx += 4;
    unsigned long rampSource = parseULong(programArray, idx);
    idx += 4;

    sendRemoteLogging(appendLong("packet: ", packet) + ", " + appendLong("delay: ", delay) + "\n");
    programEvents[programEventCount] = packet;
    colors[programEventCount] = color;
    eventDelays[programEventCount] = delay;
    programEventCount++;

    if (rampSource != 0) {
      Ramp ramp;
      ramp.currentValue = -1;
      ramp.source = rampSource;
      ramp.start = parseULong(programArray, idx) + programStartTime;   
      ramp.cycleStart = ramp.start;         
      idx += 4;
      ramp.duration = parseULong(programArray, idx);            
      idx += 4;   
      ramp.startValue = programArray[idx++];
      ramp.endValue = programArray[idx++];
      ramp.repeating = programArray[idx++];
      ramp.reversed = 0;
      ramp.shape = programArray[idx++];
      ramp.dutyCycle = parseULong(programArray, idx);      

      if (ramp.duration > 0 && (!ramp.repeating || ramp.dutyCycle > 0)) {
        ramps[rampCount++] = ramp;
        sendRemoteLogging(appendLong("ramp: ", rampSource) + appendLong(", start: ", ramp.start) + appendLong(", duration: ", ramp.duration) + appendByte(", startValue: ", ramp.startValue) + appendByte(", endValue: ", ramp.endValue) + appendByte(", repeating: ", ramp.repeating) + appendByte(", shape: ", ramp.shape) + "\n");
      }
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

double convertProgressToRampShape(int index, double progress) {
  Ramp ramp = ramps[index];
  
  if (ramp.shape == SQUARE_SHAPE) {
    return progress < 0.5 ? 0.0 : 1.0;
  }
  if (ramp.shape == SINE_SHAPE) {
    return sin(1.570796326794897 * progress);
  }
  return progress;
}

void processRampingEvents() {
  unsigned long now = millis();
  
  for (int i=0; i<rampCount; i++) { 
    Ramp ramp = ramps[i];
    if (ramp.repeating) {
      preProcessRepeatingRamp(i, now);
    } else {
      preProcessRamp(i, now);      
    }
  }
}

void preProcessRepeatingRamp(int index, unsigned long now) {
  Ramp ramp = ramps[index];
  unsigned long globalEndTime = ramp.start + ramp.duration;
  unsigned long cycleEndTime = ramp.cycleStart + ramp.duration;
  double overallProgress = ((double)now - (double)ramp.start) / (double)ramp.duration;
  if (overallProgress >= 1.0) {
    if (ramp.currentValue != ramp.endValue) {
      performRamp(index, 1.0);      
    }
    return;
  }
  
  double cycleProgress = ((double)now - (double)ramp.cycleStart) / (double)ramp.dutyCycle;
  if (cycleProgress > 1.0) {
    ramps[index].reversed = !ramps[index].reversed;
    ramps[index].cycleStart += ramp.dutyCycle;
    preProcessRepeatingRamp(index, now);
    return;
  }

  if (now >= ramp.cycleStart) {
//    sendRemoteLogging(appendLong("ramping packet now: ", now) + appendLong(" start ", ramp.start) + appendLong(" end ", ramp.start + ramp.duration) + appendLong(" progress: ", (unsigned long)(progress * 100.0)) + "\n");
    double truncatedProgress = min(max(cycleProgress, 0.0), 1.0);
    if (ramp.reversed) {
      truncatedProgress = 1.0 - truncatedProgress;
    }
    double shapeProgress = convertProgressToRampShape(index, truncatedProgress);
    performRamp(index, shapeProgress);      
  }  
}

void preProcessRamp(int index, unsigned long now) {
  Ramp ramp = ramps[index];
  unsigned long endTime = ramp.start + ramp.duration;
  double progress = ((double)now - (double)ramp.start) / (double)ramp.duration;
  if (now >= ramp.start) {
    if (now <= endTime || ramp.currentValue != ramp.endValue) {
//        sendRemoteLogging(appendLong("ramping packet now: ", now) + appendLong(" start ", ramp.start) + appendLong(" end ", ramp.start + ramp.duration) + appendLong(" progress: ", (unsigned long)(progress * 100.0)) + "\n");
      double truncatedProgress = min(max(progress, 0.0), 1.0);
      double shapeProgress = convertProgressToRampShape(index, truncatedProgress);
      performRamp(index, shapeProgress);      
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
    sendRemoteLogging(appendInt("processing packet: ", i) + appendInt(" of ", programEventCount) + appendLong(" color: ", colors[i]) + appendLong(" ", programEvents[i]) + "\n");
    processPacket(programEvents[i]);
    setCurrentColor(colors[i]);
    programIndex++;
  }
}
