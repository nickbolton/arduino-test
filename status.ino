

const int RED_PIN =  3;
const int GREEN_PIN =  4;
const int BLUE_PIN =  5;

const int LED_ON = 0;
const int LED_OFF = 255;

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
  if (_isConnected) {
    isLedOn = true;
    if (currentColor > 0) {
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
  int red = 255 - (currentColor >> 16);
  int green = 255 - ((currentColor & 0xffff) >> 8);
  int blue = 255 - (currentColor & 0xff);

//  String redString = appendInt("ble showing current color red: ", red);
//  String greenString = appendInt(" green: ", green);
//  String blueString = appendInt(" blue: ", blue);
//  sendRemoteLogging(redString + greenString + blueString + "\n");
  
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}

void showYellowLED() {
  analogWrite(RED_PIN, LED_ON);
  analogWrite(GREEN_PIN, LED_ON);
  analogWrite(BLUE_PIN, LED_OFF);    
}

void showGreenLED() {
  analogWrite(RED_PIN, LED_OFF);
  analogWrite(GREEN_PIN, LED_ON);
  analogWrite(BLUE_PIN, LED_OFF);  
}

void showRedLED() {
  analogWrite(RED_PIN, LED_ON);
  analogWrite(GREEN_PIN, LED_OFF);
  analogWrite(BLUE_PIN, LED_OFF);  
}

void clearLEDs() {
  analogWrite(RED_PIN, LED_OFF);
  analogWrite(GREEN_PIN, LED_OFF);
  analogWrite(BLUE_PIN, LED_OFF);    
}
