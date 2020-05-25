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

float Power::batteryVoltage() {
  float raw = analogRead(A0); // 0-1023
  float voltage = 4.2*raw/1023.0;

  return( voltage );
}
