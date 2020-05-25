#include "Sound.h"

// many more in the .zip files 
static const char boot[] = "boot:d=10,o=6,b=180,c,e,g";

void Sound::begin() {
  Serial << "Sound::begin()" << endl;

  // start
  pinMode(PIN_BUZZER, OUTPUT);
  rtttl::begin(PIN_BUZZER, boot);
}

boolean Sound::update() {
  boolean playing = !rtttl::done();
  if( playing ) rtttl::play();
  return( playing );
}
