struct Ramp {
  unsigned long source;
  double start;
  double end;
  double duration;
  double dutyCycle;
  double cycleStart;
  uint8_t eventType;
  uint8_t startValue;
  uint8_t endValue;
  uint8_t shape;
  uint8_t reversed;
  int8_t currentValue;
  bool ended;
  int count;
};

struct ProgramEvent {
  unsigned long packet;
  unsigned long delay;
  unsigned long color;
  uint8_t eventType;
  uint8_t volumeValue;
  uint8_t isStartEvent;
};

enum EventType: uint8_t {
  MIDI_EVENT = 0,
};

enum ProgramStatus: unsigned long {
  UNLOADED = 0,
  LOADED,
  RUNNING,
  PAUSED,
  STOPPED,
};

const int MAX_EVENTS_SIZE = 512;
const uint8_t TRIANGLE_SHAPE = 0;
const uint8_t SQUARE_SHAPE = 1;
const uint8_t SINE_SHAPE = 2;
const uint8_t EXPONENTIAL_SHAPE = 3;
const double EXPONENTIAL_STEEPNESS = 15.0;
const double PI_2 = PI/2.0;

const int ULONG_SIZE = sizeof(unsigned long);

unsigned long status = UNLOADED;

ProgramEvent events[MAX_EVENTS_SIZE];
unsigned long stopEvents[MAX_EVENTS_SIZE];
Ramp ramps[MAX_EVENTS_SIZE];

int rampCount = 0;

bool processedStartEvents = false;
bool processedStopEvents = false;
int programIndex = 0;
int programEventCount = 0;
int stopEventsCount = 0;
double programStartTime = 0;
double minEventTime = 0;

void resetProgram() {
  rampCount = 0;
  programIndex = 0;
  programEventCount = 0;
  processedStartEvents = false;
  processedStopEvents = false;
}

void parseSongProgram(const uint8_t *programArray) {

  if (isTraceLogging) {
    for (int i=0; i<songProgram.valueLength(); i++) {
      trace(appendByte("byte: ", programArray[i]));
    }
  }

  int idx = 0;

  unsigned long newStatus = parseULong(programArray, idx);
  idx += ULONG_SIZE;
  unsigned long firstEventTime = parseULong(programArray, idx);
  idx += ULONG_SIZE;

  unsigned long stopCount = parseULong(programArray, idx);
  idx += ULONG_SIZE;
  unsigned long eventCount = parseULong(programArray, idx);
  idx += ULONG_SIZE;
  unsigned long rampCountIn = parseULong(programArray, idx);
  idx += ULONG_SIZE;

  switch (newStatus) {
    case UNLOADED:
      status = newStatus;
      resetProgram();
      stopEventsCount = 0;
      setCurrentColor(0);
      sendRemoteLogging("Song Program UNLOADED\n");
      return;
    case LOADED:
      if (status != UNLOADED) {
        return;
      }
      status = newStatus;
      sendRemoteLogging("Song Program LOADED\n");
      break;
    case RUNNING:
      programStartTime = (double)millis();
      minEventTime = (double)firstEventTime;
      status = newStatus;
      sendRemoteLogging(appendLong("Song Program STARTED at ", firstEventTime) + "\n");
      if (eventCount > 0) {
        resetProgram();
      } else {
        resetRamps();
        programIndex = 0;
        processProgramEvents(); 
        return;
      }
      break;
    case PAUSED:
      if (status != RUNNING) {
        return;
      }
      status = newStatus;
      resetRamps();
      sendRemoteLogging("Song Program PAUSED\n");
      return;
    case STOPPED:
      status = newStatus;
      sendRemoteLogging("Song Program STOPPED\n");
      resetRamps();
      processProgramEvents();
      return;
    default:
      status = UNLOADED;
      resetProgram();
      stopEventsCount = 0;
      sendRemoteLogging(appendLong("unknown status: ", newStatus) + "\n");
      return;      
  }

  unsigned long parseStartTime = millis();  
  resetProgram();  

  stopEventsCount = stopCount;
  programEventCount = eventCount;
  rampCount = rampCountIn;

  sendRemoteLogging(appendInt("stopEventsCount: ", stopEventsCount) + appendInt(" idx: ", idx) + "\n");

  unsigned long eventIndex = 0;
  while (eventIndex < stopEventsCount && idx < songProgram.valueLength()) {
    stopEvents[eventIndex] = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    eventIndex++;
  }

  sendRemoteLogging(appendInt("programEventCount: ", programEventCount) + appendInt(" idx: ", idx) + "\n");

  eventIndex = 0;
  while (eventIndex < programEventCount && idx < songProgram.valueLength()) {
    ProgramEvent event;
    event.eventType = programArray[idx++];
    event.packet = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    event.delay = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    event.color = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    event.volumeValue = programArray[idx++];
    event.isStartEvent = programArray[idx++];

    sendRemoteLogging(appendUint8("event type: ", event.eventType) + ", " + appendLong("packet: ", event.packet) + ", " + packetString(event.packet) + appendLong(", delay: ", event.delay) + appendUint8(" volume value: ", event.volumeValue) + appendUint8(" isStartEvent: ", event.isStartEvent) + "\n");
    events[eventIndex] = event;
    eventIndex++;
  }

  sendRemoteLogging(appendInt("rampCount: ", rampCount) + appendInt(" idx: ", idx) + appendInt(" songProgram.valueLength: ", songProgram.valueLength()) + "\n");

  int rampIndex = 0;
  while (rampIndex < rampCount && idx < songProgram.valueLength()) {
    unsigned long rampSource = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    
    Ramp ramp;
    ramp.eventType = programArray[idx++];
    ramp.currentValue = -1;
    ramp.source = rampSource;
    ramp.start = (double)parseULong(programArray, idx);   
    idx += ULONG_SIZE;
    ramp.cycleStart = ramp.start;         
    ramp.duration = (double)parseULong(programArray, idx);     
    ramp.end = ramp.start + ramp.duration;       
    idx += ULONG_SIZE;
    ramp.dutyCycle = (double)parseULong(programArray, idx);      
    idx += ULONG_SIZE;
    ramp.startValue = programArray[idx++];
    ramp.endValue = programArray[idx++];
    ramp.reversed = 0;
    ramp.shape = programArray[idx++];
    ramp.count = 0;

    if (status == RUNNING) {
      ramp.ended = minEventTime >= ramp.start;
    } else {
      ramp.ended = false;
    }

    if (ramp.duration > 0 && ramp.dutyCycle > 0) {
      ramps[rampIndex++] = ramp;
      sendRemoteLogging(appendLong("ramp: ", rampSource) + ", " + packetString(rampSource) + appendDouble(", start: ", ramp.start) + appendDouble(", duration: ", ramp.duration) + appendDouble(", end: ", ramp.end) + appendDouble(", cycleStart: ", ramp.cycleStart) + appendDouble(", dutyCycle: ", ramp.dutyCycle) + appendDouble(", cycleEnd: ", (ramp.cycleStart + ramp.dutyCycle)) + appendByte(", startValue: ", ramp.startValue) + appendByte(", endValue: ", ramp.endValue) + appendByte(", shape: ", ramp.shape) + appendInt(", ended: ", ramp.ended) + "\n");
    }
  }
  rampCount = rampIndex;

  sendRemoteLogging(appendLong("song program parsing took: ", millis() - parseStartTime) + " ms\n");

  processProgramEvents();

  if (isTraceLogging) {
    trace(appendBuffer("song program buffer: ", idx, programArray));
  }
}

void processStartEvents() {
  if (processedStartEvents) {
    return;
  }
  
  for (int i=0; i<programEventCount; i++) {
    if (events[i].isStartEvent) {
      switch (events[i].eventType) {
        case MIDI_EVENT:
          sendRemoteLogging(appendInt("processing start packet: ", i) + appendInt(" of ", programEventCount) + appendLong(" color: ", events[i].color) + " " + packetString(events[i].packet) + "\n");
          processPacket(events[i].packet);
          setCurrentColor(events[i].color);
        break;
      }
    }
  }
  processedStartEvents = true;
}

void processStopEvents() {
  if (processedStopEvents) {
    return;
  }
  for (int i=0; i<stopEventsCount; i++) {
    sendRemoteLogging(appendInt("processing stop event: ", i) + appendInt(" of ", stopEventsCount) + " : " + packetString(stopEvents[i]) + "\n");
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
  
  if (programIndex >= programEventCount || status != RUNNING) {
    if (isDebugLogging) {
      debug(appendLong("programIndex: ", programIndex));
      debug(appendLong("programEventCount: ", programEventCount));
      debug("Bailing 1…");
    }
    return;
  }

  unsigned long now = millis();
  unsigned long elapsedTime = now - programStartTime;

  if (isDebugLogging) {
    debug(appendLong("minEventTime: ", minEventTime));
    debug(appendLong("elapsedTime: ", elapsedTime));
    debug(appendInt("programIndex: ", programIndex));
    debug(appendLong("delay: ", events[programIndex].delay));
  }

  // first skip all events prior to minEventTime
  for (; programIndex<programEventCount; programIndex++) {
    if (events[programIndex].delay >= minEventTime) {
      break;
    }
    sendRemoteLogging(appendLong("skipping event: ", programIndex) + "\n");
  }

  for (; programIndex<programEventCount; programIndex++) {
    if ((events[programIndex].delay - minEventTime) > elapsedTime) {
      debug("Bailing 3…");
      return;
    }
    switch (events[programIndex].eventType) {
      case MIDI_EVENT:
        sendRemoteLogging(appendInt("processing packet: ", programIndex) + appendInt(" of ", programEventCount) + appendLong(" color: ", events[programIndex].color) + " " + packetString(events[programIndex].packet) + "\n");
        processPacket(events[programIndex].packet);
        setCurrentColor(events[programIndex].color);
      break;
    }
  }
}

// Ramps

void resetRamps() {
  for (int i=0; i<rampCount; i++) { 
    ramps[i].currentValue = -1;
    ramps[i].reversed = false;
    ramps[i].cycleStart = ramps[i].start;
    ramps[i].ended = minEventTime > ramps[i].start;
    ramps[i].count = 0;
    debug(appendInt("ramp: ", i) + appendInt(" ended: ", ramps[i].ended) + "\n");
  }
}

void performRamp(int index, double progress, double linearProgress, double elapsed, bool force) {
  Ramp ramp = ramps[index];
  int8_t value = ramp.startValue + (int8_t)((ramp.endValue - ramp.startValue) * progress);
  if (ramp.shape == SQUARE_SHAPE) {
    if (progress < 0.5) {
      value = ramp.startValue;
    } else {
      value = ramp.endValue;
    }
  }
  
  if (isDebugLogging) {
    debug(appendUint8("ramp.startValue: ", ramp.startValue) + "\n");
    debug(appendUint8("ramp.endValue: ", ramp.endValue) + "\n");
    debug(appendUint8("ramp.currentValue: ", ramp.currentValue) + "\n");
    debug(appendDouble("progress: ", progress) + "\n");
    debug(appendUint8("value: ", value) + "\n");
  }

  if (value == ramp.currentValue) {
    return;
  }
  ramps[index].currentValue = value;
  ramps[index].count++;

  if (ramps[index].eventType == MIDI_EVENT) {
      unsigned long basePacket = ramp.source & 0xFFFFFF00;
      unsigned long packet = basePacket + (uint8_t)value;
      processPacket(packet);
  }
}

double convertProgressToRampShape(int index, double progress) {
  Ramp ramp = ramps[index];
  
  if (ramp.shape == SINE_SHAPE) {
    return 0.5 + (0.5 * sin((PI * progress) - PI_2));
  }
  if (ramp.shape == EXPONENTIAL_SHAPE) {
    return (pow(EXPONENTIAL_STEEPNESS, progress) - 1) / (EXPONENTIAL_STEEPNESS - 1);
  }
  return progress;
}

void processRampingEvents() {
  if (status != RUNNING) {
    return;
  }
  double now = (double)millis();

  for (int i=0; i<rampCount; i++) { 
    preProcessRamp(i, now);
  }
}

void preProcessRamp(int index, double now) {
  Ramp ramp = ramps[index];
  double elapsedTime = now - programStartTime;
  double timeInSong = elapsedTime + minEventTime;
  double overallProgress = (timeInSong - ramp.start) / ramp.duration;
  double shapeOverallProgress = convertProgressToRampShape(index, overallProgress);

  if (isDebugLogging) {
    debug(appendDouble("minEventTime: ", minEventTime) + appendDouble("ramp.start: ", ramp.start));
  }

  if (ramp.ended) {
    return;
  }
  
  if (overallProgress >= 1.0) {
    ramps[index].ended = true;
    sendRemoteLogging(appendInt("end of ramp: ", index) + appendDouble(" start ", ramp.start) + appendDouble(" end ", ramp.end) + appendDouble(" minEventTime: ", (double)minEventTime) + appendDouble(" overallProgress: ", overallProgress) + appendUint8(" ramp.currentValue: ", ramp.currentValue) + "\n");
    if (ramp.currentValue != ramp.endValue && ramp.duration == ramp.dutyCycle && ramp.shape != SQUARE_SHAPE) {
      performRamp(index, 1.0, overallProgress, elapsedTime, true);      
      return;
    } else if (overallProgress > 1.0) {
      return;
    }
  }

  double cycleProgress = (timeInSong - ramp.cycleStart) / ramp.dutyCycle;
  if (cycleProgress > 1.0) {
    ramps[index].reversed = !ramps[index].reversed;
    ramps[index].cycleStart += ramp.dutyCycle;
    preProcessRamp(index, now);
    return;
  }

  double truncatedProgress = min(max(cycleProgress, 0.0), 1.0);
  if (ramp.reversed) {
    truncatedProgress = 1.0 - truncatedProgress;
  }
  double shapeProgress = convertProgressToRampShape(index, truncatedProgress);
    
  if (timeInSong >= ramp.cycleStart) {
    performRamp(index, shapeProgress, truncatedProgress, elapsedTime, false);      
  }  
}
