#include <Metro.h>
#include <Streaming.h>

// Number of touch elements
#define N_TOUCH 4

byte touchPin[N_TOUCH] = {T0, T2, T3, T5};

// ISR's.  Don't mess with these.
volatile uint32_t lastPressed[N_TOUCH];
void ISR0_Pressed() { lastPressed[0]=millis(); }
void ISR1_Pressed() { lastPressed[1]=millis(); }
void ISR2_Pressed() { lastPressed[2]=millis(); }
void ISR3_Pressed() { lastPressed[3]=millis(); }

#define LED_ON LOW
#define LED_OFF HIGH

void setup()
{
  Serial.begin(115200);
  delay(1000); // give me time to bring up serial mLED_ONitor
  Serial.println("ESP32 Touch Test");
  pinMode(BUILTIN_LED, OUTPUT);

  digitalWrite(BUILTIN_LED, LED_ON);
}

uint16_t touchReadCensor(uint16_t lastVal, uint8_t pin) {
  uint32_t r = touchRead(pin);
  // getting spurious zeros; dropping
  if( r < 1 ) r = lastVal;

  // exponential smoother
  const uint32_t weight = 10;
  return (lastVal*(weight-1)+r)/weight;
}

boolean calibratedTouch(uint32_t foldThrehold) {
  static boolean calibrated=false;
  if( calibrated ) return( calibrated );
  
  static uint32_t touch[N_TOUCH]={100};

  // lots of averaging
  for( byte i=0; i<N_TOUCH; i++ ) 
    touch[i] = touchReadCensor(touch[i], touchPin[i]);

  static Metro calibrateTime(5000UL);
  if( calibrateTime.check() ) {
    Serial << "Calibrated Touch.\t";
    Serial << "T0:" << touch[0] << "\t";
    Serial << "T2:" << touch[1] << "\t";
    Serial << "T3:" << touch[2] << "\t";
    Serial << "T5:" << touch[3] << "\t";
    Serial << endl;

    touchAttachInterrupt(touchPin[0], ISR0_Pressed, touch[0]/foldThrehold);
    touchAttachInterrupt(touchPin[1], ISR1_Pressed, touch[1]/foldThrehold);
    touchAttachInterrupt(touchPin[2], ISR2_Pressed, touch[2]/foldThrehold);
    touchAttachInterrupt(touchPin[3], ISR3_Pressed, touch[3]/foldThrehold);

    calibrated = true;
    digitalWrite(BUILTIN_LED, LED_OFF);
  }

  return( calibrated );
}

// ISRs seem to fire about every 56 ms when the button is held.
boolean isPressed(byte n) {
  const uint32_t changeTime = 100; 
  static boolean state[N_TOUCH]={false};

  // has there been a new press event in the last changeTime milliseconds?
  boolean newPress = (millis()-lastPressed[n]) < changeTime;

  // if yes, we turn on the button; if not, turn it off.
  if( newPress && state[n]==false ) state[n]=true;
  if( (!newPress) && state[n]==true ) state[n]=false;

  boolean led = state[0] || state[1] || state[2] || state[3];
  digitalWrite(BUILTIN_LED, led ? LED_ON : LED_OFF);
  
  return(state[n]);
}

void loop() {
  while( ! calibratedTouch(2) );

  Serial << "T0:" << isPressed(0) << "\t";
  Serial << "T2:" << isPressed(1) << "\t";
  Serial << "T3:" << isPressed(2) << "\t";
  Serial << "T5:" << isPressed(3) << "\t";
  Serial << endl;
}
