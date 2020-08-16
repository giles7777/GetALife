#ifndef Power_h
#define Power_h

#include <Arduino.h>

#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <EEPROM.h>
#include <Streaming.h>

// https://www.wemos.cc/en/latest/d1/d1_mini_pro.html

#define PIN_WAKEUP    D0  
#define PIN_BATTERY   A0  

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
