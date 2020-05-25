#ifndef Sound_h
#define Sound_h

#include <Arduino.h>

#include <NonBlockingRtttl.h>

#include <Streaming.h>

#define PIN_BUZZER      D8

class Sound {
  public:
    void begin();
    boolean update();
    
  private:

};

#endif
