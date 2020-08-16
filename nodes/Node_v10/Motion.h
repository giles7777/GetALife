#ifndef Motion_h
#define Motion_h

#include <Arduino.h>

#include <Streaming.h>

// https://www.wemos.cc/en/latest/d1_mini_shiled/pir.html

#define PIN_MOTION      D3

class Motion {
  public:
    void begin();
    boolean update();

    boolean isMotion();
  private:
};

#endif
