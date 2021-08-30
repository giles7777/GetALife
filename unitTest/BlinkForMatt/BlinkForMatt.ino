
#include "Streaming.h"

uint32_t onTime = 10; // ms

#define ON LOW

void setup() {
  Serial.begin(115200);

  Serial << endl << endl << "Startup" << endl;
  
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, ON);   // Turn the LED on (Note that LOW is the voltage level
 
  delay(onTime);                      // Wait for a second

  digitalWrite(LED_BUILTIN, !ON);  // Turn the LED off by making the voltage HIGH

  delay(onTime);                      // Wait for two seconds (to demonstrate the active low LED)
}
