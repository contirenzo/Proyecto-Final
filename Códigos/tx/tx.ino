#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <LoRa.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
 
// Define GPS serial communication pins
#define rxGPS 4
#define txGPS -1
#define gpsControlPin 5

// Define LoRa parameters
#define ss 10   // Pin for the CS (chip select)
#define rst 8    // Pin for the RESET
#define dio0 9   // Pin for the interrupt. Can be any digital pin.
 
TinyGPSPlus gps;    // GPS object
SoftwareSerial gpsSerial(rxGPS, txGPS); // Serial connection to the GPS
 
volatile int f_wdt = 1; // Watchdog timer flag
 
// Watchdog timer interrupt service routine
ISR(WDT_vect) {
  if (f_wdt == 0) {
    f_wdt = 1;
  } else {
    WDTCSR |= _BV(WDIE);
  }
}
 
void setup() {
  
  Serial.begin(9600); // Start serial communication at 9600 baud
  gpsSerial.begin(9600); // Start GPS serial communication at 9600 baud
  Serial.println("GPS initialized...");
 
  pinMode(gpsControlPin, OUTPUT);
  digitalWrite(gpsControlPin, LOW);
  
  // Initialize LoRa with specific pins and frequency
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(433E6)) { // Start LoRa at 433 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa initialized...");
 
  // Set sleep mode to power down mode for energy efficiency
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}
 
void loop() {
  Serial.println("Enciendo GPS");
  digitalWrite(gpsControlPin, HIGH);

  bool gpsDataAcquired = false;
  unsigned long startTime = millis();
  unsigned long timeout = 300000; // 1 minute timeout for acquiring GPS data

  while ((millis() - startTime < timeout) && !gpsDataAcquired) {
    if (gpsSerial.available()) {
      if (gps.encode(gpsSerial.read())) {
        if (gps.location.isValid()) {
          gpsDataAcquired = true;
          Serial.println("Acquired");
        }
      }
    }
  }

  if (gpsDataAcquired) {
    // Store GPS data in strings
    String lat = String(gps.location.lat(), 6);
    String lon = String(gps.location.lng(), 6);
    String speed = String(gps.speed.mps());

    // Print the acquired GPS data to the Serial Monitor
    Serial.println("GPS data acquired...");
    Serial.println("LAT: " + lat);
    Serial.println("LONG: " + lon);
    Serial.println("SPEED: " + speed);

    // Combine GPS data into one string to send
    String dataToSend = "LAT:" + lat + ",LONG:" + lon + ",SPEED:" + speed;

    // Begin LoRa transmission
    LoRa.beginPacket();
    LoRa.print(dataToSend); // Send the GPS data string
    int result = LoRa.endPacket(); // Finish LoRa transmission

    // Check if the LoRa transmission was successful and print result to Serial Monitor
    if (result) {
      Serial.println("Packet transmission successful");
    } else {
      Serial.println("Packet transmission failed");
    }
  } else {
    Serial.println("Failed to acquire GPS data");
  }
  

  // Go to sleep mode to save energy
  digitalWrite(gpsControlPin, LOW);
  Serial.println("Apagué GPS");
  Serial.println("Going to sleep now");
  Serial.println();
  delay(2000);
  // Bucle sleep
  for (int i = 0; i < 2; i++) { // 38 * 8 seconds = 304 seconds (5 minutes and 4 seconds)
    f_wdt = 0;
    setup_watchdog(9); // Set watchdog timer for approx 8 seconds
    while (f_wdt == 0) {
      system_sleep();
    }
  }

  // Disable watchdog timer
  wdt_disable();
  delay(2000);
}

// Function to put the system to sleep to save energy
void system_sleep() {
  ADCSRA &= ~(1 << ADEN); // Switch Analog to Digital converter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // Set sleep mode to power down mode
  sleep_enable(); // Enable sleep mode
  sleep_mode();  // System actually sleeps here
  sleep_disable(); // Disable sleep mode
  ADCSRA |= (1 << ADEN); // Switch Analog to Digital converter ON
}

// Function to set up watchdog timer
void setup_watchdog(int i) {
  byte bb;
  int ww;
  if (i > 9) i = 9; // Ensure that the interval does not exceed the maximum
  bb = i & 7;
  if (i > 7) bb |= (1 << 5); // Set the correct bit for longer time intervals
  bb |= (1 << WDCE); // Enable changes
  ww = bb;

  MCUSR &= ~(1 << WDRF); // Reset the reset flag
  WDTCSR |= (1 << WDCE) | (1 << WDE); // Start timed sequence
  WDTCSR = bb; // Set new watchdog timeout value
  WDTCSR |= _BV(WDIE); // Enable watchdog interrupt
}
