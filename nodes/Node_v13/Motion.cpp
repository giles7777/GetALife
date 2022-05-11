#include "Motion.h"

void Motion::begin() {
  Serial << "Motion::begin()" << endl;

  pinMode(PIN_PIR, INPUT);
  this->triggerTimeout = Metro(this->triggerTimeoutMillis);
}

bool Motion::update() {
  bool newMotionDetectedState;
  if (digitalRead(PIN_PIR) == HIGH) {
    this->triggerTimeout.reset();
    newMotionDetectedState = true;
  } else if (this->triggerTimeout.check()) {
    newMotionDetectedState = false;
  }

  bool motionChanged = newMotionDetectedState != this->motionDetected;
  if (motionChanged) {
    this->motionDetected = newMotionDetectedState;
  }
  return motionChanged;
}

void Motion::setTriggerTimeout(unsigned long ms) {
  this->triggerTimeoutMillis = ms;
  this->triggerTimeout.interval(this->triggerTimeoutMillis);
  Serial << "Motion::setTriggerTimeout " << this->triggerTimeoutMillis << endl;
}

bool Motion::isMotion() {
  return this->motionDetected;
}
