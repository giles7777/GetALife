#ifndef Sound_h
#define Sound_h

#include <Arduino.h>

#include <NonBlockingRtttl.h>

#include <Streaming.h>
#include <ArduinoJson.h>
#include <Metro.h>

// https://www.wemos.cc/en/latest/d1_mini_shiled/buzzer.html

#define PIN_BUZZER      D5

class Sound {
  public:
    void begin();
    void update();

    void setSong(String song);
    void setFrequency(uint32_t songsPerMinute=0); // set to zero for once
    void setCount(uint32_t songCount=1); // set to 1 for once
    
    String getStatus();
    void setStatus(String & msg);

  private:
    String song;
    uint32_t songsPerMinute, songCount;

};

#endif
