#include <SoftwareSerial.h>

// ----------------------------------------------------------------------
// Sends the static message "HEllo" via SMS every 5 seconds.
// No input, no waiting for a reply from the module -- fire and forget.
// SIM800L on SoftwareSerial: pin 7 = RX (wire to SIM800L TX),
// pin 8 = TX (wire to SIM800L RX, through a voltage divider/level
// shifter -- SIM800L's RX is 3.3V logic).
//
// POWER: SIM800L needs 3.7-4.2V and can spike to ~2A while
// transmitting. Use a dedicated regulated supply, not the Arduino's
// 5V pin, with a shared ground.
// ----------------------------------------------------------------------

#define SIM800_RX 7   // Arduino pin 7 <- SIM800L TX
#define SIM800_TX 8   // Arduino pin 8 -> SIM800L RX (through level shifter)
#define NOTIFY_NUMBER "+639317093056"

SoftwareSerial sim800(SIM800_RX, SIM800_TX);

void setup() {
  Serial.begin(9600);
  sim800.begin(9600);
  delay(1000);

  sim800.println("AT+CMGF=1"); // text mode
  delay(500);
}

void loop() {
  sim800.print("AT+CMGS=\"");
  sim800.print(NOTIFY_NUMBER);
  sim800.println("\"");
  delay(500);
  sim800.print("HEllo");
  delay(200);
  sim800.write(26); // Ctrl+Z sends the message
  delay(3000);

  Serial.println("Sent: HEllo");
  delay(5000);
}
