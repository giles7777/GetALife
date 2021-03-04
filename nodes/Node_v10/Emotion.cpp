#include "Emotion.h"

void Emotion::begin() {
  Serial << "Emotion::begin()" << endl;

  setEmotion(MAD);
}

void Emotion::setEmotion(byte state) {
  this->emotion = state;

  switch(emotion) {
    case MAD:
      brightness = 255;
      colorStepTime = 10;
      currentPalette = LavaColors_p;
      break;
    case SAD:
      currentPalette = OceanColors_p;
      brightness = 60;
      colorStepTime = 100;
      break;
    case SCARED:
      colorStepTime = 10;
      brightness = 255;
      currentPalette = ForestColors_p;
      break;
    case JOYFUL:
      colorStepTime = 40;
      brightness = 255;
      currentPalette = PartyColors_p;
      break;
    case POWERFUL:
      colorStepTime = 40;
      brightness = 255;
      currentPalette = RainbowColors_p;
      break;
    case PEACEFUL:
      colorStepTime = 80;
      brightness = 200;
      currentPalette = CloudColors_p;
  }

  lastColorStep = millis();
}


char * Emotion::getEmotionLabel() {

  switch(emotion) {
    case MAD:
      return "Mad";
      break;
    case SAD:
      return "Sad";
      break;
    case SCARED:
      return "Scared";
      break;
    case JOYFUL:
      return "Joyful";
      break;
    case POWERFUL:
      return "Powerful";
      break;
    case PEACEFUL:
      return "Peaceful";
  }
}

void Emotion::animate() {
  static uint8_t colorIndex = 0;

  unsigned long currTime = millis();
  if (currTime - lastColorStep > colorStepTime) {
    colorIndex += 1;  
    lastColorStep = currTime;
  }
  
  CRGB color = ColorFromPalette(currentPalette, colorIndex, brightness, LINEARBLEND);
  for(byte i=0; i < NUM_LEDS; i++) {
    light->led[i] = color; 
  }
}

void Emotion::update() {  
  EVERY_N_MILLISECONDS( 10 ) {
    animate();
    /*
    switch(emotion) {
      case MAD:
        madLoop();
        break;
      case SAD:
        sadLoop();
        break;
      case SCARED:
        scaredLoop();
        break;
      case JOYFUL:
        joyfulLoop();
        break;
      case POWERFUL:
        powerfulLoop();
        break;
      case PEACEFUL:
        peacefulLoop();
        break;
    }
*/
    FastLED.show();
  }
}

void Emotion::madLoop() {

}

void Emotion::sadLoop() {

}

void Emotion::scaredLoop() {
}

void Emotion::joyfulLoop() {

}

void Emotion::powerfulLoop() {

}

void Emotion::peacefulLoop() {
}
