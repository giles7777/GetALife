#ifndef Power_h
#define Power_h

#include <Arduino.h>

#include <Streaming.h>

#define PIN_WAKEUP    D0  // wire to RST
#define PIN_BATTERY   A0  // has 130k inline, so need a 67k tying A0 to GND

class Power {
  public:
    void begin();
    void update();
    
    void deepSleep(float minutes);
    uint16_t batteryVoltage(); // 0-5000 mV
    
  private:
};

#endif
