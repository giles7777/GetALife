#include "Light.h"

void Light::begin() {
  Serial << "Light::begin()" << endl;

  FastLED.addLeds<LED_TYPE, PIN_RGB, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  this->setBrightness(128);

  this->setFrameRate(50);
  this->setColorIncrement(1);
  this->setBlending(LINEARBLEND);
  this->setPalette(ForestColors_p); // http://fastled.io/docs/3.1/colorpalettes_8cpp_source.html

  this->setSparkleRate(50);
}

String Light::getStatus() {
  DynamicJsonDocument doc(2048);
  JsonObject l = doc.createNestedObject("light");

  l["bright"] = this->brightness;
  l["interval"] = this->frameRate;
  l["increment"] = this->frameRate;
  l["blend"] = this->blending;

  for ( byte i = 0; i < 16; i++ ) {
    l["palette"][i][0] = this->palette.entries[i].red;
    l["palette"][i][1] = this->palette.entries[i].green;
    l["palette"][i][2] = this->palette.entries[i].blue;
  }

  String ret;
  serializeJson(doc, ret);
  return (ret);
}

void Light::setStatus(String & msg) {
  DynamicJsonDocument doc(2048);
  if(doc.capacity() == 0) {
    Serial << "Light::setStatus() out of memory." << endl;
    return;
  }
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial << "Light::setStatus() error: " << err.c_str();
    Serial << " status: " << this->getStatus() << endl;
    return;
  } else {
    Serial << "Light::setStatus() memory usage " << doc.memoryUsage() << endl;
  }
  JsonObject l = doc["light"];

  if ( ! l["bright"].isNull() ) this->setBrightness( l["bright"].as<uint8_t>() );
  if ( ! l["interval"].isNull() ) this->setFrameRate( l["interval"].as<uint32_t>() );
  if ( ! l["increment"].isNull() ) this->setColorIncrement( l["increment"].as<uint8_t>() );
  if ( ! l["blend"].isNull() ) this->setBlending( l["blend"].as<TBlendType>() );
  if ( ! l["palette"].isNull() ) {
    CRGBPalette16 palette;
    for ( byte i = 0; i < 16; i++ ) {
      palette.entries[i].red = l["palette"][i][0].as<uint8_t>();
      palette.entries[i].green = l["palette"][i][1].as<uint8_t>();
      palette.entries[i].blue = l["palette"][i][2].as<uint8_t>();
    }
    this->setPalette( palette );
  }
  if ( ! l["sparkle"].isNull() ) this->setSparkleRate( l["sparkle"].as<uint8_t>() );
}

void Light::setBrightness(uint8_t bright) {
  if ( bright > 0 ) {
    this->brightness = bright;
    FastLED.setBrightness(this->brightness);
  } else {
    FastLED.clear();
  }
  Serial << "Light::setBrightness() " << this->brightness << endl;
}
void Light::setFrameRate(uint32_t fr) {
  this->frameRate = fr;
  Serial << "Light::setFrameRate() " << this->frameRate << endl;
}
void Light::setColorIncrement(uint8_t inc) {
  this->colorIncrement = inc;
  Serial << "Light::setColorIncrement() " << this->colorIncrement << endl;
}
void Light::setBlending(TBlendType bl) {
  this->blending = bl;
  Serial << "Light::setBlending() " << this->blending << endl;
}
void Light::setPalette(CRGBPalette16 pal) {
  this->palette = pal;
  Serial << "Light::setPalette() " << endl;
}
void Light::setSparkleRate(uint16_t sparkleRate) {
  this->sparkleRate = sparkleRate;
  Serial << "Light::sparkleRate() " <<  this->sparkleRate << endl;

}

void Light::paletteUpdate() {
  // are we doing this?
  if ( this->frameRate == 0 ) return;

  EVERY_N_MILLISECONDS( this->frameRate ) {

    // what was done last round
    static uint8_t index = 0;
    static CRGB lastColor = CRGB::Black;

    // what is done this round
    index += this->colorIncrement;
    CRGB newColor = ColorFromPalette( this->palette, index, this->brightness, this->blending );

    // adjust the array
    for (byte i = 0; i < NUM_LEDS; i++) {
      leds[i] -= lastColor; // undo
      leds[i] += newColor;  // do
    }

    // track our changes
    lastColor = newColor;
    FastLED.show();
  }
}

void Light::sparkleUpdate() {
  // are we doing this?
  if ( this->sparkleRate == 0 ) return;

  // track our sparkliness; critical we do so.
  static boolean isSparkling = false;

  // only sparkle briefly.
  EVERY_N_MILLISECONDS( 25 ) {
    if ( isSparkling ) {
      for (byte i = 0; i < NUM_LEDS; i++) leds[i] -= CRGB::White;
      isSparkling = false;
    }
  }

  // decide if we want to sparkle again
  EVERY_N_MILLISECONDS( this->frameRate ) {
    // every frameRate, we have a 1/sparkleRate probability of sparkling.
    if ( random16(this->sparkleRate) == 0 ) {
      for (byte i = 0; i < NUM_LEDS; i++) leds[i] += CRGB::White;
      isSparkling = true;
    }
  }
}

void Light::update() {
  // run the animations; note that they are _concurrent_, so the animation need to use "light math" to not clobber each other
  this->paletteUpdate();
  this->sparkleUpdate();
}
