#define sim800l Serial1

#include <Servo.h>

// ==================================================
// INPUT PINS
// ==================================================

const int buttonPin = 7;

const int irPin1 = 2;   // IR #2 - triggers servo
const int irPin2 = 4;   // IR #1 - Relay 3 -> Relay 4
const int irPin3 = 6;   // IR #3 - final trigger / SMS


// ==================================================
// RELAY OUTPUT PINS
// ==================================================

const int relay1 = 8;
const int relay2 = 9;
const int relay3 = 10;
const int relay4 = 11;
const int relay5 = 13;


// ==================================================
// SERVO
// ==================================================

const int servoPin = 12;

Servo myServo;


// ==================================================
// LOGIC
// ==================================================

const int RELAY_ON = HIGH;
const int RELAY_OFF = LOW;

const int BUTTON_PRESSED = LOW;
const int OBJECT_DETECTED = LOW;


// ==================================================
// TIMER DURATIONS
// ==================================================

const unsigned long relay12Duration = 5000;  // 5 sec
const unsigned long relay4Duration = 10000; // 10 sec


// ==================================================
// IR 6 PROTECTION
// ==================================================

// Time to ignore IR pin 6 after IR pin 2 triggers.
// This prevents the same obstacle/action from
// immediately triggering IR pin 6.
const unsigned long IR6_IGNORE_TIME = 1000;

unsigned long ir6IgnoreStartTime = 0;

bool ir6Armed = false;


// ==================================================
// PREVIOUS INPUT STATES
// ==================================================

bool lastButtonState = HIGH;

bool lastIrPin1State = HIGH;
bool lastIrPin2State = HIGH;
bool lastIrPin3State = HIGH;


// ==================================================
// SYSTEM STATES
// ==================================================

enum SystemState {

  WAITING_FOR_BUTTON,

  RELAYS_1_2_RUNNING,

  RELAY_3_RUNNING,

  RELAY_4_RUNNING,

  RELAYS_3_5_RUNNING,

  WAITING_FOR_IR6
};

SystemState systemState = WAITING_FOR_BUTTON;


// ==================================================
// TIMERS
// ==================================================

unsigned long relay12StartTime = 0;
unsigned long relay4StartTime = 0;


// ==================================================
// SIM800L SETTINGS
// ==================================================

const char PHONE_NUMBER[] = "+639935417612";

const char MESSAGE[] =
  "Hair Pouch is available for pick-up!";

const int MIN_SIGNAL = 10;

const int STABLE_COUNT_REQUIRED = 5;

const unsigned long CHECK_DELAY = 3000;

bool simReady = false;


// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(9600);


  // ------------------------------------------------
  // INPUT PINS
  // ------------------------------------------------

  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(irPin1, INPUT);
  pinMode(irPin2, INPUT);
  pinMode(irPin3, INPUT);


  // ------------------------------------------------
  // RELAY OUTPUT PINS
  // ------------------------------------------------

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  pinMode(relay5, OUTPUT);


  // ------------------------------------------------
  // ALL RELAYS OFF
  // ------------------------------------------------

  digitalWrite(relay1, RELAY_OFF);
  digitalWrite(relay2, RELAY_OFF);
  digitalWrite(relay3, RELAY_OFF);
  digitalWrite(relay4, RELAY_OFF);
  digitalWrite(relay5, RELAY_OFF);


  // ------------------------------------------------
  // SERVO
  // ------------------------------------------------

  myServo.attach(servoPin);

  myServo.write(0);

  delay(500);


  // ------------------------------------------------
  // READ INITIAL IR STATES
  // ------------------------------------------------

  lastIrPin1State = digitalRead(irPin1);
  lastIrPin2State = digitalRead(irPin2);
  lastIrPin3State = digitalRead(irPin3);

  lastButtonState = digitalRead(buttonPin);


  // ------------------------------------------------
  // SIM800L
  // ------------------------------------------------

  sim800l.begin(9600);

  Serial.println();
  Serial.println("================================");
  Serial.println("SIM800L COLD START");
  Serial.println("================================");

  delay(5000);


  if (!initializeModem()) {

    Serial.println(
      "SIM800L initialization failed. SMS alerts will be DISABLED."
    );

    simReady = false;
  }

  else if (!waitForStableNetwork()) {

    Serial.println(
      "Network failed to stabilize. SMS alerts will be DISABLED."
    );

    simReady = false;
  }

  else {

    Serial.println();
    Serial.println("================================");
    Serial.println("SIM800L CONNECTION STABLE");
    Serial.println("================================");

    simReady = true;
  }


  Serial.println();
  Serial.println(
    "System Ready. Waiting for button press."
  );
}


// ==================================================
// MAIN LOOP
// ==================================================

void loop() {

  unsigned long currentMillis = millis();


  // ==================================================
  // READ INPUTS
  // ==================================================

  bool buttonState = digitalRead(buttonPin);

  bool irPin1State = digitalRead(irPin1);

  bool irPin2State = digitalRead(irPin2);

  bool irPin3State = digitalRead(irPin3);


  // ==================================================
  // BUTTON DETECTION
  // ==================================================

  bool newButtonPress =
    (
      lastButtonState == HIGH &&
      buttonState == BUTTON_PRESSED
    );


  // ==================================================
  // IR PIN 4 DETECTION
  // ==================================================

  bool newObstacleOnPin4 =
    (
      lastIrPin2State == HIGH &&
      irPin2State == OBJECT_DETECTED
    );


  // ==================================================
  // IR PIN 2 DETECTION
  // ==================================================

  bool newObstacleOnPin2 =
    (
      lastIrPin1State == HIGH &&
      irPin1State == OBJECT_DETECTED
    );


  // ==================================================
  // WAITING FOR BUTTON
  // ==================================================

  if (systemState == WAITING_FOR_BUTTON) {

    if (newButtonPress) {

      digitalWrite(relay1, RELAY_ON);
      digitalWrite(relay2, RELAY_ON);

      relay12StartTime = currentMillis;

      systemState = RELAYS_1_2_RUNNING;

      Serial.println();
      Serial.println(
        "Button pressed -> Relay 1 and Relay 2 ON"
      );
    }
  }


  // ==================================================
  // RELAY 1 + 2 RUN FOR 5 SECONDS
  // ==================================================

  else if (systemState == RELAYS_1_2_RUNNING) {

    if (
      currentMillis - relay12StartTime
      >= relay12Duration
    ) {

      digitalWrite(relay1, RELAY_OFF);
      digitalWrite(relay2, RELAY_OFF);

      digitalWrite(relay3, RELAY_ON);

      systemState = RELAY_3_RUNNING;

      Serial.println(
        "5 seconds finished -> Relay 1 and Relay 2 OFF"
      );

      Serial.println(
        "Relay 3 ON"
      );
    }
  }


  // ==================================================
  // RELAY 3 RUNS UNTIL IR PIN 4
  // ==================================================

  else if (systemState == RELAY_3_RUNNING) {

    if (newObstacleOnPin4) {

      digitalWrite(relay3, RELAY_OFF);

      digitalWrite(relay4, RELAY_ON);

      relay4StartTime = currentMillis;

      systemState = RELAY_4_RUNNING;

      Serial.println(
        "IR pin 4 detected obstacle -> Relay 3 OFF"
      );

      Serial.println(
        "Relay 4 ON"
      );
    }
  }


  // ==================================================
  // RELAY 4 RUNS FOR 10 SECONDS
  // ==================================================

  else if (systemState == RELAY_4_RUNNING) {

    if (
      currentMillis - relay4StartTime
      >= relay4Duration
    ) {

      digitalWrite(relay4, RELAY_OFF);

      digitalWrite(relay3, RELAY_ON);
      digitalWrite(relay5, RELAY_ON);

      systemState = RELAYS_3_5_RUNNING;

      Serial.println(
        "10 seconds finished -> Relay 4 OFF"
      );

      Serial.println(
        "Relay 3 and Relay 5 ON"
      );
    }
  }


  // ==================================================
  // RELAY 3 + 5 RUN UNTIL IR PIN 2
  // ==================================================

  else if (systemState == RELAYS_3_5_RUNNING) {

    if (newObstacleOnPin2) {

      // ------------------------------------------------
      // IR PIN 2 DETECTED
      // ------------------------------------------------

      Serial.println();
      Serial.println(
        "IR pin 2 detected obstacle"
      );


      // ------------------------------------------------
      // RELAY 3 + 5 OFF
      // ------------------------------------------------

      digitalWrite(relay3, RELAY_OFF);
      digitalWrite(relay5, RELAY_OFF);

      Serial.println(
        "Relay 3 and Relay 5 OFF"
      );


      // ==================================================
      // SERVO
      // EXACTLY AS REQUESTED
      // ==================================================

      Serial.println(
        "Servo -> 90 degrees"
      );

      myServo.write(90);

      delay(500);

      Serial.println(
        "Servo -> 0 degrees"
      );

      myServo.write(0);


      // ==================================================
      // IR PIN 6 PROTECTION
      // ==================================================

      // IR pin 6 is now temporarily ignored.
      ir6Armed = false;

      ir6IgnoreStartTime = millis();

      systemState = WAITING_FOR_IR6;

      // IMPORTANT:
      // Do NOT treat the current IR6 state as a new
      // detection.
      lastIrPin3State = digitalRead(irPin3);


      Serial.println();
      Serial.println(
        "Servo sequence complete."
      );

      Serial.println(
        "IR pin 6 temporarily disabled."
      );

      Serial.println(
        "Waiting for IR pin 6 to become CLEAR..."
      );
    }
  }


  // ==================================================
  // WAITING FOR IR PIN 6
  // ==================================================

  else if (systemState == WAITING_FOR_IR6) {


    // ==================================================
    // STEP 1:
    // IGNORE IR 6 FOR A SHORT TIME
    // ==================================================

    if (!ir6Armed) {

      // Wait for the ignore period.
      if (
        currentMillis - ir6IgnoreStartTime
        >= IR6_IGNORE_TIME
      ) {

        // Read IR 6 again.
        bool ir6Now = digitalRead(irPin3);


        // ----------------------------------------------
        // IR 6 MUST BE CLEAR FIRST
        // ----------------------------------------------

        if (ir6Now == HIGH) {

          ir6Armed = true;

          // Start from HIGH.
          lastIrPin3State = HIGH;

          Serial.println();
          Serial.println(
            "IR pin 6 is CLEAR."
          );

          Serial.println(
            "IR pin 6 is now ARMED."
          );

          Serial.println(
            "Waiting for NEW IR pin 6 detection..."
          );
        }

        else {

          // Still LOW.
          // Do NOT arm it.
          Serial.println(
            "IR pin 6 still detecting obstacle..."
          );
        }
      }
    }


    // ==================================================
    // STEP 2:
    // IR 6 IS ARMED
    // ==================================================

    else {

      bool ir6Now = digitalRead(irPin3);


      // NEW LOW DETECTION
      if (
        lastIrPin3State == HIGH &&
        ir6Now == LOW
      ) {

        Serial.println();
        Serial.println(
          "IR pin 6 detected NEW obstacle!"
        );

        Serial.println(
          "Cycle complete."
        );


        // ==================================================
        // SEND SMS
        // ==================================================

        if (simReady) {

          sendHelloSMS();
        }

        else {

          Serial.println(
            "SMS skipped: SIM800L was not ready at startup."
          );
        }


        // ==================================================
        // RESET
        // ==================================================

        ir6Armed = false;

        systemState = WAITING_FOR_BUTTON;

        Serial.println();
        Serial.println(
          "System ready for the next button press."
        );
      }


      // Save IR 6 state
      lastIrPin3State = ir6Now;
    }
  }


  // ==================================================
  // SAVE INPUT STATES
  // ==================================================

  lastButtonState = buttonState;

  lastIrPin1State = irPin1State;

  lastIrPin2State = irPin2State;


  // Only update IR 6 normally when we are not
  // waiting to arm it.
  if (systemState != WAITING_FOR_IR6) {

    lastIrPin3State = irPin3State;
  }


  // Small debounce
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


    Serial.println(
      "Waiting for modem..."
    );

    delay(2000);
  }


  if (!modemReady) {

    return false;
  }


  sim800l.println("AT+CMEE=2");

  delay(1000);

  clearBuffer();


  Serial.println(
    "Checking SIM card..."
  );


  sim800l.println("AT+CPIN?");

  delay(1000);


  String simResponse =
    readResponse();


  Serial.println(
    simResponse
  );


  if (
    simResponse.indexOf("READY") < 0
  ) {

    Serial.println(
      "SIM card is not ready."
    );

    return false;
  }


  Serial.println(
    "Setting SMS text mode..."
  );


  sim800l.println("AT+CMGF=1");

  delay(1000);


  Serial.println(
    readResponse()
  );


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

  const int MAX_ATTEMPTS = 40;

  int attempts = 0;


  while (
    attempts < MAX_ATTEMPTS
  ) {

    Serial.println();
    Serial.println(
      "Checking network..."
    );


    int registration =
      getRegistration();

    int signal =
      getSignal();


    Serial.print(
      "Registration: "
    );

    Serial.println(
      registration
    );


    Serial.print(
      "Signal: "
    );

    Serial.println(
      signal
    );


    bool registered =
      (
        registration == 1 ||
        registration == 5
      );


    bool signalOK =
      (
        signal >= MIN_SIGNAL &&
        signal <= 31
      );


    if (
      registered &&
      signalOK
    ) {

      stableCount++;


      Serial.print(
        "Stable: "
      );

      Serial.print(
        stableCount
      );

      Serial.print(
        "/"
      );

      Serial.println(
        STABLE_COUNT_REQUIRED
      );


      if (
        stableCount >=
        STABLE_COUNT_REQUIRED
      ) {

        return true;
      }
    }

    else {

      stableCount = 0;

      Serial.println(
        "Network not stable yet."
      );
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

  sim800l.println(
    "AT+CREG?"
  );

  delay(1000);


  String response =
    readResponse();


  Serial.println(
    response
  );


  if (
    response.indexOf(
      "+CREG: 0,1"
    ) >= 0
  ) {

    return 1;
  }


  if (
    response.indexOf(
      "+CREG: 0,5"
    ) >= 0
  ) {

    return 5;
  }


  return 0;
}


// ==================================================
// GET SIGNAL
// ==================================================

int getSignal() {

  clearBuffer();

  sim800l.println(
    "AT+CSQ"
  );

  delay(1000);


  String response =
    readResponse();


  Serial.println(
    response
  );


  int position =
    response.indexOf(
      "+CSQ:"
    );


  if (position >= 0) {

    int comma =
      response.indexOf(
        ",",
        position
      );


    if (comma > position) {

      String value =
        response.substring(
          position + 6,
          comma
        );


      value.trim();


      return value.toInt();
    }
  }


  return -1;
}


// ==================================================
// SEND SMS
// ==================================================

void sendHelloSMS() {

  Serial.println();
  Serial.println("================================");
  Serial.println(
    "SENDING SMS (IR PIN 6 TRIGGER)"
  );
  Serial.println("================================");


  clearBuffer();


  Serial.print(
    "Sending to: "
  );

  Serial.println(
    PHONE_NUMBER
  );


  sim800l.print(
    "AT+CMGS=\""
  );

  sim800l.print(
    PHONE_NUMBER
  );

  sim800l.println(
    "\""
  );


  Serial.println(
    "Waiting for SMS prompt..."
  );


  unsigned long startTime =
    millis();


  bool promptReceived = false;


  while (
    millis() - startTime < 10000
  ) {

    if (
      sim800l.available()
    ) {

      char c =
        sim800l.read();


      Serial.write(c);


      if (c == '>') {

        promptReceived = true;

        break;
      }
    }
  }


  if (!promptReceived) {

    Serial.println();

    Serial.println(
      "SMS prompt not received."
    );

    return;
  }


  Serial.println();

  Serial.print(
    "Sending: "
  );

  Serial.println(
    MESSAGE
  );


  sim800l.print(
    MESSAGE
  );


  delay(1000);


  sim800l.write(
    (char)26
  );


  Serial.println(
    "Waiting for SMS result..."
  );


  startTime =
    millis();


  String response = "";


  while (
    millis() - startTime < 30000
  ) {

    while (
      sim800l.available()
    ) {

      char c =
        sim800l.read();


      Serial.write(c);

      response += c;
    }


    if (
      response.indexOf("+CMGS:") >= 0 &&
      response.indexOf("OK") >= 0
    ) {

      Serial.println();

      Serial.println(
        "================================"
      );

      Serial.println(
        "SMS SENT SUCCESSFULLY"
      );

      Serial.println(
        "================================"
      );

      return;
    }


    if (
      response.indexOf(
        "+CMS ERROR"
      ) >= 0
    ) {

      Serial.println();

      Serial.println(
        "================================"
      );

      Serial.println(
        "SMS ERROR"
      );

      Serial.println(
        "================================"
      );

      return;
    }
  }


  Serial.println();

  Serial.println(
    "SMS RESPONSE TIMEOUT."
  );
}


// ==================================================
// READ SIM800L RESPONSE
// ==================================================

String readResponse() {

  String response = "";

  unsigned long startTime =
    millis();


  while (
    millis() - startTime < 2000
  ) {

    while (
      sim800l.available()
    ) {

      char c =
        sim800l.read();


      response += c;
    }
  }


  return response;
}


// ==================================================
// CLEAR SERIAL BUFFER
// ==================================================

void clearBuffer() {

  while (
    sim800l.available()
  ) {

    sim800l.read();
  }
}
