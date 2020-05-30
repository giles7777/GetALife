#ifndef Power_h
#define Power_h

#include <Arduino.h>

#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <EEPROM.h>
#include <Streaming.h>

#define PIN_WAKEUP    D0  // wire to RST
#define PIN_BATTERY   A0  // has 130k inline, so need a 67k tying A0 to GND

class Power {
  public:
    void begin();
    void update();
    
    void deepSleep(float minutes);
    uint16_t batteryVoltage(); // 0-5000 mV

    // save and load stuff from the filesystem
    void saveConfiguration(JsonDocument &doc, size_t address, size_t size);
    uint32_t loadConfiguration(JsonDocument &doc, size_t address, size_t size);

  private:
};

#endif
