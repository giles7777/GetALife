#include "Power.h"

void Power::begin() {
  Serial << "Power::begin()" << endl;

//  pinMode(PIN_WAKEUP, WAKEUP_PULLUP);
}

void Power::update() {
  // NOP
}

void Power::deepSleep(float minutes) {
  Serial << "Power::deepSleep minutes=" << minutes << endl;
  Serial.flush();
  delay(500);
  
  ESP.deepSleep(minutes*60e6); // minutes to microseconds
  delay(300);
}

uint16_t Power::batteryVoltage() {
  uint32_t raw = analogRead(A0); // 0-1023
  uint32_t voltage = (4200*raw)/1023;

  return( voltage );
}
