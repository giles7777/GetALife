#ifndef Light_h
#define Light_h

#include <Arduino.h>
#include <FastLED.h>

#include <Streaming.h>

// size and shape
#define LED_TYPE            WS2812B
//#define COLOR_ORDER         RGB
#define COLOR_ORDER         GRB
#define NUM_LEDS            7

// https://www.wemos.cc/en/latest/d1_mini_shiled/rgb_led.html
#define PIN_RGB    D7  // NOT default (which is D4 aka BUILTIN_LED; doh!)
//#define PIN_RGB    D3  // Alan's set is different

class Light {
  public:
    void begin();
    void update(); // call for updates

    void setBrightness(byte bright); // [0-255]

    CRGBArray <NUM_LEDS> led; // can access directly, but be a better person.
    
  private:

    boolean isRunning = true; // track pattern update.

    void test_loop();
    void pacifica_loop();
    void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff);
    void pacifica_add_whitecaps();
    void pacifica_deepen_colors();
};


#endif
