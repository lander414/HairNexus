// ==================================================
// RELAY CONTROL SYSTEM + SIM800L SMS ALERT
// ARDUINO MEGA
//
// SIM800L wiring (hardware Serial1):
//   SIM800L TXD -> Mega pin 19 (RX1)
//   SIM800L RXD -> Mega pin 18 (TX1)  (via level shifter / voltage divider)
//   SIM800L GND -> Mega GND
//
// Behavior:
//   - On startup, the system waits until the SIM800L
//     modem is initialized AND the network signal/
//     registration is stable before printing "System Ready".
//   - The button/relay/IR logic works exactly as before
//     and is NEVER blocked by SMS sending.
//   - When IR pin 2 detects an obstacle (end of cycle,
//     from RELAYS_3_5_RUNNING state), an SMS is sent
//     IF the modem initialized successfully at startup.
// ==================================================

#define sim800l Serial1


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


// ==================================================
// SIM800L SETTINGS
// ==================================================
const char PHONE_NUMBER[] = "+639317093056";
const char MESSAGE[] = "IR Pin 2 triggered - cycle complete!";

const int MIN_SIGNAL = 10;
const int STABLE_COUNT_REQUIRED = 5;
const unsigned long CHECK_DELAY = 3000;

// Set to true only if modem init + network stability succeed at startup
bool simReady = false;


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


  // ------------------------------------------------
  // SIM800L: initialize modem and wait for stable
  // network BEFORE the system is considered ready.
  // This does NOT block button/relay logic once the
  // system is running -- it only runs once here.
  // ------------------------------------------------
  sim800l.begin(9600);

  Serial.println();
  Serial.println("================================");
  Serial.println("SIM800L COLD START");
  Serial.println("================================");

  delay(5000);

  if (!initializeModem()) {
    Serial.println("SIM800L initialization failed. SMS alerts will be DISABLED.");
    simReady = false;
  }
  else if (!waitForStableNetwork()) {
    Serial.println("Network failed to stabilize. SMS alerts will be DISABLED.");
    simReady = false;
  }
  else {
    Serial.println();
    Serial.println("================================");
    Serial.println("SIM800L CONNECTION STABLE");
    Serial.println("================================");
    simReady = true;
  }


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

      // ------------------------------------------------
      // Send SMS alert now that IR pin 2 has triggered.
      // Button/relay logic above is already done --
      // this does not block the next button press.
      // ------------------------------------------------
      if (simReady) {
        sendHelloSMS();
      }
      else {
        Serial.println("SMS skipped: SIM800L was not ready at startup.");
      }
    }
  }

  // Save previous input states
  lastButtonState = buttonState;
  lastIrPin1State = irPin1State;
  lastIrPin2State = irPin2State;

  // Basic debounce delay
  delay(20);
}


// ==================================================
// INITIALIZE MODEM
// ==================================================
bool initializeModem() {

  Serial.println();
  Serial.println("Initializing SIM800L...");

  bool modemReady = false;

  for (int i = 0; i < 10; i++) {

    clearBuffer();

    sim800l.println("AT");

    delay(1000);

    String response = readResponse();

    Serial.println(response);

    if (response.indexOf("OK") >= 0) {
      modemReady = true;
      break;
    }

    Serial.println("Waiting for modem...");

    delay(2000);
  }

  if (!modemReady) {
    return false;
  }

  sim800l.println("AT+CMEE=2");
  delay(1000);
  clearBuffer();

  Serial.println("Checking SIM card...");

  sim800l.println("AT+CPIN?");

  delay(1000);

  String simResponse = readResponse();

  Serial.println(simResponse);

  if (simResponse.indexOf("READY") < 0) {
    Serial.println("SIM card is not ready.");
    return false;
  }

  Serial.println("Setting SMS text mode...");

  sim800l.println("AT+CMGF=1");

  delay(1000);

  Serial.println(readResponse());

  return true;
}


// ==================================================
// WAIT FOR STABLE NETWORK
// ==================================================
bool waitForStableNetwork() {

  Serial.println();
  Serial.println("================================");
  Serial.println("WAITING FOR STABLE NETWORK");
  Serial.println("================================");

  int stableCount = 0;

  // Safety cap so a missing SIM/antenna doesn't hang setup() forever.
  const int MAX_ATTEMPTS = 40; // ~40 * 3s = ~2 minutes
  int attempts = 0;

  while (attempts < MAX_ATTEMPTS) {

    Serial.println();
    Serial.println("Checking network...");

    int registration = getRegistration();
    int signal = getSignal();

    Serial.print("Registration: ");
    Serial.println(registration);

    Serial.print("Signal: ");
    Serial.println(signal);

    bool registered =
      (registration == 1 || registration == 5);

    bool signalOK =
      (signal >= MIN_SIGNAL && signal <= 31);

    if (registered && signalOK) {
      stableCount++;

      Serial.print("Stable: ");
      Serial.print(stableCount);
      Serial.print("/");
      Serial.println(STABLE_COUNT_REQUIRED);

      if (stableCount >= STABLE_COUNT_REQUIRED) {
        return true;
      }
    }
    else {
      stableCount = 0;
      Serial.println("Network not stable yet.");
    }

    attempts++;
    delay(CHECK_DELAY);
  }

  return false;
}


// ==================================================
// GET NETWORK REGISTRATION
// ==================================================
int getRegistration() {

  clearBuffer();

  sim800l.println("AT+CREG?");

  delay(1000);

  String response = readResponse();

  Serial.println(response);

  if (response.indexOf("+CREG: 0,1") >= 0) {
    return 1;
  }

  if (response.indexOf("+CREG: 0,5") >= 0) {
    return 5;
  }

  return 0;
}


// ==================================================
// GET SIGNAL STRENGTH
// ==================================================
int getSignal() {

  clearBuffer();

  sim800l.println("AT+CSQ");

  delay(1000);

  String response = readResponse();

  Serial.println(response);

  int position = response.indexOf("+CSQ:");

  if (position >= 0) {
    int comma = response.indexOf(",", position);

    if (comma > position) {
      String value = response.substring(position + 6, comma);
      value.trim();
      return value.toInt();
    }
  }

  return -1;
}


// ==================================================
// SEND SMS (triggered by IR pin 2)
// ==================================================
void sendHelloSMS() {

  Serial.println();
  Serial.println("================================");
  Serial.println("SENDING SMS (IR PIN 2 TRIGGER)");
  Serial.println("================================");

  clearBuffer();

  Serial.print("Sending to: ");
  Serial.println(PHONE_NUMBER);

  sim800l.print("AT+CMGS=\"");
  sim800l.print(PHONE_NUMBER);
  sim800l.println("\"");

  Serial.println("Waiting for SMS prompt...");

  unsigned long startTime = millis();
  bool promptReceived = false;

  while (millis() - startTime < 10000) {
    if (sim800l.available()) {
      char c = sim800l.read();
      Serial.write(c);

      if (c == '>') {
        promptReceived = true;
        break;
      }
    }
  }

  if (!promptReceived) {
    Serial.println();
    Serial.println("SMS prompt not received.");
    return;
  }

  Serial.println();
  Serial.print("Sending: ");
  Serial.println(MESSAGE);

  sim800l.print(MESSAGE);

  delay(1000);

  sim800l.write((char)26);

  Serial.println("Waiting for SMS result...");

  startTime = millis();
  String response = "";

  while (millis() - startTime < 30000) {
    while (sim800l.available()) {
      char c = sim800l.read();
      Serial.write(c);
      response += c;
    }

    if (response.indexOf("+CMGS:") >= 0 &&
        response.indexOf("OK") >= 0) {
      Serial.println();
      Serial.println("================================");
      Serial.println("SMS SENT SUCCESSFULLY");
      Serial.println("================================");
      return;
    }

    if (response.indexOf("+CMS ERROR") >= 0) {
      Serial.println();
      Serial.println("================================");
      Serial.println("SMS ERROR");
      Serial.println("================================");
      return;
    }
  }

  Serial.println();
  Serial.println("SMS RESPONSE TIMEOUT.");
}


// ==================================================
// READ SIM800L RESPONSE
// ==================================================
String readResponse() {

  String response = "";
  unsigned long startTime = millis();

  while (millis() - startTime < 2000) {
    while (sim800l.available()) {
      char c = sim800l.read();
      response += c;
    }
  }

  return response;
}


// ==================================================
// CLEAR SERIAL BUFFER
// ==================================================
void clearBuffer() {
  while (sim800l.available()) {
    sim800l.read();
  }
}
