#ifndef Emotion_h
#define Emotion_h

#include <Arduino.h>
#include <FastLED.h>
#include <Streaming.h>

#include "Light.h"

#define MAD 0
#define SAD 1
#define SCARED 2
#define JOYFUL 3
#define POWERFUL 4
#define PEACEFUL 5

class Emotion {
  public:
    Emotion(Light * light) {
      this->light = light;  
    }
    void begin();
    void update(); // call for updates

    void setEmotion(byte emotion);
    char * getEmotionLabel();

    
  private:

    Light * light;
    uint8_t emotion = 0; 
    uint8_t colorStepTime;  // Time in ms for each step change
    unsigned long lastColorStep;
    CRGBPalette16 currentPalette;
    uint8_t brightness;

    void madLoop();
    void sadLoop();
    void scaredLoop();
    void joyfulLoop();
    void powerfulLoop();
    void peacefulLoop();

    void animate();

};


#endif
