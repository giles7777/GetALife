// ArduinoJson - arduinojson.org
// Copyright Benoit Blanchon 2014-2020
// MIT License
//

// https://arduinojson.org/v6/example/config/

#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <EEPROM.h>
#include <Streaming.h>

const size_t dataSize = 1024;
const size_t dataAddress = 0;
StaticJsonDocument<dataSize> dataDoc;

// check CRC32 for data integrity
uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}

// Loads the configuration from EEPROM
uint32_t loadConfiguration(JsonDocument &doc, size_t address, size_t size) {
  EEPROM.begin(size);
  EepromStream eepromStream(address, size);
  deserializeJson(doc, eepromStream);
  EEPROM.end();

  return( doc.size() );
}

// Saves the configuration to a file
void saveConfiguration(JsonDocument &doc, size_t address, size_t size) {  
  EEPROM.begin(size);
  EepromStream eepromStream(address, size);
  serializeJson(doc, eepromStream);
  eepromStream.flush();
  EEPROM.commit();
  EEPROM.end();
}

void setup() {
  // Initialize serial port
  Serial.begin(115200);
  while (!Serial) continue;
  Serial << endl << endl;

  // Should load default config if run for the first time
  Serial << "Loading stuff...";
  uint32_t objectCount = loadConfiguration(dataDoc, dataAddress, dataSize);
  Serial << " objects=" << objectCount << endl;
  
  // Dump config file
  Serial << "Contents... " << endl;
  serializeJsonPretty(dataDoc, Serial);
  Serial << endl;
  
  dataDoc["test"]=String("testing");
  int counter = dataDoc["counter"];
  dataDoc["counter"] = counter+1;
  dataDoc.remove("_CRC32");
  
  Serial << "Saving configuration..." << endl;
  saveConfiguration(dataDoc, dataAddress, dataSize);

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
