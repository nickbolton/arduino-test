

const int RED_PIN =  3;
const int GREEN_PIN =  4;
const int BLUE_PIN =  5;

const int LED_ON = LOW;
const int LED_OFF = HIGH;

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

    if (_isConnected) {
      isLedOn = true;
      showGreenLED();
    } else {
      isLedOn = !isLedOn;
      if (isLedOn) {
        showYellowLED();
      } else {
        clearLEDs();
      }
    }
  }
}

void showYellowLED() {
  digitalWrite(RED_PIN, LED_ON);
  digitalWrite(GREEN_PIN, LED_ON);
  digitalWrite(BLUE_PIN, LED_OFF);    
}

void showGreenLED() {
  digitalWrite(RED_PIN, LED_OFF);
  digitalWrite(GREEN_PIN, LED_ON);
  digitalWrite(BLUE_PIN, LED_OFF);  
}

void showRedLED() {
  digitalWrite(RED_PIN, LED_ON);
  digitalWrite(GREEN_PIN, LED_OFF);
  digitalWrite(BLUE_PIN, LED_OFF);  
}

void clearLEDs() {
  digitalWrite(RED_PIN, LED_OFF);
  digitalWrite(GREEN_PIN, LED_OFF);
  digitalWrite(BLUE_PIN, LED_OFF);    
}
