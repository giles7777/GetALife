#ifndef Motion_h
#define Motion_h

#include <Arduino.h>

#include <Streaming.h>

#define PIN_MOTION      D6

class Motion {
  public:
    void begin();
    boolean update();

    boolean isMotion();
  private:
};

#endif
