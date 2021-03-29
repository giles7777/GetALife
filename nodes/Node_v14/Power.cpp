#include "Power.h"

void Power::begin() {
  Serial << "Power::begin()" << endl;

  //  pinMode(PIN_WAKEUP, WAKEUP_PULLUP);
}

String Power::getStatus() {
  DynamicJsonDocument doc(2048);
  JsonObject p = doc.createNestedObject("power");

  p["sleep"] = 0;
  p["battery"] = this->batteryVoltage();
  p["heap"]= this->getFreeHeap();
  
  String ret;
  serializeJson(doc, ret);
  return (ret);
}

void Power::setStatus(String & msg) {
  DynamicJsonDocument doc(2048);
  if(doc.capacity() == 0) {
    Serial << "Power::setStatus() out of memory." << endl;
    return;
  }
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial << "Power::setStatus() error: " << err.c_str();
    Serial << " status: " << this->getStatus() << endl;
    return;
  } else {
    Serial << "Power::setStatus() memory usage " << doc.memoryUsage() << endl;
  }
  JsonObject p = doc["power"];
  
  if ( ! p["sleep"].isNull() ) this->deepSleep( p["sleep"].as<float>() );
}

void Power::update() {
  // NOP
}

void Power::deepSleep(float minutes) {
  Serial << "Power::deepSleep() minutes=" << minutes << endl;
  Serial.flush();
  delay(500);

  if ( minutes == 0.0 ) return; // nope
/*
  Serial << "Power::deepSleep() saving configuration size: " << this->saveConfiguration() << endl;
  Serial.flush();
  delay(500);
*/
  Serial << "Power::deepSleep() sleeping now..." << endl;
  Serial.flush();
  delay(500);

  ESP.deepSleep(minutes * 60e6); // minutes to microseconds
  delay(300);
}

uint16_t Power::batteryVoltage() {
  uint32_t raw = analogRead(A0); // 0-1023
  uint32_t voltage = (4200 * raw) / 1023;

  return ( voltage );
}

uint32_t Power::getFreeHeap() {
  return( ESP.getFreeHeap() );
}

// Loads the configuration from EEPROM
String Power::loadConfiguration(size_t s) {
  Serial << "Power::loadConfiguration()" << endl;
/*
  EEPROM.begin(_J.capacity());
  EepromStream eepromStream(0, s);
  deserializeJson(_J, eepromStream);
  EEPROM.end();

  return ( _J.memoryUsage() );
*/
}

// Saves the configuration to a file
size_t Power::saveConfiguration(String & msg) {
  Serial << "Power::saveConfiguration()" << endl;
/*     
 _J.garbageCollect();

  EEPROM.begin(_J.capacity());
  EepromStream eepromStream(0, _J.capacity());
  serializeJson(_J, eepromStream);
  eepromStream.flush();
  EEPROM.commit();
  EEPROM.end();

  return ( _J.memoryUsage() );
*/
}
