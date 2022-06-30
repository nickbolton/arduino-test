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

enum ProgramStatus: unsigned long {
  UNLOADED = 0,
  LOADED,
  RUNNING,
  PAUSED,
  STOPPED,
};

const int MAX_EVENTS_SIZE = 1024;
const uint8_t TRIANGLE_SHAPE = 0;
const uint8_t SQUARE_SHAPE = 1;
const uint8_t SINE_SHAPE = 2;

const int ULONG_SIZE = sizeof(unsigned long);

unsigned long status = UNLOADED;

unsigned long programEvents[MAX_EVENTS_SIZE];
unsigned long startEvents[MAX_EVENTS_SIZE];
unsigned long stopEvents[MAX_EVENTS_SIZE];
unsigned long eventDelays[MAX_EVENTS_SIZE];
unsigned long colors[MAX_EVENTS_SIZE];
Ramp ramps[MAX_EVENTS_SIZE];

int rampCount = 0;

bool processedStartEvents = false;
bool processedStopEvents = false;
int programIndex = 0;
int programEventCount = 0;
int startEventsCount = 0;
int stopEventsCount = 0;
unsigned long programStartTime = 0;
unsigned long minEventTime = 0;

void resetProgram() {
  rampCount = 0;
  programIndex = 0;
  programEventCount = 0;
  processedStartEvents = false;
  processedStopEvents = false;
}

void parseSongProgram(const uint8_t *programArray) {

//  for (int i=0; i<songProgram.valueLength(); i++) {
//    sendRemoteLogging(appendByte("byte: ", programArray[i]) + "\n");
//  }

  int idx = 0;

  unsigned long newStatus = parseULong(programArray, idx);
  idx += ULONG_SIZE;
  unsigned long firstEventTime = parseULong(programArray, idx);
  idx += ULONG_SIZE;

  unsigned long startCount = parseULong(programArray, idx);
  idx += ULONG_SIZE;
  unsigned long stopCount = parseULong(programArray, idx);
  idx += ULONG_SIZE;
  unsigned long eventsCount = parseULong(programArray, idx);
  idx += ULONG_SIZE;

  switch (newStatus) {
    case UNLOADED:
      status = newStatus;
      resetProgram();
      startEventsCount = 0;
      stopEventsCount = 0;
      sendRemoteLogging("Song Program UNLOADED\n");
      return;
    case LOADED:
      if (status != UNLOADED) {
        return;
      }
      status = newStatus;
      startEventsCount = startCount;
      stopEventsCount = stopCount;
      sendRemoteLogging("Song Program LOADED\n");
      break;
    case RUNNING:
      programStartTime = millis();
      minEventTime = firstEventTime;
      status = newStatus;
      sendRemoteLogging(appendLong("Song Program STARTED at ", firstEventTime) + "\n");
      if (eventsCount > 0) {
        resetProgram();
      } else {
        processProgramEvents(); 
        return;
      }
      break;
    case PAUSED:
      if (status != RUNNING) {
        return;
      }
      status = newStatus;
      sendRemoteLogging("Song Program PAUSED\n");
      return;
    case STOPPED:
      status = newStatus;
      sendRemoteLogging("Song Program STOPPED\n");
      processProgramEvents();
      return;
    default:
      status = UNLOADED;
      resetProgram();
      startEventsCount = 0;
      stopEventsCount = 0;
      sendRemoteLogging(appendLong("unknown status: ", newStatus) + "\n");
      return;      
  }

  unsigned long parseStartTime = millis();  
  resetProgram();  

  unsigned long startEventIndex = 0;
  while (startEventIndex < startEventsCount) {
    startEvents[startEventIndex] = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    startEventIndex ++;
  }

  sendRemoteLogging(appendInt("startEventsCount: ", startEventsCount) + "\n");

  unsigned long stopEventIndex = 0;
  while (stopEventIndex < stopEventsCount) {
    stopEvents[stopEventIndex] = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    stopEventIndex ++;
  }

  sendRemoteLogging(appendInt("stopEventsCount: ", stopEventsCount) + "\n");
  sendRemoteLogging(appendInt("event parsing start idx: ", idx) + "\n");

  while (idx < songProgram.valueLength()) {    
    unsigned long packet = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    unsigned long delay = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    unsigned long color = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    unsigned long rampSource = parseULong(programArray, idx);
    idx += ULONG_SIZE;

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
      idx += ULONG_SIZE;
      ramp.duration = parseULong(programArray, idx);            
      idx += ULONG_SIZE;
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

  sendRemoteLogging(appendInt("idx: ", idx) + "\n");
  sendRemoteLogging(appendInt("song program count: ", programEventCount) + "\n");
  sendRemoteLogging(appendLong("song program parsing took: ", millis() - parseStartTime) + " ms\n");

  processProgramEvents();

//  sendRemoteLogging(appendBuffer("song program buffer: ", idx, programArray) + "\n");
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

void processStartEvents() {
  if (processedStartEvents) {
    return;
  }
  sendRemoteLogging(appendInt("startEventsCount2: ", startEventsCount) + "\n");
  
  for (int i=0; i<startEventsCount; i++) {
    sendRemoteLogging(appendInt("processing start event: ", i) + appendInt(" of ", startEventsCount) + "\n");
    processPacket(startEvents[i]);
  }
  processedStartEvents = true;
}

void processStopEvents() {
  if (processedStopEvents) {
    return;
  }
  for (int i=0; i<stopEventsCount; i++) {
    sendRemoteLogging(appendInt("processing stop event: ", i) + appendInt(" of ", stopEventsCount) + "\n");
    processPacket(stopEvents[i]);
  }
  processedStopEvents = true;
}

void processProgramEvents() {
  switch (status) {
    case UNLOADED:
      break;
    case LOADED:
      processStartEvents();
      break;
    case RUNNING:
      processRunningEvents();
      break;
    case STOPPED:
      processStopEvents();
      break;
  }
}

void processRunningEvents() {

//  sendRemoteLogging(appendLong("status: ", status) + "\n");
  
  if (programIndex >= programEventCount || status != RUNNING) {
//    sendRemoteLogging(appendLong("programIndex: ", programIndex) + "\n");
//    sendRemoteLogging(appendLong("programEventCount: ", programEventCount) + "\n");
//    sendRemoteLogging("Bailing 1…\n");
    return;
  }

  unsigned long now = millis();
  unsigned long elapsedTime = now - programStartTime;

//  sendRemoteLogging(appendLong("minEventTime: ", minEventTime) + "\n");
//  sendRemoteLogging(appendLong("elapsedTime: ", elapsedTime) + "\n");
//  sendRemoteLogging(appendInt("programIndex: ", programIndex) + "\n");
//  sendRemoteLogging(appendLong("eventDelays[programIndex]: ", eventDelays[programIndex]) + "\n");

  if (now < minEventTime) {
//    sendRemoteLogging("Bailing 2…\n");
    return;
  }

  // first skip all events prior to minEventTime
  for (; programIndex<programEventCount; programIndex++) {
    if (eventDelays[programIndex] >= minEventTime) {
      break;
    }
    sendRemoteLogging(appendLong("skipping event: ", programIndex) + "\n");
  }

  for (; programIndex<programEventCount; programIndex++) {
    if ((eventDelays[programIndex] - minEventTime) > elapsedTime) {
//      sendRemoteLogging("Bailing 3…\n");
      return;
    }
    sendRemoteLogging(appendInt("processing packet: ", programIndex) + appendInt(" of ", programEventCount) + appendLong(" color: ", colors[programIndex]) + appendLong(" ", programEvents[programIndex]) + "\n");
    processPacket(programEvents[programIndex]);
    setCurrentColor(colors[programIndex]);
  }
}
