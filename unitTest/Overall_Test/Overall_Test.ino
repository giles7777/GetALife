// EEPROM
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <EEPROM.h>
// buzzer
#include <NonBlockingRtttl.h>
// LED
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
FASTLED_USING_NAMESPACE
// general
#include <Streaming.h>
#include <Metro.h>

// touch elements
#define N_TOUCH 4

byte touchPin[N_TOUCH] = {T0, T2, T3, T5};

// ISR's.  Don't mess with these.
volatile uint32_t lastPressed[N_TOUCH];

#define LED_ON LOW
#define LED_OFF HIGH

// deep sleep
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR int bootCount = 0;

// EEPROM
struct Config {
  char hostname[64];
  int port;
};
Config config;                         // <- global configuration object

// LED
#define DATA_PIN            21
#define DATA_PIN2           22
#define NUM_LEDS            8*2
#define MAX_POWER_MILLIWATTS (uint32_t)(3.3*200.0) // W = V * A
#define LED_TYPE            WS2812B
#define COLOR_ORDER         GRB
CRGB leds[NUM_LEDS];

// buzzer
#define BUZZER_PIN 25

// many more in the .zip files with this unit test.
prog_char song_0[] PROGMEM = "GirlFromIpane:d=4,o=5,b=160:g.,8e,8e,d,g.,8e,e,8e,8d,g.,e,e,8d,g,8g,8e,e,8e,8d,f,d,d,8d,8c,e,c,c,8c,a#4,2c";
prog_char song_1[] PROGMEM = "The Simpsons:d=4,o=5,b=160:c.6,e6,f#6,8a6,g.6,e6,c6,8a,8f#,8f#,8f#,2g,8p,8p,8f#,8f#,8f#,8g,a#.,8c6,8c6,8c6,c6";
prog_char song_2[] PROGMEM = "Indiana:d=4,o=5,b=250:e,8p,8f,8g,8p,1c6,8p.,d,8p,8e,1f,p.,g,8p,8a,8b,8p,1f6,p,a,8p,8b,2c6,2d6,2e6,e,8p,8f,8g,8p,1c6,p,d6,8p,8e6,1f.6,g,8p,8g,e.6,8p,d6,8p,8g,e.6,8p,d6,8p,8g,f.6,8p,e6,8p,8d6,2c6";
prog_char song_3[] PROGMEM = "TakeOnMe:d=4,o=4,b=160:8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5,8f#5,8e5,8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5";
prog_char song_4[] PROGMEM = "Entertainer:d=4,o=5,b=140:8d,8d#,8e,c6,8e,c6,8e,2c.6,8c6,8d6,8d#6,8e6,8c6,8d6,e6,8b,d6,2c6,p,8d,8d#,8e,c6,8e,c6,8e,2c.6,8p,8a,8g,8f#,8a,8c6,e6,8d6,8c6,8a,2d6";
prog_char song_5[] PROGMEM = "Muppets:d=4,o=5,b=250:c6,c6,a,b,8a,b,g,p,c6,c6,a,8b,8a,8p,g.,p,e,e,g,f,8e,f,8c6,8c,8d,e,8e,8e,8p,8e,g,2p,c6,c6,a,b,8a,b,g,p,c6,c6,a,8b,a,g.,p,e,e,g,f,8e,f,8c6,8c,8d,e,8e,d,8d,c";
prog_char song_6[] PROGMEM = "Xfiles:d=4,o=5,b=125:e,b,a,b,d6,2b.,1p,e,b,a,b,e6,2b.,1p,g6,f#6,e6,d6,e6,2b.,1p,g6,f#6,e6,d6,f#6,2b.,1p,e,b,a,b,d6,2b.,1p,e,b,a,b,e6,2b.,1p,e6,2b.";
prog_char song_7[] PROGMEM = "Looney:d=4,o=5,b=140:32p,c6,8f6,8e6,8d6,8c6,a.,8c6,8f6,8e6,8d6,8d#6,e.6,8e6,8e6,8c6,8d6,8c6,8e6,8c6,8d6,8a,8c6,8g,8a#,8a,8f";
prog_char song_8[] PROGMEM = "20thCenFox:d=16,o=5,b=140:b,8p,b,b,2b,p,c6,32p,b,32p,c6,32p,b,32p,c6,32p,b,8p,b,b,b,32p,b,32p,b,32p,b,32p,b,32p,b,32p,b,32p,g#,32p,a,32p,b,8p,b,b,2b,4p,8e,8g#,8b,1c#6,8f#,8a,8c#6,1e6,8a,8c#6,8e6,1e6,8b,8g#,8a,2b";
prog_char song_9[] PROGMEM = "Bond:d=4,o=5,b=80:32p,16c#6,32d#6,32d#6,16d#6,8d#6,16c#6,16c#6,16c#6,16c#6,32e6,32e6,16e6,8e6,16d#6,16d#6,16d#6,16c#6,32d#6,32d#6,16d#6,8d#6,16c#6,16c#6,16c#6,16c#6,32e6,32e6,16e6,8e6,16d#6,16d6,16c#6,16c#7,c.7,16g#6,16f#6,g#.6";
prog_char song_10[] PROGMEM = "MASH:d=8,o=5,b=140:4a,4g,f#,g,p,f#,p,g,p,f#,p,2e.,p,f#,e,4f#,e,f#,p,e,p,4d.,p,f#,4e,d,e,p,d,p,e,p,d,p,2c#.,p,d,c#,4d,c#,d,p,e,p,4f#,p,a,p,4b,a,b,p,a,p,b,p,2a.,4p,a,b,a,4b,a,b,p,2a.,a,4f#,a,b,p,d6,p,4e.6,d6,b,p,a,p,2b";
prog_char song_11[] PROGMEM = "StarWars:d=4,o=5,b=45:32p,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#.6,32f#,32f#,32f#,8b.,8f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32c#6,8b.6,16f#.6,32e6,32d#6,32e6,8c#6";
prog_char song_12[] PROGMEM = "GoodBad:d=4,o=5,b=56:32p,32a#,32d#6,32a#,32d#6,8a#.,16f#.,16g#.,d#,32a#,32d#6,32a#,32d#6,8a#.,16f#.,16g#.,c#6,32a#,32d#6,32a#,32d#6,8a#.,16f#.,32f.,32d#.,c#,32a#,32d#6,32a#,32d#6,8a#.,16g#.,d#";
prog_char song_13[] PROGMEM = "TopGun:d=4,o=4,b=31:32p,16c#,16g#,16g#,32f#,32f,32f#,32f,16d#,16d#,32c#,32d#,16f,32d#,32f,16f#,32f,32c#,16f,d#,16c#,16g#,16g#,32f#,32f,32f#,32f,16d#,16d#,32c#,32d#,16f,32d#,32f,16f#,32f,32c#,g#";
prog_char song_14[] PROGMEM = "A-Team:d=8,o=5,b=125:4d#6,a#,2d#6,16p,g#,4a#,4d#.,p,16g,16a#,d#6,a#,f6,2d#6,16p,c#.6,16c6,16a#,g#.,2a#";
prog_char song_15[] PROGMEM = "Flinstones:d=4,o=5,b=40:32p,16f6,16a#,16a#6,32g6,16f6,16a#.,16f6,32d#6,32d6,32d6,32d#6,32f6,16a#,16c6,d6,16f6,16a#.,16a#6,32g6,16f6,16a#.,32f6,32f6,32d#6,32d6,32d6,32d#6,32f6,16a#,16c6,a#,16a6,16d.6,16a#6,32a6,32a6,32g6,32f#6,32a6,8g6,16g6,16c.6,32a6,32a6,32g6,32g6,32f6,32e6,32g6,8f6,16f6,16a#.,16a#6,32g6,16f6,16a#.,16f6,32d#6,32d6,32d6,32d#6,32f6,16a#,16c.6,32d6,32d#6,32f6,16a#,16c.6,32d6,32d#6,32f6,16a#6,16c7,8a#.6";
prog_char song_16[] PROGMEM = "Jeopardy:d=4,o=6,b=125:c,f,c,f5,c,f,2c,c,f,c,f,a.,8g,8f,8e,8d,8c#,c,f,c,f5,c,f,2c,f.,8d,c,a#5,a5,g5,f5,p,d#,g#,d#,g#5,d#,g#,2d#,d#,g#,d#,g#,c.7,8a#,8g#,8g,8f,8e,d#,g#,d#,g#5,d#,g#,2d#,g#.,8f,d#,c#,c,p,a#5,p,g#.5,d#,g#";
prog_char song_17[] PROGMEM = "Gadget:d=16,o=5,b=50:32d#,32f,32f#,32g#,a#,f#,a,f,g#,f#,32d#,32f,32f#,32g#,a#,d#6,4d6,32d#,32f,32f#,32g#,a#,f#,a,f,g#,f#,8d#";
prog_char song_18[] PROGMEM = "Smurfs:d=32,o=5,b=200:4c#6,16p,4f#6,p,16c#6,p,8d#6,p,8b,p,4g#,16p,4c#6,p,16a#,p,8f#,p,8a#,p,4g#,4p,g#,p,a#,p,b,p,c6,p,4c#6,16p,4f#6,p,16c#6,p,8d#6,p,8b,p,4g#,16p,4c#6,p,16a#,p,8b,p,8f,p,4f#";
prog_char song_19[] PROGMEM = "MahnaMahna:d=16,o=6,b=125:c#,c.,b5,8a#.5,8f.,4g#,a#,g.,4d#,8p,c#,c.,b5,8a#.5,8f.,g#.,8a#.,4g,8p,c#,c.,b5,8a#.5,8f.,4g#,f,g.,8d#.,f,g.,8d#.,f,8g,8d#.,f,8g,d#,8c,a#5,8d#.,8d#.,4d#,8d#.";
prog_char song_20[] PROGMEM = "LeisureSuit:d=16,o=6,b=56:f.5,f#.5,g.5,g#5,32a#5,f5,g#.5,a#.5,32f5,g#5,32a#5,g#5,8c#.,a#5,32c#,a5,a#.5,c#.,32a5,a#5,32c#,d#,8e,c#.,f.,f.,f.,f.,f,32e,d#,8d,a#.5,e,32f,e,32f,c#,d#.,c#";
prog_char song_21[] PROGMEM = "MissionImp:d=16,o=6,b=95:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,g,8p,g,8p,a#,p,c7,p,g,8p,g,8p,f,p,f#,p,a#,g,2d,32p,a#,g,2c#,32p,a#,g,2c,a#5,8c,2p,32p,a#5,g5,2f#,32p,a#5,g5,2f,32p,a#5,g5,2e,d#,8d";
prog_char song_22[] PROGMEM = "TakeOnMe:d=4,o=4,b=160:8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5,8f#5,8e5,8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p,8e5,8p,8e5,8g#5,8g#5,8a5,8b5,8a5,8a5,8a5,8e5,8p,8d5,8p,8f#5,8p,8f#5,8p,8f#5,8e5,8e5";
prog_char song_23[] PROGMEM = "90210:d=4,o=5,b=140:8f,8a#,8c6,d.6,2d6,p,8f,8a#,8c6,8d6,8d#6,f6,f.6,2a#.,8f,8a#,8c6,8d6,8d#6,8f6,8g6,f6,8d#6,d#6,d6,2c.6,8a#,a,a#.,g6,8f6,8d#6,8d6,8d#6,8d6,8a#,f";
prog_char song_24[] PROGMEM = "Abdelazer:d=4,o=5,b=160:2d,2f,2a,d6,8e6,8f6,8g6,8f6,8e6,8d6,2c#6,a6,8d6,8f6,8a6,8f6,d6,2a6,g6,8c6,8e6,8g6,8e6,c6,2a6,f6,8b,8d6,8f6,8d6,b,2g6,e6,8a,8c#6,8e6,8c6,a,2f6,8e6,8f6,8e6,8d6,c#6,f6,8e6,8f6,8e6,8d6,a,d6,8c#6,8d6,8e6,8d6,2d6";
prog_char song_25[] PROGMEM = "aadams:d=4,o=5,b=160:8c,f,8a,f,8c,b4,2g,8f,e,8g,e,8e4,a4,2f,8c,f,8a,f,8c,b4,2g,8f,e,8c,d,8e,1f,8c,8d,8e,8f,1p,8d,8e,8f#,8g,1p,8d,8e,8f#,8g,p,8d,8e,8f#,8g,p,8c,8d,8e,8f";
prog_char song_26[] PROGMEM = "Smoke:d=4,o=5,b=112:c,d#,f.,c,d#,8f#,f,p,c,d#,f.,d#,c,2p,8p,c,d#,f.,c,d#,8f#,f,p,c,d#,f.,d#,c,p";
prog_char song_27[] PROGMEM = "smb:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,16p,8c6,16p,8g,16p,8e,16p,8a,8b,16a#,8a,16g.,16e6,16g6,8a6,16f6,8g6,8e6,16c6,16d6,8b,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16c7,16p,16c7,16c7,p,16g6,16f#6,16f6,16d#6,16p,16e6,16p,16g#,16a,16c6,16p,16a,16c6,16d6,8p,16d#6,8p,16d6,8p,16c6";
prog_char song_28[] PROGMEM = "smb_under:d=4,o=6,b=100:32c,32p,32c7,32p,32a5,32p,32a,32p,32a#5,32p,32a#,2p,32c,32p,32c7,32p,32a5,32p,32a,32p,32a#5,32p,32a#,2p,32f5,32p,32f,32p,32d5,32p,32d,32p,32d#5,32p,32d#,2p,32f5,32p,32f,32p,32d5,32p,32d,32p,32d#5,32p,32d#";
prog_char song_29[] PROGMEM = "smbdeath:d=4,o=5,b=90:32c6,32c6,32c6,8p,16b,16f6,16p,16f6,16f.6,16e.6,16d6,16c6,16p,16e,16p,16c";
prog_char song_30[] PROGMEM = "ducktales:d=4,o=5,b=112:8e6,8e6,16p,16g6,8b6,g#6,p,8e6,8d6,8c6,8d6,8e6,8d6,8c6,8d6,8e6,8e6,16p,16g6,8b6,g#6,p,8e6,8d6,8c6,8d6,8e6,8d6,8c6,8g6,8e6,8e6";
prog_char song_31[] PROGMEM = "Zelda1:d=4,o=5,b=125:a#,f.,8a#,16a#,16c6,16d6,16d#6,2f6,8p,8f6,16f.6,16f#6,16g#.6,2a#.6,16a#.6,16g#6,16f#.6,8g#.6,16f#.6,2f6,f6,8d#6,16d#6,16f6,2f#6,8f6,8d#6,8c#6,16c#6,16d#6,2f6,8d#6,8c#6,8c6,16c6,16d6,2e6,g6,8f6,16f,16f,8f,16f,16f,8f,16f,16f,8f,8f,a#,f.,8a#,16a#,16c6,16d6,16d#6,2f6,8p,8f6,16f.6,16f#6,16g#.6,2a#.6,c#7,c7,2a6,f6,2f#.6,a#6,a6,2f6,f6,2f#.6,a#6,a6,2f6,d6,2d#.6,f#6,f6,2c#6,a#,c6,16d6,2e6,g6,8f6,16f,16f,8f,16f,16f,8f,16f,16f,8f,8f";
prog_char song_32[] PROGMEM = "smario2:d=4,o=5,b=125:8g,16c,8e,8g.,16c,8e,16g,16c,16e,16g,8b,a,8p,16c,8g,16c,8e,8g.,16c,8e,16g,16c#,16e,16g,8b,a,8p,16b,8c6,16b,8c6,8a.,16c6,8b,16a,8g,16f#,8g,8e.,16c,8d,16e,8f,16e,8f,8b.4,16e,8d.,c";
prog_char song_33[] PROGMEM = "smb3lvl1:d=4,o=5,b=80:16g,32c,16g.,16a,32c,16a.,16b,32c,16b,16a.,32g#,16a.,16g,32c,16g.,16a,32c,16a,4b.,32p,16c6,32f,16c.6,16d6,32f,16d.6,16e6,32f,16e6,16d.6,32c#6,16d.6,16c6,32f,16c.6,16d6,32f,16d6,4e.6,32p,16g,32c,16g.,16a,32c,16a.,16b,32c,16b,16a.,32g#,16a.,16c6,8c.6,32p,16c6,4c.6";
prog_char song_34[] PROGMEM = "shinobi:d=4,o=5,b=140:b,f#6,d6,b,g,f#,e,2f#.,a,1f#,p,b,f#6,d6,b,g,f#,e,1f#.,8a,1b.,8a,1f#.,8a,1b.,8a,1f#.";
prog_char song_35[] PROGMEM = "outrun_magic:d=4,o=5,b=160:f6,d#6,8g#.6,f6,d#6,8c#.6,d#6,c6,2g#.,c#6,c6,8d#.6,c#6,c6,8f.,a#,16c.6,1a#,f6,d#6,8g#.6,f6,d#6,8c#.6,d#6,c6,2g#.,c#6,c6,8d#.6,c#6,c6,16f.,16g#.,c6,2a#.";
prog_char song_36[] PROGMEM = "Popeye:d=4,o=5,b=140:16g.,16f.,16g.,16p,32p,16c.,16p,32p,16c.,16p,32p,16e.,16d.,16c.,16d.,16e.,16f.,g,8p,16a,16f,16a,16c6,16b,16a,16g,16a,16g,8e,16g,16g,16g,16g,8a,16b,32c6,32b,32c6,32b,32c6,32b,8c6";
prog_char song_37[] PROGMEM = "Wonderboy:d=4,o=5,b=225:f6,d6,f6,8d6,f6,32p,8f6,d6,f6,d6,e6,c6,e6,8c6,e6,32p,8e6,c6,e6,c6";
prog_char song_38[] PROGMEM = "smwwd1:d=4,o=5,b=125:a,8f.,16c,16d,16f,16p,f,16d,16c,16p,16f,16p,16f,16p,8c6,8a.,g,16c,a,8f.,16c,16d,16f,16p,f,16d,16c,16p,16f,16p,16a#,16a,16g,2f,16p,8a.,8f.,8c,8a.,f,16g#,16f,16c,16p,8g#.,2g,8a.,8f.,8c,8a.,f,16g#,16f,8c,2c6";
prog_char song_39[] PROGMEM = "dkong:d=4,o=5,b=160:2c,8d.,d#.,c.,16b,16c6,16b,16c6,16b,16c6,16b,16c6,16b,16c6,16b,16c6,16b,2c6";
prog_char song_40[] PROGMEM = "BarbieGirl:d=4,o=5,b=125:8g#,8e,8g#,8c#6,a,p,8f#,8d#,8f#,8b,g#,8f#,8e,p,8e,8c#,f#,c#,p,8f#,8e,g#,f#";
prog_char song_41[] PROGMEM = "Coca-cola:d=4,o=5,b=125:8f#6,8f#6,8f#6,8f#6,g6,8f#6,e6,8e6,8a6,f#6,d6,2p";
prog_char song_42[] PROGMEM = "90210:d=4,o=5,b=140:8f,8a#,8c6,d.6,2d6,p,8f,8a#,8c6,8d6,8d#6,f6,f.6,2a#.,8f,8a#,8c6,8d6,8d#6,8f6,8g6,f6,8d#6,d#6,d6,2c.6,8a#,a,a#.,g6,8f6,8d#6,8d6,8d#6,8d6,8a#,f";
prog_char song_43[] PROGMEM = "Abdelazer:d=4,o=5,b=160:2d,2f,2a,d6,8e6,8f6,8g6,8f6,8e6,8d6,2c#6,a6,8d6,8f6,8a6,8f6,d6,2a6,g6,8c6,8e6,8g6,8e6,c6,2a6,f6,8b,8d6,8f6,8d6,b,2g6,e6,8a,8c#6,8e6,8c6,a,2f6,8e6,8f6,8e6,8d6,c#6,f6,8e6,8f6,8e6,8d6,a,d6,8c#6,8d6,8e6,8d6,2d6";
prog_char song_44[] PROGMEM = "aadams:d=4,o=5,b=160:8c,f,8a,f,8c,b4,2g,8f,e,8g,e,8e4,a4,2f,8c,f,8a,f,8c,b4,2g,8f,e,8c,d,8e,1f,8c,8d,8e,8f,1p,8d,8e,8f#,8g,1p,8d,8e,8f#,8g,p,8d,8e,8f#,8g,p,8c,8d,8e,8f";
prog_char song_45[] PROGMEM = "aadams:d=4,o=5,b=160:8c,f,8a,f,8c,b4,2g,8f,e,8g,e,8e4,a4,2f,8c,f,8a,f,8c,b4,2g,8f,e,8c,d,8e,1f,8c,8d,8e,8f,1p,8d,8e,8f#,8g,1p,8d,8e,8f#,8g,p,8d,8e,8f#,8g,p,8c,8d,8e,8f";
prog_char song_46[] PROGMEM = "Agadoo:d=4,o=5,b=125:8b,8g#,e,8e,8e,e,8e,8e,8e,8e,8d#,8e,f#,8a,8f#,d#,8d#,8d#,d#,8d#,8d#,8d#,8d#,8c#,8d#,e";
prog_char song_47[] PROGMEM = "Argentina:d=4,o=5,b=70:8e.4,8e4,8e4,8e.4,8f4,8g4,8a4,g4,8p,8g4,8a4,8a4,8g4,c,g4,8f4,e.4,8p,8e4,8f4,8g4,8d4,d4,8d4,8e4,8f4,c4,16p,8c4,8d4,8c4,8e4,g4,16p,8g4,8g4,8a4,c,16p";
prog_char song_48[] PROGMEM = "Auld L S:d=4,o=5,b=100:g,c.6,8c6,c6,e6,d.6,8c6,d6,8e6,8d6,c.6,8c6,e6,g6,2a.6,a6,g.6,8e6,e6,c6,d.6,8c6,d6,8e6,8d6,c.6,8a,a,g,2c.6";
prog_char song_49[] PROGMEM = " :d=4,o=5,b=125:g,8a#.,16g,16p,16g,8c6,8g,8f,g,8d.6,16g,16p,16g,8d#6,8d6,8a#,8g,8d6,8g6,16g,16f,16p,16f,8d,8a#,2g,p,16f6,8d6,8c6,8a#,g,8a#.,16g,16p,16g,8c6,8g,8f,g,8d.6,16g,16p,16g,8d#6,8d6,8a#,8g,8d6,8g6,16g,16f,16p,16f,8d,8a#,2g";
prog_char song_50[] PROGMEM = "axelf:d=4,o=5,b=160:f#,8a.,8f#,16f#,8a#,8f#,8e,f#,8c.6,8f#,16f#,8d6,8c#6,8a,8f#,8c#6,8f#6,16f#,8e,16e,8c#,8g#,f#.";
prog_char song_51[] PROGMEM = "girl:d=4,o=5,b=125:8g#,8e,8g#,8c#6,a,p,8f#,8d#,8f#,8b,g#,8f#,8e,p,8e,8c#,f#,c#,p,8f#,8e,g#,f#";
prog_char song_52[] PROGMEM = "Black Bear:d=4,o=5,b=180:d#,d#,8g.,16d#,8a#.,16g,d#,d#,8g.,16d#,8a#.,16g,f,8c.,16b4,c,8f.,16d#,8d.,16d#,8c.,16d,8a#.4,16c,8d.,16a#4,d#,d#,8g.,16d#,8a#.,16g,d#,d#,8g.,16d#,8a#.,16g,f,f,f,8g.,16f,d#,g,2d#";
prog_char song_53[] PROGMEM = "Bebopalula:d=4,o=5,b=180:2p,2a,a,8a,8e,g,a,a,a,g,a,8p,8a,8a,8e,g,8a,8a,a,a,g,a";
prog_char song_54[] PROGMEM = "Be-Bop-A-Lula:d=4,o=5,b=180:2p,2a,a,8a,8e,g,a,a,a,g,a,8p,8a,8a,8e,g,8a,8a,a,a,g,a";
prog_char song_55[] PROGMEM = "Birdy S:d=4,o=5,b=100:16g,16g,16a,16a,16e,16e,8g,16g,16g,16a,16a,16e,16e,8g,16g,16g,16a,16a,16c6,16c6,8b,8b,8a,8g,8f,16f,16f,16g,16g,16d,16d,8f,16f,16f,16g,16g,16d,16d,8f,16f,16f,16g,16g,16a,16b,8c6,8a,8g,8e,c";
prog_char song_56[] PROGMEM = "Bogey:d=4,o=5,b=140:8g,8e,p,8p,8e,8f,8g,e6,e6,2c6,8g,8e,p,8p,8e,8f,8e,g,g,2f,8f,8d,p,8p,8d,8e,8f,8g,8e,p,8p,8e,8f#,8e,8d,8g,8p,8e,8f#,8d,8p,8a,8g.,16f#,8g,8a,8g,8f,8e,8d,8c";
prog_char song_57[] PROGMEM = "Bolero:d=4,o=5,b=80:c6,8c6,16b,16c6,16d6,16c6,16b,16a,8c6,16c6,16a,c6,8c6,16b,16c6,16a,16g,16e,16f,2g,16g,16f,16e,16d,16e,16f,16g,16a,g,g,16g,16a,16b,16a,16g,16f,16e,16d,16e,16d,8c,8c,16c,16d,8e,8f,d,2g";
prog_char song_58[] PROGMEM = "Bulletme:d=4,o=5,b=112:b.6,g.6,16f#6,16g6,16f#6,8d.6,8e6,p,16e6,16f#6,16g6,8f#.6,8g6,8a6,b.6,g.6,16f#6,16g6,16f#6,8d.6,8e6,p,16c6,16b,16a,16b";
prog_char song_59[] PROGMEM = " :d=4,o=5,b=80:8d,8f#,8a,8d6,8c#,8e,8a,8c#6,8d,8f#,8b,8d6,8a,8c#,8f#,8a,8b,8d,8g,8b,8a,8d,8f#,8a,8b,8f#,8g,8b,8c#,8e,8a,8c#6,f#6,8f#,8a,e6,8e,8a,d6,8f#,8a,c#6,8c#,8e,b,8d,8g,a,8f#,8d,b,8d,8g,c#.6";
prog_char song_60[] PROGMEM = "careaboutus:d=4,o=5,b=125:16f,16e,16f,16e,16f,16e,8d,16e,16d,16e,16d,16e,16d,16c,16d,d";
prog_char song_61[] PROGMEM = "Children:d=4,o=5,b=63:8p,f.6,1p,g#6,8g6,d#.6,1p,g#6,8g6,c.6,1p,g#6,8g6,g#.,1p,16f,16g,16g#,16c6,f.6,1p,g#6,8g6,d#.6,1p,16c#6,16c6,c#6,8c6,g#,2p,g.,g#,8c6,f.";
prog_char song_62[] PROGMEM = " :d=4,o=5,b=70:16e,16f,16g,16a,16b,16c6,16d6,16d6,16d6,c6,e6,8d6,8c6,16b,16c6,32g,32a,16e,f,f,8g,8a,8b,16c6,8b,16d6,16a,16b,16d6,16d6,16a,16b,16c6,16b,16f,16b,8a,f,e,8c6,d,8b,e,8a,8e,8f,8g,8a,8b";
prog_char song_63[] PROGMEM = "countdown:d=4,o=5,b=125:p,8p,16b,16a,b,e,p,8p,16c6,16b,8c6,8b,a,p,8p,16c6,16b,c6,e,p,8p,16a,16g,8a,8g,8f#,8a,g.,16f#,16g,a.,16g,16a,8b,8a,8g,8f#,e,c6,2b.,16b,16c6,16b,16a,1b";
prog_char song_64[] PROGMEM = "Crypt:d=4,o=5,b=160:d#,f#,a,8p,8b,a#,f#,d#,8p,8b4,a#4,d#,f#,a,2b4,8p,a#4,d,f,8p,8f#,g#,b,a#,8p,8g#,f#,f,d#,d,2d#,1p,1p,p.,f,g#,b,8p,8c#6,c6,g#,f,8p,8c#,c,f,g#,b,1c#,c,e,g,8p,8g#,a#,c#6,c6,8p,8a#,g#,g,f,e,2f,16p";
prog_char song_65[] PROGMEM = "Dallas:d=4,o=5,b=125:8e,a.,8e,e.6,8a,c#6,8b,8c#6,a,e,a,f#6,e6,8c#6,8d6,2e.6,8p,8e,a,f#6,e6,8c#6,8d6,e6,8b,8c#6,a,e,a,8c#6,8d6,b.,8a,2a";
prog_char song_66[] PROGMEM = "dark:d=4,o=5,b=140:8f#6,8e6,2f#6,16e6,16d#6,16d6,16b,a#,1b,8f#,8e,2f#,8c#,8d,8a#4,1b4,8f#,8e,2f#,16e,16d#,16d,16b4,a#4,1b4,8f#,8e,2f#,c#,2d,2e4,1b4";
prog_char song_67[] PROGMEM = "DasBoot:d=4,o=5,b=100:d#.4,8d4,8c4,8d4,8d#4,8g4,a#.4,8a4,8g4,8a4,8a#4,8d,2f.,p,f.4,8e4,8d4,8e4,8f4,8a4,c.,8b4,8a4,8b4,8c,8e,2g.,2p";
prog_char song_68[] PROGMEM = "DavyCrockett:d=4,o=5,b=160:f,8f.,16g,8a.,16g,8f.,16c,d,f,2c,f,g,a,8g.,16f,g,8g.,16a,2g,c,8c.,16c,f,8c.,16c,d,8d.,16d,2g,e,8e.,16e,e,8e.,16d,c,8d.,16e,2f,a,2c.6,d.6,8d6,8c6,a.,8c.,16c,8c.,16c,e,g,2f.,p,a,2c.6,d.6,8d6,8c6,a.,8c.,16c,8c.,16c,e,g,2f.";
prog_char song_69[] PROGMEM = " :d=4,o=5,b=100:c.,c,8c,c.,d#,8d,d,8c,c,8c,2c.";
prog_char song_70[] PROGMEM = "Deutschlandlied:d=4,o=5,b=160:2g,8a,b,a,c6,b,8a,8f#,g,e6,d6,c6,b,a,8b,8g,2d6,2g,8a,b,a,c6,b,8a,8f#,g,e6,d6,c6,b,a,8b,8g,2d6,a,b,8a,8f#,d,c6,b,8a,8f#,d,d6,c6,2b,8b,c#6,8c#6,8d6,2d6,2g6,8f#6,8f#6,8e6,d6,2e6,8d6,8d6,8c6,b,2a,16b,16c6,8d6,8e6,8c6,8a,2g,8b,8a,2g";
prog_char song_71[] PROGMEM = "Do you hear the people sing:d=4,o=5,b=140:8e.6,16d6,8c.6,16d6,8e.6,16f6,g6,8e6,8d6,8c6,8b.,16a,8b.,16c6,g,8a,8g,8f,8e.,16g,8c.6,16e6,8d.6,16c#6,8d.6,16a,8c.6,16b,8b.,16c6,d6";
prog_char song_72[] PROGMEM = "don'tcare:d=4,o=5,b=125:16f,16e,16f,16e,16f,16e,8d,16e,16d,16e,16d,16e,16d,16c,16d,d";
prog_char song_73[] PROGMEM = "don't wanna miss a thing:d=4,o=5,b=125:2p,16a,16p,16a,16p,8a.,16p,a,16g,16p,2g,16p,p,8p,16g,16p,16g,16p,16g,8g.,16p,c6,16a#,16p,a,8g,f,g,8d,8f.,16p,16f,16p,16c,8c,16p,a,8g,16f,16p,8f,16p,16c,16p,g,f";
prog_char song_74[] PROGMEM = "dualingbanjos:d=4,o=5,b=200:8c#,8d,e,c#,d,b4,c#,d#4,b4,p,16c#6,16p,16d6,16p,8e6,8p,8c#6,8p,8d6,8p,8b,8p,8c#6,8p,8a,8p,b,p,a4,a4,b4,c#,d#4,c#,b4,p,8a,8p,8a,8p,8b,8p,8c#6,8p,8a,8p,8c#6,8p,8b";
prog_char song_75[] PROGMEM = "Dustman:d=4,o=5,b=140:8a.,16a,16b,16p,16c6,16p,8c#6,p,8e6,16c#6,16p,16c#6,16p,16c#6,16p,16c#6,16p,16c#6,16p,c#6,16c#6,16p,16c#6,16p,16c#6,16p,16d6,16p,16c#6,16p,b,16b,16p,16b,16p,16b,16p,16b,16p,16b,16p,16b,16p,16b,16p,8b.,16p,16e6,16e6,16e6,16p,16d6,16p,16c#6,16p,16b,16p,a";
prog_char song_76[] PROGMEM = "Equidor:d=4,o=5,b=140:8g.,8d.,8a#,8a,8c6,8a,8f,8g.,8d.,8a#,8a,8c6,8a,8f,8a#.,8f.,8d6,8c6,8d6,8c6,8a,8a#.,8g.,8a#,8a,8a#,8a,8f";
prog_char song_77[] PROGMEM = "Eternally:d=4,o=5,b=112:b,8b,8a,8b,8c6,a,8a,8g,8a,8b,g,8g,8f#,8e,8d#,2e";
prog_char song_78[] PROGMEM = "Exodus:d=4,o=5,b=70:8c#,f#.,8c#6,b.,8f#,8a,8b,8g#.,16e,f#.,8c#6,e.6,8d#6,8e6,8f#6,8d#.6,16b,2c#6";
prog_char song_79[] PROGMEM = "Fawlty:d=4,o=5,b=125:8b,8c6,8d6,8c#6,8d6,8c#6,8d6,8g6,e.6,8d6,8c6,8b,8c6,8b,8c6,8b,8c6,8f#6,d.6,8c6,8b,8a,8g,8f#,8g,8f#,8g,8d6,8c6,8b,8c6,8b,8a,8g,8f#,8g,8e,8f#,d,8c6,8d6,8b,8c6,a";
prog_char song_80[] PROGMEM = "Flntstn:d=4,o=5,b=200:g#,c#,8p,c#6,8a#,g#,c#,8p,g#,8f#,8f,8f,8f#,8g#,c#,d#,2f,2p,g#,c#,8p,c#6,8a#,g#,c#,8p,g#,8f#,8f,8f,8f#,8g#,c#,d#,2c#";
prog_char song_81[] PROGMEM = "Friends:d=4,o=5,b=80:c,g,a#4,f,c,g,a#4,8a#,8e,c,g,a#4,f,c,g,a#4,8a#,8e";
prog_char song_82[] PROGMEM = "Fun2Remix:d=4,o=5,b=320:c6,8c6,g,8g,a,a#,a,g,a,c6,8c6,g,8g,a,a#,a,g,a,a#,8a#,f,8f,g,g#,g,f,g,c6,8c6,c6,8c6,8c6,8c6,c6,c6,c6,c6";
prog_char song_83[] PROGMEM = "FunkyTown:d=4,o=4,b=125:8c6,8c6,8a#5,8c6,8p,8g5,8p,8g5,8c6,8f6,8e6,8c6,2p,8c6,8c6,8a#5,8c6,8p,8g5,8p,8g5,8c6,8f6,8e6,8c6";
prog_char song_84[] PROGMEM = "song11:d=4,o=5,b=125:8g.,8g.,8g,8c,8c,8d,8d,8g.,8g.,8g,8a#,8a#,8c6,8c6,8g.,8g.,8g,8c,8c,8d,8d,8g.,8g.,8g,8a#,8a#,8c6,8d6";
prog_char song_85[] PROGMEM = "National Anthem:d=4,o=5,b=140:g6,g6,a6,f#.6,8g6,a6,b6,b6,c7,b.6,8a6,g6,a6,g6,f#6,g6";
prog_char song_86[] PROGMEM = "Greensleaves:d=4,o=5,b=140:g,2a#,c6,d.6,8d#6,d6,2c6,a,f.,8g,a,2a#,g,g.,8f,g,2a,f,2d,g,2a#,c6,d.6,8e6,d6,2c6,a,f.,8g,a,a#.,8a,g,f#.,8e,f#,2g";
prog_char song_87[] PROGMEM = "Halloween:d=4,o=5,b=180:8d6,8g,8g,8d6,8g,8g,8d6,8g,8d#6,8g,8d6,8g,8g,8d6,8g,8g,8d6,8g,8d#6,8g,8c#6,8f#,8f#,8c#6,8f#,8f#,8c#6,8f#,8d6,8f#,8c#6,8f#,8f#,8c#6,8f#,8f#,8c#6,8f#,8d6,8f#";
prog_char song_88[] PROGMEM = "HeyBaby:d=4,o=5,b=900:8a4,16a#4,16b4,16c,16c#,16d,16d#,16e,16f,16f#,16g,16g#,16a,16a#,16b,16c6,8c#6,16d6,16d#6,16e6,16f6,p,p,16a4,16a#4,16b4,16c,16c#,16d,16d#,16e,16f,16f#,16g,16g#,16a,16a#,16b,16a#,16a,16g#,16g,16f#,16f,16e,16d#,16d,16c#,16c,16b4,16a#4,16a4";
prog_char song_89[] PROGMEM = "Hitchcoc:d=4,o=5,b=200:16c,16p,16f4,8p,8f,32g,32p,16f,32p,16e,32p,16d,32p,16e,8p,16f,32p,16g,8p.,16c,16p,16f4,8p,8f,32g,32p,16f,32p,16e,32p,16d,32p,16e,8p,16f,32p,16g,8p.,16c,16p,16f4,8p,16g#,32p,8c6,16p,16a#,32p,16g#,8p,16c6,32p,8d#6,16p,16c#6,32p,16c6,8p,16d#6,32p,8g6,16p,16f6,32p,16e6,32p,16c#6,32p,16c6,32p,16a#,32p,16g#,32p,16g,32p,8f4";
prog_char song_90[] PROGMEM = "Ickley:d=4,o=5,b=100:8d,8g.,16g,8g,8d,g,8p,8a,8b.,16b,8b,8a,b,8p,8b,a,g,g,f#,2g";
prog_char song_91[] PROGMEM = "Indiana:d=4,o=5,b=250:e,8p,8f,8g,8p,1c6,8p.,d,8p,8e,1f,p.,g,8p,8a,8b,8p,1f6,p,a,8p,8b,2c6,2d6,2e6,e,8p,8f,8g,8p,1c6,p,d6,8p,8e6,1f.6,g,8p,8g,e.6,8p,d6,8p,8g,e.6,8p,d6,8p,8g,f.6,8p,e6,8p,8d6,2c6";
prog_char song_92[] PROGMEM = "GirlFromIpane:d=4,o=5,b=160:g.,8e,8e,d,g.,8e,e,8e,8d,g.,e,e,8d,g,8g,8e,e,8e,8d,f,d,d,8d,8c,e,c,c,8c,a#4,2c";
prog_char song_93[] PROGMEM = "I swear:d=4,o=5,b=125:2p,p,8b,8a.,16f#,8e,p,8p,8f#,8g#,a,8a,8a,a,8c#,8d,2e,8p,8f#,8g#,2e";
prog_char song_94[] PROGMEM = "Itchy:d=4,o=5,b=160:8c6,8a,p,8c6,8a6,p,8c6,8a,8c6,8a,8c6,8a6,p,8p,8c6,8d6,8e6,8p,8e6,8f6,8g6,p,8d6,8c6,d6,8f6,a#6,a6,2c7";
prog_char song_95[] PROGMEM = "Jesus:d=4,o=5,b=100:f,8d,2a#4,g,8d#,2a#4,g#,8f,8g#,g,8f,8d#,f,8d,2a#4";
prog_char song_96[] PROGMEM = "killing me softly:d=4,o=5,b=90:p,8e,f,g,8a,a,8g,d,g.,p,8p,8a,g,8f,8e,8e,8f,2c,p,8e,f,g,8a,a,8g,a,b,8b,8c6,8b,16a,8g,16a,2a,2a.";
prog_char song_97[] PROGMEM = "KnightRider:d=4,o=5,b=125:16e,16p,16f,16e,16e,16p,16e,16e,16f,16e,16e,16e,16d#,16e,16e,16e,16e,16p,16f,16e,16e,16p,16f,16e,16f,16e,16e,16e,16d#,16e,16e,16e,16d,16p,16e,16d,16d,16p,16e,16d,16e,16d,16d,16d,16c,16d,16d,16d,16d,16p,16e,16d,16d,16p,16e,16d,16e,16d,16d,16d,16c,16d,16d,16d";
prog_char song_98[] PROGMEM = "Lazy:d=4,o=5,b=160:8d.4,8f4,16d4,8g4,16f4,8d.4,8f4,16d4,8g4,16f4,8d4,8p,8p";
prog_char song_99[] PROGMEM = "Walk of Life:d=4,o=5,b=160:b.,b.,p,8p,8f#,8g,b,8g,8f,e.,e.,p,2p,p,8f,8g,b.,b.,p,8p,8f,8g,b,8g,f,e.,e.,p,8p,8f,8g,b,8g,8f,8e";
prog_char song_100[] PROGMEM = "Little Wing:d=4,o=5,b=63:2p,p,8e,8g,8a,a.,p,8a,8g,8g,e.,p,8d,8c,8d,16e,8d.,8p,8d,8d,8c,2a";
prog_char song_101[] PROGMEM = "Looney:d=4,o=5,b=140:c6,8f6,8e6,8d6,8c6,a.,8c6,8f6,8e6,8d6,8d#6,e.6,8e6,8e6,8c6,8d6,8c6,8e6,8c6,8d6,8a,8c6,8g,8a#,8a,8f";
prog_char song_102[] PROGMEM = "losing:d=4,o=5,b=63:2p,8b,8c#6,8b,8f#,a.,8a,8a,a,a,a.,8b,8c#6,8b,8f#,a.,8a,8a,a,a.,8b,8c#6,8b,8f#,a.,8a,8a,a,a.,8b,8c#6,8b,8f#,a,a,8a,a,8g#,2g#";
prog_char song_103[] PROGMEM = "Lulay Lula:d=4,o=4,b=100:d6,d6,c#6,2d6,f6,8e6,8e6,e6,d6,2c#.6,d6,e6,f6,g6,2e6,2d6,a6,2g6,f6,2e6,f6,8e6,8e6,e6,d6,2c#.6,d6,e6,f6,g6,2e6,2f#6";
prog_char song_104[] PROGMEM = "Macarena:d=4,o=5,b=180:f,8f,8f,f,8f,8f,8f,8f,8f,8f,8f,8a,8c,8c,f,8f,8f,f,8f,8f,8f,8f,8f,8f,8d,8c,p,f,8f,8f,f,8f,8f,8f,8f,8f,8f,8f,8a,p,2c.6,a,8c6,8a,8f,p,2p";
prog_char song_105[] PROGMEM = "Barbie girl:d=4,o=5,b=125:8g#,8e,8g#,8c#6,a,p,8f#,8d#,8f#,8b,g#,8f#,8e,p,8e,8c#,f#,c#,p,8f#,8e,g#,f#,8g#,8e,8g#,8c#6,a,p,8f#,8d#,8f#,8b,g#,8f#,8e,p,8e,8c#,f#,c#,p,8f#,8e,g#,f#,8g#,8e,8g#,8c#6,a,p,8f#,8d#,8f#,8b,g#,8f#,8e,p,8e,8c#,f#,c#,p,8f#,8e,g#,f#";
prog_char song_106[] PROGMEM = "Match of the day:d=4,o=5,b=100:8c,8f,8a,8c.6,16a,8a,8a,8a,a,8a#,8c.6,16a,8g,8a,8a#,8c,8e,8g,8a#.,16g,8g,8g,8g,g,8a,8a#.,16g,8f,8g,8a,8c,8f,8a,8c.6,16a,8a,8a,8a,a,8a#,8c.6,16a,8a#,8c6,d6,8d6,8e6,8f6,16f6,8e6,16e6,8d6,8f6,8c6,8c6,8d6,8c6,16a#,8a,16a,8g,f";
prog_char song_107[] PROGMEM = "missathing:d=4,o=5,b=125:2p,16a,16p,16a,16p,8a.,16p,a,16g,16p,2g,16p,p,8p,16g,16p,16g,16p,16g,8g.,16p,c6,16a#,16p,a,8g,f,g,8d,8f.,16p,16f,16p,16c,8c,16p,a,8g,16f,16p,8f,16p,16c,16p,g,f";
prog_char song_108[] PROGMEM = "Mission:d=4,o=6,b=100:32d,32d#,32d,32d#,32d,32d#,32d,32d#,32d,32d,32d#,32e,32f,32f#,32g,16g,8p,16g,8p,16a#,16p,16c,16p,16g,8p,16g,8p,16f,16p,16f#,16p,16g,8p,16g,8p,16a#,16p,16c,16p,16g,8p,16g,8p,16f,16p,16f#,16p,16a#,16g,2d,32p,16a#,16g,2c#,32p,16a#,16g,2c,16p,16a#5,16c";
prog_char song_109[] PROGMEM = "songs12:d=4,o=5,b=112:8e6,8e6,8e6,8e6,8e6,8e6,16e,16a,16c6,16e6,8d#6,8d#6,8d#6,8d#6,8d#6,8d#6,16f,16a,16c6,16d#6,d6,8c6,8a,8c6,c6,2a,32a,32c6,32e6,8a6";
prog_char song_110[] PROGMEM = "Monty P:d=4,o=5,b=200:f6,8e6,d6,8c#6,c6,8b,a#,8a,8g,8a,8a#,a,8g,2c6,8p,8c6,8a,8p,8a,8a,8g#,8a,8f6,8p,8c6,8c6,8p,8a,8a#,8p,8a#,8a#,8p,8c6,2d6,8p,8a#,8g,8p,8g,8g,8f#,8g,8e6,8p,8d6,8d6,8p,8a#,8a,8p,8a,8a,8p,8a#,2c6,8p,8c6";
prog_char song_111[] PROGMEM = "munsters:d=4,o=5,b=160:d,8f,8d,8g#,8a,d6,8a#,8a,2g,8f,8g,a,8a4,8d#4,8a4,8b4,c#,8d,p,c,c6,c6,2c6,8a#,8a,8a#,8g,8a,f,p,g,g,2g,8f,8e,8f,8d,8e,2c#,p,d,8f,8d,8g#,8a,d6,8a#,8a,2g,8f,8g,a,8d#4,8a4,8d#4,8b4,c#,2d";
prog_char song_112[] PROGMEM = "LightMyFire:d=4,o=5,b=140:8b,16g,16a,8b,8d6,8c6,8b,8a,8g,8a,16f,16a,8c6,8f6,16d6,16c6,16a#,16g,8g#,8g,8g#,16g,16a,8b,8c#6,16b,16a,16g,16f,8e,8f,1a,a";
prog_char song_113[] PROGMEM = "Newyear:d=4,o=5,b=125:a4,d.,8d,d,f#,e.,8d,e,8f#,8e,d.,8d,f#,a,2b.,b,a.,8f#,f#,d,e.,8d,e,8f#,8e,d.,8b4,b4,a4,2d,16p";
prog_char song_114[] PROGMEM = "PinkPanther:d=4,o=5,b=160:8d#,8e,2p,8f#,8g,2p,8d#,8e,16p,8f#,8g,16p,8c6,8b,16p,8d#,8e,16p,8b,2a#,2p,16a,16g,16e,16d,2e";
prog_char song_115[] PROGMEM = "peanuts:d=4,o=5,b=160:f,8g,a,8a,8g,f,2g,f,p,f,8g,a,1a,2p,f,8g,a,8a,8g,f,2g,2f,2f,8g,1g";
prog_char song_116[] PROGMEM = "piccolo:d=4,o=5,b=320:d6,g6,g,g6,8d6,8e6,8d6,8b,g,d,8g,8a,8b,8c6,d6,g6,1d6,d6,g6,g,g6,8d6,8e6,8b,g,d,8f,8g,8a,8b,c6,f6,1c6";
prog_char song_117[] PROGMEM = "Pilipom:d=4,o=5,b=160:16e,16p,16e,16p,16g,16p,16g,16p,16b4,16c#,16d,16p,16g,16p,16g,16p,16e,16p,16e,16p,16g,16p,16g,16p,16b,16g,16b,16e6,8d#6,8p,16d#6,16d6,16b,16a#,16d#6,16d6,16b,16a#,16d#6,16d6,16b,16a#,16b,16c6,16d6,16d#6,16b,16a#,16g,16f#,16e,16d#,16c,16b4,16e,16f#,16d#,16b4,8e,16p";
prog_char song_118[] PROGMEM = "Poison:d=4,o=5,b=112:8d,8d,8a,8d,8e6,8d,8d6,8d,8f#,8g,8c6,8f#,8g,8c6,8e,8d,8d,8d,8a,8d,8e6,8d,8d6,8d,8f#,8g,8c6,8f#,8g,8c6,8e,8d,8c,8d,8a,8d,8e6,8d,8d6,8d,8f#,8g,8c6,8f#,8g,8c6,8e,8d,8c,8d,8a,8d,8e6,8d,8d6,8d,8a,8d,8e6,8d,8d6,8d,2a,8d";
prog_char song_119[] PROGMEM = "polkka:d=4,o=5,b=140:16d,16c#,16d,16e,16f,16e,16f,16f#,16g,16f#,16g,16a,16a#,16a,16g,16a#,16a,16a4,16c#,16e,16a,16g,16f,16e,16f,16e,16d,16c#,16d,16a4,16b4,16c#,16d,16c#,16d,16e,16f,16e,16f,16f#,16g,16f#,16g,16a,16a#,16a,16g,16a#,16a,16a4,16c#,16e,16a,16g,16f,16e,16d,p,2c#,8d,8a4,8d";
prog_char song_120[] PROGMEM = "Popcorn:d=4,o=5,b=160:8c6,8a#,8c6,8g,8d#,8g,c,8c6,8a#,8c6,8g,8d#,8g,c,8c6,8d6,8d#6,16c6,8d#6,16c6,8d#6,8d6,16a#,8d6,16a#,8d6,8c6,8a#,8g,8a#,c6";
prog_char song_121[] PROGMEM = "Postman Pat:d=4,o=5,b=100:16f#,16p,16a,16p,8b,8p,16f#,16p,16a,16p,8b,8p,16f#,16p,16a,16p,16b,16p,16d6,16d6,16c#6,16c#6,16a,16p,b.,8p,32f#,16g,16p,16a,16p,16b,16p,16g,16p,8f#.,8e,8p,32f#,16g,16p,16a,16p,32b.,32b.,16g,16p,8f#.,8e,8p,32f#,16g,16p,16a,16p,16b,16p,16g,16p,16f#,16p,16e,16p,16d,16p,16c#,16p,2d";
prog_char song_122[] PROGMEM = "Rhubarb:d=4,o=5,b=180:8e,8f,8g,d#.,8e,8f,8g,d#.,8e,8f,8g,a#,8a#,2g.,8e,8f,8g,d#.,8e,8f,8g,d#.,e,8e,d,8d,2c.";
prog_char song_123[] PROGMEM = "Rikasmiesjos:d=4,o=5,b=160:8g,8f,8g,8f,e,c,p,8e,8f,8g,8f,8g,8f,8e,8f,8g,8a,8a#,8a,8a#,8a,g,p,g#,g,f#,f,8d#,8d,8c,8d,d#,p,8d#,8d,8c,8d,d#,c,g,p";
prog_char song_124[] PROGMEM = "Kiss:d=4,o=5,b=140:8d4,8e4,f.4,8g4,f4,e4,d4,c4,2d4,8d4,8c4,2d4,8d4,8e4,f.4,8g4,f4,e4,c4,e4,2d.4";
prog_char song_125[] PROGMEM = "Rule B:d=4,o=5,b=100:e.,8e,8f,f,8e,8f.,16e,8d.,16c,2b4,g,f,16e,16c,16f,16d,8g,8f,e,8d.,16c,c";
prog_char song_126[] PROGMEM = "Scatman:d=4,o=5,b=200:8b,16b,32p,8b,16b,32p,8b,2d6,16p,16c#.6,16p.,8d6,16p,16c#6,8b,16p,8f#,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8f#,2p,32p,2d6,16p,16c#6,8p,16d.6,16p.,16c#6,16a.,16p.,8e,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8b,16b,32p,8b,16b,32p,8b,2d6,16p,16c#.6,16p.,8d6,16p,16c#6,8b,16p,8f#,2p.,16c#6,8p,16d.6,16p.,16c#6,16b,8p,8f#,2p,32p,2d6,16p,16c#6,8p,16d.6,16p.,16c#6,16a.,16p.,8e,2p.,16c#6,8p,16d.6,16p.,16c#6,16a,8p,8e,2p,32p,16f#.6,16p.,16b.,16p.";
prog_char song_127[] PROGMEM = "Schweine:d=4,o=5,b=180:8g.,16p,16g.,8p,16a.,8p,8a,16p,8b,8p,8b.,16p,16d6,16p,d6,16p,e6,16p,16e6,8p,16b.,8p,16b.,8p,16a.,8p,8a.,16p,16g.,16p,g,16p,8d.6,16p,16d6,8p,8c6,16p,8c.6,16p,8b,16p,16b.,16p,8a.,16p,a,16p,8d.6,16p,16d6,8p,8c6,16p,8c.6,16p,8b,16p,16e.6,16p,8b.,16p,d.6,8p";
prog_char song_128[] PROGMEM = "ScoobyDoo:d=4,o=5,b=160:8e6,8e6,8d6,8d6,2c6,8d6,e6,2a,8a,b,g,e6,8d6,c6,8d6,2e6,p,8e6,8e6,8d6,8d6,2c6,8d6,f6,2a,8a,b,g,e6,8d6,2c6";
prog_char song_129[] PROGMEM = "shoopsong:d=4,o=5,b=125:g,g,g,g,f,8f,8d#,8f,8d#,c,g,8g,8g,g,8g,8g,b,8g,g.,8e,8d,8f,e,d.";
prog_char song_130[] PROGMEM = "Skala:d=4,o=5,b=160:32c,32d,32e,32f,32g,32a,32b,32c6,32b,32a,32g,32f,32e,32d,32c";
prog_char song_131[] PROGMEM = "Soap:d=4,o=5,b=125:g,8a,8c6,8p,8a,c6,p,8a,8g,8e,8c,p,g,8a,8c6,p,b,p,8a,8g,8e,8c#,2p,p,8a,8c6,2p,p,8a,8g,2p,8a,8g,8e,c";
prog_char song_132[] PROGMEM = "Song1:d=4,o=5,b=100:2p,8p,16f#6,16f#6,16f#6,16e6,16d6,16c#6,b,8f#6,e6,16e6,16e6,16e6,16d6,16c#6,16b,a,8f#6,d6,16f#6,16f#6,16f#6,16e6,16d6,16c#6,b,8f#6,e6,8d6,8c#6,2d6";
prog_char song_133[] PROGMEM = "Song2:d=4,o=5,b=140:2p,d#6,e6,8f6,a.6,f6,e6,8d#6,g.6,d#6,a#,8p,8g6,8a,8d#6,8f6";
prog_char song_134[] PROGMEM = "song3:d=4,o=5,b=90:2p,8e,8g,8g,8e,a.,8g,g,8p,8g,g,8a,g,g,g,8g,8a,8g,8f,f,8f,g";
prog_char song_135[] PROGMEM = "song4:d=4,o=5,b=112:8p,8d,8d,d,8d,8d,e.,8f#,f#,8f#,8a,d.6,8a,b.,8f#,1e";
prog_char song_136[] PROGMEM = "song5:d=4,o=5,b=100:p,e,e.,8d,2e.,a,c.6,8b,a,g,e,2e,p,p,e,e.,8d,2e.,a,b.,8a,g,a,1e";
prog_char song_137[] PROGMEM = "song6:d=4,o=5,b=90:e,b,b,8b,8b,8c6,8b,8a,8g,f#.,8g,a,8a,8a,8a,8a,8b,8a,2g,f#,8p,8f#,8g,8g,8g,8e,f#.,8f#,8g,8g,8g,8e,f#.,8a,8a,8a,8b,8c6,b,8a,8g,2f#,e";
prog_char song_138[] PROGMEM = "song7:d=4,o=5,b=90:g,d,g,d,g,b,a#,a,g,d,g,d,g";
prog_char song_139[] PROGMEM = "song8:d=4,o=5,b=180:e.,g#.,b.,b,8e6,c#.6,b,8b,b.,p,a,8a,a,8b,a,8g#,8g#,8g#,8g#,g#,8g#,g#,8g#,8g#,f#,p";
prog_char song_140[] PROGMEM = "song9:d=4,o=5,b=140:c6,8b,8a,b,8a,8g,8a#,8a#,8a,8g,a,8g,8f,8p,8f,8f,8e,d,8a,2g";
prog_char song_141[] PROGMEM = "Wannabe:d=4,o=5,b=125:16g,16g,16g,16g,8g,8a,8g,8e,8p,16c,16d,16c,8d,8d,8c,e,p,8g,8g,8g,8a,8g,8e,8p,c6,8c6,8b,8g,8a,16b,16a,g";
prog_char song_142[] PROGMEM = "Stairway:d=4,o=5,b=63:8a6,8c6,8e6,8a6,8b6,8e6,8c6,8b6,8c7,8e6,8c6,8c7,8f#6,8d6,8a6,8f#6,8e6,8c6,8a6,c6,8e6,8c6,8a,8g,8g,8a,a";
prog_char song_143[] PROGMEM = "SWEnd:d=4,o=5,b=225:2c,1f,2g.,8g#,8a#,1g#,2c.,c,2f.,g,g#,c,8g#.,8c.,8c6,1a#.,2c,2f.,g,g#.,8f,c.6,8g#,1f6,2f,8g#.,8g.,8f,2c6,8c.6,8g#.,8f,2c,8c.,8c.,8c,2f,8f.,8f.,8f,2f";
prog_char song_144[] PROGMEM = "Cantina:d=4,o=5,b=250:8a,8p,8d6,8p,8a,8p,8d6,8p,8a,8d6,8p,8a,8p,8g#,a,8a,8g#,8a,g,8f#,8g,8f#,f.,8d.,16p,p.,8a,8p,8d6,8p,8a,8p,8d6,8p,8a,8d6,8p,8a,8p,8g#,8a,8p,8g,8p,g.,8f#,8g,8p,8c6,a#,a,g";
prog_char song_145[] PROGMEM = "StWars:d=4,o=5,b=180:8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6,p,8f,8f,8f,2a#.,2f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8c6,2a#.6,f.6,8d#6,8d6,8d#6,2c6";
prog_char song_146[] PROGMEM = "Star Trek:d=4,o=5,b=63:8f.,16a#,d#.6,8d6,16a#.,16g.,16c.6,f6";
prog_char song_147[] PROGMEM = "SuperMan:d=4,o=5,b=180:8g,8g,8g,c6,8c6,2g6,8p,8g6,8a.6,16g6,8f6,1g6,8p,8g,8g,8g,c6,8c6,2g6,8p,8g6,8a.6,16g6,8f6,8a6,2g.6,p,8c6,8c6,8c6,2b.6,g.6,8c6,8c6,8c6,2b.6,g.6,8c6,8c6,8c6,8b6,8a6,8b6,2c7,8c6,8c6,8c6,8c6,8c6,2c.6";
prog_char song_148[] PROGMEM = "TheSweeney:d=4,o=5,b=125:16a,8c6,a.,p.,16a,8e6,2d6,p.,8p,c6,8c6,16a.,8c6,e.6,8d6,16a,c6,8d6,16a,8c6,a.,p.,16a,8e6,2d6,p.,8p,e6,8e6,16d#.6,8e6,f.6,c6,b,a,2f.6,c6,8g6,1f6";
prog_char song_149[] PROGMEM = "T Birds:d=4,o=4,b=125:8g#5,16f5,16g#5,a#5,8p,16d#5,16f5,8g#5,8a#5,8d#6,16f6,16c6,8d#6,8f6,2a#5,8g#5,16f5,16g#5,a#5,8p,16d#5,16f5,8g#5,8a#5,8d#6,16f6,16c6,8d#6,8f6,2g6,8g6,16a6,16e6,g6,8p,16e6,16d6,8c6,8b5,8a.5,16b5,8c6,8e6,2d6,8d#6,16f6,16c6,d#6,8p,16c6,16a#5,8g#5,8g5,8f.5,16g5,8g#5,8a#5,8c6,8a#5,8g5,8d#5";
prog_char song_150[] PROGMEM = "tears:d=4,o=5,b=112:p,8b,8g,d6,8d6,8b,16a,g.,2p,p,8c6,8c6,8b,8a,8g,b,2a";
prog_char song_151[] PROGMEM = "Time to say good bye:d=4,o=5,b=80:8c,16d,16e,16d,16e,16f#,16g,16f#,16g,16a,16g,16e,16a,16b,c6,b";
prog_char song_152[] PROGMEM = "Timetosay:d=4,o=5,b=80:8c,16d,16e,16d,16e,16f#,16g,16f#,16g,16a,16g,16e,16a,16b,c6,b";
prog_char song_153[] PROGMEM = "Time to say good bye:d=4,o=5,b=80:8c,16d,16e,16d,16e,16f#,16g,16f#,16g,16a,16g,16e,16a,16b,c6,b";
prog_char song_154[] PROGMEM = "Wannabe:d=4,o=5,b=125:16g,16g,16g,16g,8g,8a,8g,8e,8p,16c,16d,16c,8d,8d,8c,e,p,8g,8g,8g,8a,8g,8e,8p,c6,8c6,8b,8g,8a,16b,16a,g";
prog_char song_155[] PROGMEM = "Vil du værra me' mæ hjem:d=4,o=5,b=100:2p,8p,16f#6,16f#6,16f#6,16e6,16d6,16c#6,b,8f#6,e6,16e6,16e6,16e6,16d6,16c#6,16b,a,8f#6,d6,16f#6,16f#6,16f#6,16e6,16d6,16c#6,b,8f#6,e6,8d6,8c#6,2d6";
prog_char song_156[] PROGMEM = "They don't care about us::d=4,o=5,b=125:16f,16e,16f,16e,16f,16e,8d,16e,16d,16e,16d,16e,16d,16c,16d,d";
prog_char song_157[] PROGMEM = "Solskinnsdag:d=4,o=5,b=140:2p,d#6,e6,8f6,a.6,f6,e6,8d#6,g.6,d#6,a#,8p,8g6,8a,8d#6,8f6";
prog_char song_158[] PROGMEM = "More than words:d=4,o=5,b=90:2p,8e,8g,8g,8e,a.,8g,g,8p,8g,g,8a,g,g,g,8g,8a,8g,8f,f,8f,g";
prog_char song_159[] PROGMEM = "Bullet me:d=4,o=5,b=112:b.6,g.6,16f#6,16g6,16f#6,8d.6,8e6,p,16e6,16f#6,16g6,8f#.6,8g6,8a6,b.6,g.6,16f#6,16g6,16f#6,8d.6,8e6,p,16c6,16b,16a,16b";
prog_char song_160[] PROGMEM = "The shoop shoop song:d=4,o=5,b=125:g,g,g,g,f,8f,8d#,8f,8d#,c,g,8g,8g,g,8g,8g,b,8g,g.,8e,8d,8f,e,d.";
prog_char song_161[] PROGMEM = "Losing my religion::d=4,o=5,b=63:2p,8b,8c#6,8b,8f#,a.,8a,8a,a,a,a.,8b,8c#6,8b,8f#,a.,8a,8a,a,a.,8b,8c#6,8b,8f#,a.,8a,8a,a,a.,8b,8c#6,8b,8f#,a,a,8a,a,8g#,2g#";
prog_char song_162[] PROGMEM = "Eternally:d=4,o=5,b=112:b,8b,8a,8b,8c6,a,8a,8g,8a,8b,g,8g,8f#,8e,8d#,2e";
prog_char song_163[] PROGMEM = "The final countdown:d=4,o=5,b=125:p,8p,16b,16a,b,e,p,8p,16c6,16b,8c6,8b,a,p,8p,16c6,16b,c6,e,p,8p,16a,16g,8a,8g,8f#,8a,g.,16f#,16g,a.,16g,16a,8b,8a,8g,8f#,e,c6,2b.,16b,16c6,16b,16a,1b";
prog_char song_164[] PROGMEM = "Tears in heaven:d=4,o=5,b=112:p,8b,8g,d6,8d6,8b,16a,g.,2p,p,8c6,8c6,8b,8a,8g,b,2a";
prog_char song_165[] PROGMEM = "Let it be:d=4,o=5,b=100:16e6,8d6,c6,16e6,8g6,8a6,8g.6,16g6,8g6,8e6,16d6,8c6,16a,8g,e.6,p,8e6,16e6,8f.6,8e6,8e6,8d6,16p,16e6,16d6,8d6,2c.6";
prog_char song_166[] PROGMEM = "Frank Mills:d=4,o=5,b=112:e,8e,8e,e,g,d,d,p,8e,8g,c6,c6,c6,e6,a.,8a,a,8b,8c6,8a,8g,g,p,c6,g,8f,8e,f,c6,p,8p,8a,b,8a,8b,1c6";
prog_char song_167[] PROGMEM = "Do you hear the people sing:d=4,o=5,b=140:8e.6,16d6,8c.6,16d6,8e.6,16f6,g6,8e6,8d6,8c6,8b.,16a,8b.,16c6,g,8a,8g,8f,8e.,16g,8c.6,16e6,8d.6,16c#6,8d.6,16a,8c.6,16b,8b.,16c6,d6";
prog_char song_168[] PROGMEM = "Master of the house:d=4,o=5,b=100:16a,16a,16a,16a,8e,8p,16a,16a,16a,16a,8e,8p,16a,16a,16a,16a,16a,16g#,16a,16b,8c#6,8a,8e,8p";
prog_char song_169[] PROGMEM = "Castle on a Cloud:d=4,o=5,b=90:8a,16b,16c6,8b,8a,8a,8g#,a,p,8a,16b,16c6,8b,8a,8g,8f,e,p,8d,16e,16f,8e,8a,8b,8c6,a,p,8d,16e,16f,8e,8d,8c,8b,a";
prog_char song_170[] PROGMEM = "Aquarius:d=4,o=5,b=200:e,f#,1g.,a,g,8f#,e,d,1e.,d,8e,f#,2f#.,e,8e,d,8d,1e";
prog_char song_171[] PROGMEM = "Bogey:d=4,o=5,b=140:8g,8e,p,8p,8e,8f,8g,e6,e6,2c6,8g,8e,p,8p,8e,8f,8e,g,g,2f,8f,8d,p,8p,8d,8e,8f,8g,8e,p,8p,8e,8f#,8e,8d,8g,8p,8e,8f#,8d,8p,8a,8g.,16f#,8g,8a,8g,8f,8e,8d,8c";
prog_char song_172[] PROGMEM = "Greensleaves:d=4,o=5,b=140:g,2a#,c6,d.6,8d#6,d6,2c6,a,f.,8g,a,2a#,g,g.,8f,g,2a,f,2d,g,2a#,c6,d.6,8e6,d6,2c6,a,f.,8g,a,a#.,8a,g,f#.,8e,f#,2g";
prog_char song_173[] PROGMEM = "Canon:d=4,o=5,b=80:8d,8f#,8a,8d6,8c#,8e,8a,8c#6,8d,8f#,8b,8d6,8a,8c#,8f#,8a,8b,8d,8g,8b,8a,8d,8f#,8a,8b,8f#,8g,8b,8c#,8e,8a,8c#6,f#6,8f#,8a,e6,8e,8a,d6,8f#,8a,c#6,8c#,8e,b,8d,8g,a,8f#,8d,b,8d,8g,c#.6";
prog_char song_174[] PROGMEM = "National Anthem:d=4,o=5,b=140:g6,g6,a6,f#.6,8g6,a6,b6,b6,c7,b.6,8a6,g6,a6,g6,f#6,g6";
prog_char song_175[] PROGMEM = "Rule B:d=4,o=5,b=100:e.,8e,8f,f,8e,8f.,16e,8d.,16c,2b4,g,f,16e,16c,16f,16d,8g,8f,e,8d.,16c,c";
prog_char song_176[] PROGMEM = "Monty P:d=4,o=5,b=200:f6,8e6,d6,8c#6,c6,8b,a#,8a,8g,8a,8a#,a,8g,2c6,8p,8c6,8a,8p,8a,8a,8g#,8a,8f6,8p,8c6,8c6,8p,8a,8a#,8p,8a#,8a#,8p,8c6,2d6,8p,8a#,8g,8p,8g,8g,8f#,8g,8e6,8p,8d6,8d6,8p,8a#,8a,8p,8a,8a,8p,8a#,2c6,8p,8c6";
prog_char song_177[] PROGMEM = "Zorba2:d=4,o=5,b=125:16c#6,2d6,2p,16c#6,2d6,2p,32e6,32d6,32c#6,2d6,2p,16c#6,2d6,2p,16b,2c6,2p,32d6,32c6,32b,2c6,2p,16a#,2b,p,8p,32c6,32b,32a,32g,32b,2a,2p,32a,32g,32f#,32a,1g,1p,8c#6,8d6,8d6,8d6,8d6,8d6,8d6,8d6,8c#6,8d6,8d6,8d6,8d6,8d6,16e6,16d6,16c#6,16e6,8c#6,8d6,8d6,8d6,8d6,8d6,8d6,8d6,8c#6,8d6,8d6,8d6,8d6,8d6";
prog_char song_178[] PROGMEM = "Auld L S:d=4,o=5,b=100:g,c.6,8c6,c6,e6,d.6,8c6,d6,8e6,8d6,c.6,8c6,e6,g6,2a.6,a6,g.6,8e6,e6,c6,d.6,8c6,d6,8e6,8d6,c.6,8a,a,g,2c.6";
prog_char song_179[] PROGMEM = "Black Bear:d=4,o=5,b=180:d#,d#,8g.,16d#,8a#.,16g,d#,d#,8g.,16d#,8a#.,16g,f,8c.,16b4,c,8f.,16d#,8d.,16d#,8c.,16d,8a#.4,16c,8d.,16a#4,d#,d#,8g.,16d#,8a#.,16g,d#,d#,8g.,16d#,8a#.,16g,f,f,f,8g.,16f,d#,g,2d#";
prog_char song_180[] PROGMEM = "2 Unlimited - No Limits:d=8,o=5,b=180:4e,4e,p,g,g,4e,4e,p,g,g,e,4e,p,g,e,a,a,b,4b,4e,4e,p,g,g,4e,4e,p,g,g,4e,4e,p,g,e,a,4a,4b,4b#";
prog_char song_181[] PROGMEM = "Tubular:d=8,o=5,b=180:e,a,e,b,e,g,a,e,c6,e,d6,e,b,c6,e,b,e,a,e,b,e,g,a,e,c6,e,d6,e,b,c6,e,a,e,b,e,g,a,e,c6,e,d6,e,b,c6,e,b,e,a,e,b,e,g,a,e,c6,e,d6,e,b,c6";
prog_char song_182[] PROGMEM = "PeterGunn:d=4,o=5,b=112:8e,8e,8f#,8e,8g,8e,8a,8g,8e,8e,8f#,8e,8g,8e,8a,8g,1e,c#,2p,p,1e,8c#6,8g,2p";
prog_char song_183[] PROGMEM = "Georgia on my mind:d=4,o=5,b=63:8e,2g.,8p,8e,2d.,8p,p,e,a,e,2d.,8c,8d,e,g,b,a,f,f,8e,e,1c";
prog_char song_184[] PROGMEM = "VanessaMae:d=4,o=6,b=70:32c7,32b,16c7,32g,32p,32g,32p,32d#,32p,32d#,32p,32c,32p,32c,32p,32c7,32b,16c7,32g#,32p,32g#,32p,32f,32p,16f,32c,32p,32c,32p,32c7,32b,16c7,32g,32p,32g,32p,32d#,32p,32d#,32p,32c,32p,32c,32p,32g,32f,32d#,32d,32c,32d,32d#,32c,32d#,32f,16g,8p,16d7,32c7,32d7,32a#,32d7,32a,32d7,32g,32d7,32d7,32p,32d7,32p,32d7,32p,16d7,32c7,32d7,32a#,32d7,32a,32d7,32g,32d7,32d7,32p,32d7,32p,32d7,32p,32g,32f,32d#,32d,32c,32d,32d#,32c,32d#,32f,16c";

PROGMEM const char *songs[] =
{
  song_0,
  song_1,
  song_2,
  song_3,
  song_4,
  song_5,
  song_6,
  song_7,
  song_8,
  song_9,
  song_10,
  song_11,
  song_12,
  song_13,
  song_14,
  song_15,
  song_16,
  song_17,
  song_18,
  song_19,
  song_20,
  song_21,
  song_22,
  song_23,
  song_24,
  song_25,
  song_26,
  song_27,
  song_28,
  song_29,
  song_30,
  song_31,
  song_32,
  song_33,
  song_34,
  song_35,
  song_36,
  song_37,
  song_38,
  song_39,
  song_40,
  song_41,
  song_42,
  song_43,
  song_44,
  song_45,
  song_46,
  song_47,
  song_48,
  song_49,
  song_50,
  song_51,
  song_52,
  song_53,
  song_54,
  song_55,
  song_56,
  song_57,
  song_58,
  song_59,
  song_60,
  song_61,
  song_62,
  song_63,
  song_64,
  song_65,
  song_66,
  song_67,
  song_68,
  song_69,
  song_70,
  song_71,
  song_72,
  song_73,
  song_74,
  song_75,
  song_76,
  song_77,
  song_78,
  song_79,
  song_80,
  song_81,
  song_82,
  song_83,
  song_84,
  song_85,
  song_86,
  song_87,
  song_88,
  song_89,
  song_90,
  song_91,
  song_92,
  song_93,
  song_94,
  song_95,
  song_96,
  song_97,
  song_98,
  song_99,
  song_100,
  song_101,
  song_102,
  song_103,
  song_104,
  song_105,
  song_106,
  song_107,
  song_108,
  song_109,
  song_110,
  song_111,
  song_112,
  song_113,
  song_114,
  song_115,
  song_116,
  song_117,
  song_118,
  song_119,
  song_120,
  song_121,
  song_122,
  song_123,
  song_124,
  song_125,
  song_126,
  song_127,
  song_128,
  song_129,
  song_130,
  song_131,
  song_132,
  song_133,
  song_134,
  song_135,
  song_136,
  song_137,
  song_138,
  song_139,
  song_140,
  song_141,
  song_142,
  song_143,
  song_144,
  song_145,
  song_146,
  song_147,
  song_148,
  song_149,
  song_150,
  song_151,
  song_152,
  song_153,
  song_154,
  song_155,
  song_156,
  song_157,
  song_158,
  song_159,
  song_160,
  song_161,
  song_162,
  song_163,
  song_164,
  song_165,
  song_166,
  song_167,
  song_168,
  song_169,
  song_170,
  song_171,
  song_172,
  song_173,
  song_174,
  song_175,
  song_176,
  song_177,
  song_178,
  song_179,
  song_180,
  song_181,
  song_182,
  song_183,
  song_184,

};


void setup() {
  Serial.begin(115200);

  Serial.println();

  Serial.println("ESP32 Touch Test");
  while(! calibratedTouch(2) );

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LED_ON);

  Serial << "ESP32 deep sleep results:" << endl;
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  Serial.println("ESP32 LED Test");

  // LED setup
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS / 2)
  .setCorrection( TypicalLEDStrip );
  FastLED.addLeds<LED_TYPE, DATA_PIN2, COLOR_ORDER>(leds, NUM_LEDS / 2)
  .setCorrection( TypicalLEDStrip );
  FastLED.setMaxPowerInMilliWatts( MAX_POWER_MILLIWATTS );

  fill_solid(leds, NUM_LEDS, CRGB::Red);
  FastLED.show();
  delay( 2000 );

  fill_solid(leds, NUM_LEDS, CRGB::Green);
  FastLED.show();
  delay( 2000 );

  fill_solid(leds, NUM_LEDS, CRGB::Blue);
  FastLED.show();
  delay( 2000 );

  // buzzer
  pinMode(BUZZER_PIN, OUTPUT);

  // EEPROM
  Serial.println("ESP32 EEPROM Test");
  if (!EEPROM.begin(1000))
    Serial.println("Failed to initialise EEPROM");

  // Should load default config if run for the first time
  Serial.println(F("Loading configuration..."));
  loadConfiguration(config);

  // Dump config file
  Serial.print(F("old config file... "));
  Serial << String(config.hostname) << ":" << config.port << endl;

  // adjust
  config.port++;

  // Create configuration file
  Serial.println(F("Saving configuration..."));
  saveConfiguration(config);

  // Dump config file
  Serial.print(F("Saved config file... "));
  Serial << String(config.hostname) << ":" << config.port << endl;

  // chipid
  uint64_t chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("ESP32 Chip ID = %04X", (uint16_t)(chipid >> 32)); //print High 2 bytes
  Serial.printf("%08X\n", (uint32_t)chipid); //print Low 4bytes.


}

void goToSleep() {

  /*
    First we configure the wake up source
    We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for " + String(TIME_TO_SLEEP) +
                 " seconds.");

  /*
    Next we decide what all peripherals to shut down/keep on
    By default, ESP32 will automatically power down the peripherals
    not needed by the wakeup source, but if you want to be a poweruser
    this is for you. Read in detail at the API docs
    http://esp-idf.readthedocs.io/en/latest/api-reference/system/deep_sleep.html
    Left the line commented as an example of how to configure peripherals.
    The line below turns off all RTC peripherals in deep sleep.
  */
  //esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  //Serial.println("Configured all RTC Peripherals to be powered down in sleep");

  /*
    Now that we have setup a wake cause and if needed setup the
    peripherals state in deep sleep, we can now start going to
    deep sleep.
    In the case that no wake up sources were provided but deep
    sleep was started, it will sleep forever unless hardware
    reset occurs.
  */

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  Serial.println("Going to sleep now");
  Serial.flush();
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop() {

  // LED
  EVERY_N_MILLISECONDS( 20 ) {
    pacifica_loop();
    FastLED.show();
  }

  // buzzer
  static byte songCount = 0;
  const int nSongs = sizeof(songs) / sizeof(songs[0]);

  if ( !rtttl::isPlaying() ) {
    if (songCount > 0) {
      Serial.println("ESP32 Buzzer Test");

      int song = random(0, 100);
      Serial << "Playing song " << song << " of " << nSongs << endl;
      delay(1000);

      rtttl::begin(BUZZER_PIN, songs[song]);
      songCount--; //ready for next song
    }
  }
  else {
    rtttl::play();
  }

  if( calibratedTouch(2) ) {
    if( isPressed(2) ) songCount++; // T3
    if( isPressed(1) ) goToSleep(); // T2  

    if( isPressed(3) || isPressed(0) ) {
      static float mA = 200.0;
      if( isPressed(3) ) mA -= 50; // T5
      if( isPressed(0) ) mA += 50; // T0
      if( mA > 500 ) mA=500;
      if( mA < 0 ) mA=0;

      Serial << "LED mA= " << mA << endl;
      FastLED.setMaxPowerInMilliWatts( 3.3 * mA );
      FastLED.delay(100);
    }
    
//    Serial << "T0:" << isPressed(0) << "\t";
//   Serial << "T2:" << isPressed(1) << "\t";
//    Serial << "T3:" << isPressed(2) << "\t";
//    Serial << "T5:" << isPressed(3) << "\t";
//    Serial << endl;
    
  }

}



//////////////////////////////////////////////////////////////////////////
//
// The code for this animation is more complicated than other examples, and
// while it is "ready to run", and documented in general, it is probably not
// the best starting point for learning.  Nevertheless, it does illustrate some
// useful techniques.
//
//////////////////////////////////////////////////////////////////////////
//
// In this animation, there are four "layers" of waves of light.
//
// Each layer moves independently, and each is scaled separately.
//
// All four wave layers are added together on top of each other, and then
// another filter is applied that adds "whitecaps" of brightness where the
// waves line up with each other more.  Finally, another pass is taken
// over the led array to 'deepen' (dim) the blues and greens.
//
// The speed and scale and motion each layer varies slowly within independent
// hand-chosen ranges, which is why the code has a lot of low-speed 'beatsin8' functions
// with a lot of oddly specific numeric ranges.
//
// These three custom blue-green color palettes were inspired by the colors found in
// the waters off the southern coast of California, https://goo.gl/maps/QQgd97jjHesHZVxQ7
//
CRGBPalette16 pacifica_palette_1 =
{ 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
  0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50
};
CRGBPalette16 pacifica_palette_2 =
{ 0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
  0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F
};
CRGBPalette16 pacifica_palette_3 =
{ 0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33,
  0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF
};


void pacifica_loop()
{
  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011, 10, 13));
  sCIStart2 -= (deltams21 * beatsin88(777, 8, 11));
  sCIStart3 -= (deltams1 * beatsin88(501, 5, 7));
  sCIStart4 -= (deltams2 * beatsin88(257, 4, 6));

  // Clear out the LED array to a dim background blue-green
  fill_solid( leds, NUM_LEDS, CRGB( 2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer( pacifica_palette_1, sCIStart1, beatsin16( 3, 11 * 256, 14 * 256), beatsin8( 10, 70, 130), 0 - beat16( 301) );
  pacifica_one_layer( pacifica_palette_2, sCIStart2, beatsin16( 4,  6 * 256,  9 * 256), beatsin8( 17, 40,  80), beat16( 401) );
  pacifica_one_layer( pacifica_palette_3, sCIStart3, 6 * 256, beatsin8( 9, 10, 38), 0 - beat16(503));
  pacifica_one_layer( pacifica_palette_3, sCIStart4, 5 * 256, beatsin8( 8, 10, 28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();

  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
}

// Add one layer of waves into the led array
void pacifica_one_layer( CRGBPalette16& p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    waveangle += 250;
    uint16_t s16 = sin16( waveangle ) + 32768;
    uint16_t cs = scale16( s16 , wavescale_half ) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16( ci) + 32768;
    uint8_t sindex8 = scale16( sindex16, 240);
    CRGB c = ColorFromPalette( p, sindex8, bri, LINEARBLEND);
    leds[i] += c;
  }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
  uint8_t basethreshold = beatsin8( 9, 55, 65);
  uint8_t wave = beat8( 7 );

  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    uint8_t threshold = scale8( sin8( wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = leds[i].getAverageLight();
    if ( l > threshold) {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8( overage, overage);
      leds[i] += CRGB( overage, overage2, qadd8( overage2, overage2));
    }
  }
}

// Deepen the blues and greens
void pacifica_deepen_colors()
{
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i].blue = scale8( leds[i].blue,  145);
    leds[i].green = scale8( leds[i].green, 200);
    leds[i] |= CRGB( 2, 5, 7);
  }
}


// Loads the configuration from EEPROM
void loadConfiguration(Config &config) {

  // Use arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  EepromStream eepromStream(0, 256);
  deserializeJson(doc, eepromStream);

  int port = doc["port"];
  Serial << port << endl;

  config.port = doc["port"] | 2731;
  strlcpy(config.hostname,                  // <- destination
          doc["hostname"] | "example.com",  // <- source
          sizeof(config.hostname));         // <- destination's capacity

}

// Saves the configuration to a file
void saveConfiguration(const Config &config) {

  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Set the values in the document
  doc["hostname"] = config.hostname;
  doc["port"] = config.port;

  EepromStream eepromStream(0, 256);
  serializeJson(doc, eepromStream);
  eepromStream.flush();  // (for ESP)
}

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}


uint16_t touchReadCensor(uint16_t lastVal, uint8_t pin) {
  uint32_t r = touchRead(pin);
  // getting spurious zeros; dropping
  if ( r < 1 ) r = lastVal;

  // exponential smoother
  const uint32_t weight = 10;
  return (lastVal * (weight - 1) + r) / weight;
}

boolean calibratedTouch(uint32_t foldThrehold) {
  static boolean calibrated = false;
  if ( calibrated ) return ( calibrated );

  static uint32_t touch[N_TOUCH] = {100};

  // lots of averaging
  for ( byte i = 0; i < N_TOUCH; i++ )
    touch[i] = touchReadCensor(touch[i], touchPin[i]);

  static Metro calibrateTime(5000UL);
  if ( calibrateTime.check() ) {
    Serial << "Calibrated Touch.\t";
    Serial << "T0:" << touch[0] << "\t";
    Serial << "T2:" << touch[1] << "\t";
    Serial << "T3:" << touch[2] << "\t";
    Serial << "T5:" << touch[3] << "\t";
    Serial << endl;

    touchAttachInterrupt(touchPin[0], ISR0_Pressed, touch[0] / foldThrehold);
    touchAttachInterrupt(touchPin[1], ISR1_Pressed, touch[1] / foldThrehold);
    touchAttachInterrupt(touchPin[2], ISR2_Pressed, touch[2] / foldThrehold);
    touchAttachInterrupt(touchPin[3], ISR3_Pressed, touch[3] / foldThrehold);

    calibrated = true;
    digitalWrite(BUILTIN_LED, LED_OFF);
  }

  return ( calibrated );
}

// ISRs seem to fire about every 56 ms when the button is held.
boolean isPressed(byte n) {
  const uint32_t changeTime = 100;
  static boolean state[N_TOUCH] = {false};

  // has there been a new press event in the last changeTime milliseconds?
  boolean newPress = (millis() - lastPressed[n]) < changeTime;

  // if yes, we turn on the button; if not, turn it off.
  if ( newPress && state[n] == false ) state[n] = true;
  if ( (!newPress) && state[n] == true ) state[n] = false;

  boolean led = state[0] || state[1] || state[2] || state[3];
  digitalWrite(BUILTIN_LED, led ? LED_ON : LED_OFF);

  return (state[n]);
}


void ISR0_Pressed() {
  lastPressed[0] = millis();
}
void ISR1_Pressed() {
  lastPressed[1] = millis();
}
void ISR2_Pressed() {
  lastPressed[2] = millis();
}
void ISR3_Pressed() {
  lastPressed[3] = millis();
}
