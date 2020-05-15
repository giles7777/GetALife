// ArduinoJson - arduinojson.org
// Copyright Benoit Blanchon 2014-2020
// MIT License
//
// The file contains a JSON document with the following content:
// {
//   "hostname": "examples.com",
//   "port": 2731++
// }
//
// https://arduinojson.org/v6/example/config/

#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <EEPROM.h>
#include <Streaming.h>

// Our configuration structure.
struct Config {
  char hostname[64];
  int port;
};

uint32_t Caddress, Esize;
Config config;                         // <- global configuration object

// Loads the configuration from EEPROM
void loadConfiguration(Config &config) {

 // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  EepromStream eepromStream(0, 256);
  deserializeJson(doc, eepromStream);

  int port = doc["port"];
  Serial << port << endl;

  config.port = doc["port"] | 2731;
  strlcpy(config.hostname,                  // <- destination
          doc["hostname"] | "example.com",  // <- source
          sizeof(config.hostname));         // <- destination's capacity

}

// Saves the configuration to a file
void saveConfiguration(const Config &config) {

 // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Set the values in the document
  doc["hostname"] = config.hostname;
  doc["port"] = config.port;

  EepromStream eepromStream(0, 256);
  serializeJson(doc, eepromStream);
  eepromStream.flush();  // (for ESP)
}

void setup() {
  // Initialize serial port
  Serial.begin(115200);
  while (!Serial) continue;

 if (!EEPROM.begin(1000)) 
    Serial.println("Failed to initialise EEPROM");
    
  // Should load default config if run for the first time
  Serial.println(F("Loading configuration..."));
  loadConfiguration(config);

  // Dump config file
  Serial.print(F("old config file... "));
  Serial << String(config.hostname) << ":" << config.port << endl;

  // adjust
  config.port++;

  // Create configuration file
  Serial.println(F("Saving configuration..."));
  saveConfiguration(config);

  // Dump config file
  Serial.print(F("Saved config file... "));
  Serial << String(config.hostname) << ":" << config.port << endl;
}

void loop() {
  // not used in this example
}

// See also
// --------
//
// https://arduinojson.org/ contains the documentation for all the functions
// used above. It also includes an FAQ that will help you solve any
// serialization or deserialization problem.
//
// The book "Mastering ArduinoJson" contains a case study of a project that has
// a complex configuration with nested members.
// Contrary to this example, the project in the book uses the SPIFFS filesystem.
// Learn more at https://arduinojson.org/book/
// Use the coupon code TWENTY for a 20% discount ❤❤❤❤❤
