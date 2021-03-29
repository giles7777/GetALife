#include "Motion.h"

void Motion::begin() {
  Serial << "Motion::begin()" << endl;

  pinMode(PIN_MOTION, INPUT_PULLUP);
}

String Motion::getStatus() {
  DynamicJsonDocument doc(2048);
  JsonObject m = doc.createNestedObject("motion");

  m["sensor"] = this->isMotion();

  String ret;
  serializeJson(doc, ret);
  return (ret);
}

void Motion::setStatus(String & msg) {
  DynamicJsonDocument doc(2048);
  if(doc.capacity() == 0) {
    Serial << "Motion::setStatus() out of memory." << endl;
    return;
  }
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial << "Motion::setStatus() error: " << err.c_str();
    Serial << " status: " << this->getStatus() << endl;
    return;
  } else {
    Serial << "Motion::setStatus() memory usage " << doc.memoryUsage() << endl;
  }

  // NOP
}

boolean Motion::isMotion() {
  return ( digitalRead(PIN_MOTION) );
}

boolean Motion::update() {
  static boolean lastState = isMotion();
  boolean newState = isMotion();
  if ( newState != lastState ) {
    lastState = newState;
    return ( true );
  }
  return (false);
}
