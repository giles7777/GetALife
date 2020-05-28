#ifndef Sound_h
#define Sound_h

#include <Arduino.h>

#include <NonBlockingRtttl.h>

#include <Streaming.h>

#define PIN_BUZZER      D8

enum songList {
  // OS-level
  boot,
  
  // better not go there
  morningTrain,
  AxelF,
  RickRoll,
  CrazyTrain,
  
  // expressions; 3-5 notes that... express
  Joyful,
  Powerful,
  Peaceful,
  Sad,
  Mad,
  Scared,
  
  N_SOUNDS
};

class Sound {
  public:
    void begin();
    boolean update();

    void play(songList song, boolean blocking=false);

  private:

};

#endif
