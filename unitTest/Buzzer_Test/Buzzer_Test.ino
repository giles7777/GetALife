void tone(byte pin, float note);
void noTone(byte pin);
#include <Buzzer.h>
#include <Streaming.h>

// the number of the LED pin
const int ledPin = 25;  // pin
Buzzer buzzer(ledPin);

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 10;
const byte maxVol = resolution-1; // maximum volume at 50% duty
byte volume = maxVol;

void setup(){
  delay(300);
  Serial.begin(115200);
  Serial << endl << endl;
  
  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);

  // attach the channel to the GPIO to be controlled
  ledcAttachPin(ledPin, ledChannel);

}

void setVolume(byte volumeBit) {
  volume = volumeBit % (maxVol+1);
  Serial << volume << endl;
}

void tone(byte pin, float note) {
  ledcWriteTone(ledChannel, note);   // tone
  ledcWrite(ledChannel, 1<<volume);  // idk, but must set the duty cycle after the tone?
}

void noTone(byte pin) {
  ledcWriteTone(ledChannel, 0);
}

void loop(){
  buzzer.begin(0);

  setVolume(maxVol);
//  buzzer.distortion(NOTE_C4, NOTE_C5);
  buzzer.sound(NOTE_C4,1000);
  setVolume(maxVol/2);
//  buzzer.distortion(NOTE_C5, NOTE_C4);
  buzzer.sound(NOTE_C4,1000);

  while(1);
}
