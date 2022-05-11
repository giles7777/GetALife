#ifndef Motion_h
#define Motion_h

#include <Arduino.h>

#include <Streaming.h>
#include <Metro.h>

// https://www.wemos.cc/en/latest/d1_mini_shield/pir.html
#define PIN_PIR    D2

class Motion {
  public:
    void begin();
    bool update(); // call for update; returns true when motion detected/lost

    void setTriggerTimeout(unsigned long ms); // set length of time to stay triggered
    bool isMotion();
    
  private:
    Metro triggerTimeout;
    unsigned long triggerTimeoutMillis = 600000UL;
    bool motionDetected = false;
};


#endif
