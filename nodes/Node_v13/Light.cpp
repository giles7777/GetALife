#include "Light.h"

void Light::begin() {
  Serial << "Light::begin()" << endl;

  FastLED.addLeds<LED_TYPE, PIN_RGB, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  this->setBrightness();
  this->setFrameRate();
  this->setColorIncrement();
  this->setBlending();
  // this->setPalette();
  this->palette = CloudColors_p ; // http://fastled.io/docs/3.1/colorpalettes_8cpp_source.html
}

void Light::setBrightness(uint8_t bright) {
  if ( bright > 0 ) {
    this->brightness = bright;
    FastLED.setBrightness(this->brightness);
  } else {
    FastLED.clear();
  }
  Serial << "Light::setBrightness " << this->brightness << endl;
}
void Light::setFrameRate(uint32_t fr) {
  this->frameRate = fr;
  Serial << "Light::setFrameRate " << this->frameRate << endl;
}
void Light::setColorIncrement(uint8_t inc) {
  this->colorIncrement = inc;
  Serial << "Light::setColorIncrement " << this->colorIncrement << endl;
}
void Light::setBlending(TBlendType bl) {
  this->blending = bl;
  Serial << "Light::setBlending " << this->blending << endl;
}
void Light::setPalette(CRGBPalette16 pal) {
  this->palette = pal;
  Serial << "Light::setPalette " << endl;
}

void Light::update() {
  EVERY_N_MILLISECONDS( this->frameRate ) {
    static uint8_t startIndex = 0;
    startIndex += this->colorIncrement; /* motion speed */

    CRGB color = ColorFromPalette( this->palette, startIndex, this->brightness, this->blending );
    fill_solid(leds, NUM_LEDS, color);

    FastLED.show();
  }
}
