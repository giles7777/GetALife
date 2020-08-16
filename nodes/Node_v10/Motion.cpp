#include "Motion.h"

void Motion::begin() {
  Serial << "Motion::begin()" << endl;

  pinMode(PIN_MOTION, INPUT_PULLUP);
}

boolean Motion::isMotion() {
  return( digitalRead(PIN_MOTION) );
}

boolean Motion::update() {
  static boolean lastState = isMotion();
  boolean newState = isMotion();
  if( newState != lastState ) {
    lastState = newState;
    return( true );
  }
  return(false);
}
