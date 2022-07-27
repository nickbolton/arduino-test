struct Ramp {
  unsigned long source;
  double start;
  double end;
  double duration;
  double dutyCycle;
  double cycleStart;
  uint8_t startValue;
  uint8_t endValue;
  uint8_t shape;
  uint8_t reversed;
  int8_t currentValue;
  bool ended;
  int count;
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
const uint8_t EXPONENTIAL_SHAPE = 3;
const double EXPONENTIAL_STEEPNESS = 15.0;
const double PI_2 = PI/2.0;

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
  unsigned long eventCount = parseULong(programArray, idx);
  idx += ULONG_SIZE;
  unsigned long rampCountIn = parseULong(programArray, idx);
  idx += ULONG_SIZE;

  switch (newStatus) {
    case UNLOADED:
      status = newStatus;
      resetProgram();
      startEventsCount = 0;
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
      startEventsCount = 0;
      stopEventsCount = 0;
      sendRemoteLogging(appendLong("unknown status: ", newStatus) + "\n");
      return;      
  }

  unsigned long parseStartTime = millis();  
  resetProgram();  

  startEventsCount = startCount;
  stopEventsCount = stopCount;
  programEventCount = eventCount;
  rampCount = rampCountIn;

  sendRemoteLogging(appendInt("startEventsCount: ", startEventsCount) + appendInt(" idx: ", idx) + "\n");

  unsigned long eventIndex = 0;
  while (eventIndex < startEventsCount && idx < songProgram.valueLength()) {
    startEvents[eventIndex] = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    eventIndex++;
  }

  sendRemoteLogging(appendInt("stopEventsCount: ", stopEventsCount) + appendInt(" idx: ", idx) + "\n");

  eventIndex = 0;
  while (eventIndex < stopEventsCount && idx < songProgram.valueLength()) {
    stopEvents[eventIndex] = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    eventIndex++;
  }

  sendRemoteLogging(appendInt("programEventCount: ", programEventCount) + appendInt(" idx: ", idx) + "\n");

  eventIndex = 0;
  while (eventIndex < programEventCount && idx < songProgram.valueLength()) {
    unsigned long packet = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    unsigned long delay = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    unsigned long color = parseULong(programArray, idx);
    idx += ULONG_SIZE;

    sendRemoteLogging(appendLong("packet: ", packet) + ", " + packetString(packet) + appendLong(", delay: ", delay) + "\n");
    programEvents[eventIndex] = packet;
    colors[eventIndex] = color;
    eventDelays[eventIndex] = delay;
    eventIndex++;
  }

  sendRemoteLogging(appendInt("rampCount: ", rampCount) + appendInt(" idx: ", idx) + appendInt(" songProgram.valueLength: ", songProgram.valueLength()) + "\n");

  int rampIndex = 0;
  while (rampIndex < rampCount && idx < songProgram.valueLength()) {
    unsigned long rampSource = parseULong(programArray, idx);
    idx += ULONG_SIZE;
    
    Ramp ramp;
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

//  sendRemoteLogging(appendBuffer("song program buffer: ", idx, programArray) + "\n");
}

void resetRamps() {
  for (int i=0; i<rampCount; i++) { 
    ramps[i].currentValue = -1;
    ramps[i].reversed = false;
    ramps[i].cycleStart = ramps[i].start;
    ramps[i].ended = minEventTime > ramps[i].start;
    ramps[i].count = 0;
    sendRemoteLogging(appendInt("ramp: ", i) + appendInt(" ended: ", ramps[i].ended) + "\n");
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
//  sendRemoteLogging(appendUint8("ramp.startValue: ", ramp.startValue) + "\n");
//  sendRemoteLogging(appendUint8("ramp.endValue: ", ramp.endValue) + "\n");
//  sendRemoteLogging(appendUint8("ramp.currentValue: ", ramp.currentValue) + "\n");
//  sendRemoteLogging(appendDouble("progress: ", progress) + "\n");
//  sendRemoteLogging(appendUint8("value: ", value) + "\n");
  if (value == ramp.currentValue) {
    return;
  }
  ramps[index].currentValue = value;
  ramps[index].count++;

//  if (ramps[index].count == 1 || force) {
  unsigned long basePacket =  ramp.source & 0xFFFFFF00;
  unsigned long packet = basePacket + (uint8_t)value;
  sendRemoteLogging(appendInt("ramping packet index: ", index) + appendInt(" count: ", ramps[index].count) + appendDouble(" elapsed: ", elapsed) + ", base packet: " + packetString(basePacket) + appendUint8(" value ", value) + " updated: " + packetString(packet) + appendDouble(" progress: ", progress) + appendDouble(" linearProgress: ", linearProgress) + "\n");
  sendRemoteLogging(appendDouble("shape metrics: ", linearProgress) + appendDouble(",", progress) + appendLong(",", value) + "\n");
  processPacket(packet);  
//  }
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
//  sendRemoteLogging(appendLong("status: ", status) + "\n");
  if (status != RUNNING) {
    return;
  }
  double now = (double)millis();

//  sendRemoteLogging(appendInt("rampCount: ", rampCount) + "\n");
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

//  sendRemoteLogging(appendDouble("minEventTime: ", minEventTime) + appendDouble("ramp.start: ", ramp.start) + "\n");

  if (ramp.ended) {
    return;
  }
  
  if (overallProgress >= 1.0) {
//    sendRemoteLogging(appendUint8("ramp.currentValue: ", ramp.currentValue) + appendUint8(" ramp.endValue: ", ramp.endValue) + appendDouble(" ramp.dutyCycle: ", ramp.dutyCycle) + appendDouble(" amp.duration: ", ramp.duration) + "\n");
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
    
//  sendRemoteLogging(appendInt("ramp: ", index) + appendDouble(" elapsedTime ", elapsedTime) + appendDouble(" cycleStart: ", ramp.cycleStart) + appendDouble(" dutyCycle: ", ramp.dutyCycle) + appendDouble(" cycleEnd: ", (ramp.cycleStart + ramp.dutyCycle)) + appendInt(" reversed: ", (int)ramp.reversed) + appendDouble(" cycleProgress: ", cycleProgress) + appendDouble(" truncatedProgress: ", truncatedProgress) + appendDouble(" shapeProgress: ", shapeProgress) + "\n");
  if (timeInSong >= ramp.cycleStart) {
//    sendRemoteLogging(appendDouble("ramping packet now: ", now) + appendDouble(" start ", ramp.start) + appendDouble(" end ", ramp.end) + appendDouble(" cycleProgress: ", cycleProgress) + "\n");
    performRamp(index, shapeProgress, truncatedProgress, elapsedTime, false);      
  }  
}

void processStartEvents() {
  if (processedStartEvents) {
    return;
  }
  sendRemoteLogging(appendInt("startEventsCount2: ", startEventsCount) + "\n");
  
  for (int i=0; i<startEventsCount; i++) {
    sendRemoteLogging(appendInt("processing start event: ", i) + appendInt(" of ", startEventsCount) + " : " + packetString(startEvents[i]) + "\n");
    processPacket(startEvents[i]);
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

//  sendRemoteLogging(appendLong("status: ", status) + appendInt(", programIndex: ", programIndex) + "\n");
  
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
    sendRemoteLogging(appendInt("processing packet: ", programIndex) + appendInt(" of ", programEventCount) + appendLong(" color: ", colors[programIndex]) + " " + packetString(programEvents[programIndex]) + "\n");
    processPacket(programEvents[programIndex]);
    setCurrentColor(colors[programIndex]);
  }
}
