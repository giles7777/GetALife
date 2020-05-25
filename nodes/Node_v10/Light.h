#ifndef Light_h
#define Light_h

#include <Arduino.h>
#include <FastLED.h>

#include <Streaming.h>

// size and shape
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB
#define NUM_LEDS            8

// place and time
#define PIN_RGB_BACK    D2  // not default (jumper change)
#define PIN_RGB_FRONT   D7

class Light {
  public:
    void begin();
    void update(); // call for updates

    void setBrightness(byte bright); // [0-255]

    CRGBArray <NUM_LEDS> front, back; // can access directly, but be a better person.
    
  private:

    boolean isRunning = true; // track pattern update.

    void pacifica_loop();
    void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff);
    void pacifica_add_whitecaps();
    void pacifica_deepen_colors();
};


#endif
