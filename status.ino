

const int RED_PIN =  3;
const int BLUE_PIN =  4;
const int GREEN_PIN =  5;

const int LED_ON = 127;
const int LED_OFF = 0;

unsigned long currentColor = 0;

unsigned long previousMillis = 0;        // will store last time LED was updated

const long interval = 1000;           // interval at which to blink (milliseconds)

bool _isConnected = false;
bool isLedOn = false;

void setupStatus() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  clearLEDs();
}

void setConnectedStatus() {
  _isConnected = true;
}

void setDisconnectedStatus() {
  _isConnected = false;
}

void processStatus() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    updateStatusColor();
  }
}

void updateStatusColor() {
//    sendRemoteLogging(appendInt("_isConnected: ", _isConnected) + appendInt("isLedOn: ", isLedOn) + appendLong("currentColor: ", currentColor) + "\n");

  if (_isConnected) {
    isLedOn = true;
    if (currentColor != 0) {
      showCurrentColor();
    } else {
      showGreenLED();
    }
  } else {
    isLedOn = !isLedOn;
    if (isLedOn) {
      showYellowLED();
    } else {
      clearLEDs();
    }
  }
}

void setCurrentColor(unsigned long color) {
  currentColor = color;
}

void showCurrentColor() {
  int rawRed = currentColor >> 16;
  int rawGreen = (currentColor & 0xffff) >> 8;
  int rawBlue = currentColor & 0xff;

  float redPercent = (float)rawRed / 255.0;
  float greenPercent = (float)rawGreen / 255.0;
  float bluePercent = (float)rawBlue / 255.0;

  int red = (int)(redPercent * 127.0);
  int green = (int)(greenPercent * 127.0);
  int blue = (int)(bluePercent * 127.0);

//  String redString = appendInt("ble showing current color red: ", red);
//  String greenString = appendInt(" green: ", green);
//  String blueString = appendInt(" blue: ", blue);
//  sendRemoteLogging(redString + greenString + blueString + "\n");
  
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}

void showYellowLED() {
//  sendRemoteLogging("Showing YELLOW COLOR\n");
  analogWrite(RED_PIN, LED_ON);
  analogWrite(GREEN_PIN, LED_ON);
  analogWrite(BLUE_PIN, LED_OFF);    
}

void showGreenLED() {
//  sendRemoteLogging("Showing GREEN COLOR\n");
  analogWrite(RED_PIN, LED_OFF);
  analogWrite(GREEN_PIN, LED_ON);
  analogWrite(BLUE_PIN, LED_OFF);  
}

void showRedLED() {
//  sendRemoteLogging("Showing RED COLOR\n");
  analogWrite(RED_PIN, LED_ON);
  analogWrite(GREEN_PIN, LED_OFF);
  analogWrite(BLUE_PIN, LED_OFF);  
}

void clearLEDs() {
//  sendRemoteLogging("Showing CLEAR COLOR\n");
  analogWrite(RED_PIN, LED_OFF);
  analogWrite(GREEN_PIN, LED_OFF);
  analogWrite(BLUE_PIN, LED_OFF);    
}
