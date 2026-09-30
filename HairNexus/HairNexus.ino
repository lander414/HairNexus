// --- Input Pins ---
const int buttonPin = 7;
const int irPin1 = 2;   // Turns OFF Relays 3 and 5
const int irPin2 = 4;   // Turns OFF Relay 3 and starts Relay 4

// --- Relay Output Pins ---
const int relay1 = 8;
const int relay2 = 9;
const int relay3 = 10;
const int relay4 = 11;
const int relay5 = 13;

// --- Logic Definitions ---
const int RELAY_ON = HIGH;
const int RELAY_OFF = LOW;

const int BUTTON_PRESSED = LOW;
const int OBJECT_DETECTED = LOW;

// --- Timer Durations ---
const unsigned long relay12Duration = 5000;  // 5 seconds
const unsigned long relay4Duration = 10000; // 10 seconds

// --- Previous Input States ---
bool lastButtonState = HIGH;
bool lastIrPin1State = HIGH;
bool lastIrPin2State = HIGH;

// --- System States ---
enum SystemState {
  WAITING_FOR_BUTTON,
  RELAYS_1_2_RUNNING,
  RELAY_3_RUNNING,
  RELAY_4_RUNNING,
  RELAYS_3_5_RUNNING
};

SystemState systemState = WAITING_FOR_BUTTON;

// --- Timers ---
unsigned long relay12StartTime = 0;
unsigned long relay4StartTime = 0;

void setup() {
  Serial.begin(9600);

  // Configure input pins
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(irPin1, INPUT);
  pinMode(irPin2, INPUT);

  // Configure relay output pins
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  pinMode(relay5, OUTPUT);

  // Turn all relays OFF at startup
  digitalWrite(relay1, RELAY_OFF);
  digitalWrite(relay2, RELAY_OFF);
  digitalWrite(relay3, RELAY_OFF);
  digitalWrite(relay4, RELAY_OFF);
  digitalWrite(relay5, RELAY_OFF);

  Serial.println("System Ready. Waiting for button press.");
}

void loop() {
  unsigned long currentMillis = millis();

  bool buttonState = digitalRead(buttonPin);
  bool irPin1State = digitalRead(irPin1);
  bool irPin2State = digitalRead(irPin2);

  // Detect a new button press
  bool newButtonPress =
    (lastButtonState == HIGH &&
     buttonState == BUTTON_PRESSED);

  // Detect a new obstacle on IR pin 4
  bool newObstacleOnPin4 =
    (lastIrPin2State == HIGH &&
     irPin2State == OBJECT_DETECTED);

  // Detect a new obstacle on IR pin 2
  bool newObstacleOnPin2 =
    (lastIrPin1State == HIGH &&
     irPin1State == OBJECT_DETECTED);

  // ==========================================
  // WAITING FOR BUTTON
  // ==========================================

  if (systemState == WAITING_FOR_BUTTON) {
    if (newButtonPress) {
      digitalWrite(relay1, RELAY_ON);
      digitalWrite(relay2, RELAY_ON);

      relay12StartTime = currentMillis;
      systemState = RELAYS_1_2_RUNNING;

      Serial.println("Button pressed -> Relay 1 and Relay 2 ON");
    }
  }

  // ==========================================
  // RELAYS 1 AND 2 RUN FOR 5 SECONDS
  // ==========================================

  else if (systemState == RELAYS_1_2_RUNNING) {
    if (currentMillis - relay12StartTime >= relay12Duration) {
      digitalWrite(relay1, RELAY_OFF);
      digitalWrite(relay2, RELAY_OFF);

      digitalWrite(relay3, RELAY_ON);
      systemState = RELAY_3_RUNNING;

      Serial.println("5 seconds finished -> Relay 1 and Relay 2 OFF");
      Serial.println("Relay 3 ON");
    }
  }

  // ==========================================
  // RELAY 3 RUNS UNTIL IR PIN 4 DETECTS
  // ==========================================

  else if (systemState == RELAY_3_RUNNING) {
    if (newObstacleOnPin4) {
      digitalWrite(relay3, RELAY_OFF);
      digitalWrite(relay4, RELAY_ON);

      relay4StartTime = currentMillis;
      systemState = RELAY_4_RUNNING;

      Serial.println("IR pin 4 detected obstacle -> Relay 3 OFF");
      Serial.println("Relay 4 ON");
    }
  }

  // ==========================================
  // RELAY 4 RUNS FOR 10 SECONDS
  // ==========================================

  else if (systemState == RELAY_4_RUNNING) {
    if (currentMillis - relay4StartTime >= relay4Duration) {
      digitalWrite(relay4, RELAY_OFF);

      // After Relay 4 turns OFF,
      // Relays 3 and 5 turn ON
      digitalWrite(relay3, RELAY_ON);
      digitalWrite(relay5, RELAY_ON);

      systemState = RELAYS_3_5_RUNNING;

      Serial.println("10 seconds finished -> Relay 4 OFF");
      Serial.println("Relay 3 and Relay 5 ON");
    }
  }

  // ==========================================
  // RELAYS 3 AND 5 RUN UNTIL IR PIN 2 DETECTS
  // ==========================================

  else if (systemState == RELAYS_3_5_RUNNING) {
    if (newObstacleOnPin2) {
      digitalWrite(relay3, RELAY_OFF);
      digitalWrite(relay5, RELAY_OFF);

      systemState = WAITING_FOR_BUTTON;

      Serial.println("IR pin 2 detected obstacle");
      Serial.println("Relay 3 and Relay 5 OFF");
      Serial.println("System ready for the next button press");
    }
  }

  // Save previous input states
  lastButtonState = buttonState;
  lastIrPin1State = irPin1State;
  lastIrPin2State = irPin2State;

  // Basic debounce delay
  delay(20);
}
