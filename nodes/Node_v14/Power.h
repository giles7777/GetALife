#ifndef Power_h
#define Power_h

#include <Arduino.h>

#include <EEPROM.h>
#include <StreamUtils.h>

#include <Streaming.h>
#include <ArduinoJson.h>

// https://www.wemos.cc/en/latest/d1/d1_mini_pro.html

#define PIN_WAKEUP    D0  
#define PIN_BATTERY   A0  

class Power {
  public:
    void begin();
    void update();
    
    void deepSleep(float minutes);
    uint16_t batteryVoltage(); // 0-5000 mV
    uint32_t getFreeHeap();
 
    // save and load stuff from the filesystem
    size_t saveConfiguration(String & msg);
    String loadConfiguration(size_t s);

    String getStatus();
    void setStatus(String & msg);
  private:
};

#endif
