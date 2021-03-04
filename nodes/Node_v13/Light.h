#ifndef Light_h
#define Light_h

#include <Arduino.h>
#include <FastLED.h>

#include <Streaming.h>

// size and shape
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB
#define NUM_LEDS            7

// https://www.wemos.cc/en/latest/d1_mini_shiled/rgb_led.html
#define PIN_RGB    D7  // NOT default (which is D4 aka BUILTIN_LED; doh!)

class Light {
  public:
    void begin();
    void update(); // call for updates

    void setBrightness(uint8_t brightness=255);
    void setFrameRate(uint32_t ms=100);
    void setColorIncrement(uint8_t inc=1);
    void setBlending(TBlendType bl=LINEARBLEND);
    void setPalette(CRGBPalette16 pal);
    
  private:
    CRGB leds[NUM_LEDS];

    uint8_t brightness, colorIncrement;
    uint32_t frameRate;
    TBlendType blending; 
    CRGBPalette16 palette;
};


#endif
