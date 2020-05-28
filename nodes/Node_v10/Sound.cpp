#include "Sound.h"

// many more in the .zip files
static const char s_morningTrain[]="Morningt:d=8,o=6,b=140:16a#,16p,4c#,d#,p,f,p,f#,p,g#,16p,f#,16p,a#,16p,a#,p,d#,16p,f,p,f#,p,g#,p,a#,p,g,16p,16a#,4p,a#,p,d#,p,16f,16p,f#,p,g#,p,a#,p,g#,16p,16a#,4p,a#,p,4a#,a#,p,a,16p,g#,p,f#,16p,g#";
static const char s_boot[] = "boot:d=10,o=6,b=180:c,e,g";
static const char s_AxelF[] = "Axel-F:d=4,o=5,b=125:g,8a#.,16g,16p,16g,8c6,8g,8f,g,8d.6,16g,16p,16g,8d#6,8d6,8a#,8g,8d6,8g6,16g,16f,16p,16f,8d,8a#,2g,p,16f6,8d6,8c6,8a#,g,8a#.,16g,16p,16g,8c6,8g,8f,g,8d.6,16g,16p,16g,8d#6,8d6,8a#,8g,8d6,8g6,16g,16f,16p,16f,8d,8a#,2g";
static const char s_RickRoll[] = "Together:d=8,o=5,b=225:4d#,f.,c#.,c.6,4a#.,4g.,f.,d#.,c.,4a#,2g#,4d#,f.,c#.,c.6,2a#,g.,f.,1d#.,d#.,4f,c#.,c.6,2a#,4g.,4f,d#.,f.,g.,4g#.,g#.,4a#,c.6,4c#6,4c6,4a#,4g.,4g#,4a#,2g#";
static const char s_CrazyTrain[] = "CrazyTra:d=4,o=5,b=355:g.,g.,d.6,g.,d#.6,g.,d.6,g.,c.6,a#.,a.,a#.,c.6,a#.,a.,f.,g.,g.,d.6,g.,d#.6,g.,d.6,g.,c.6,a#.,a.,a#.,c.6,a#.,a.,f.,g.,g.,d.6,g.,d#.6,g.,d.6,g.,c.6,a#.,a.,a#.,c.6,a#.,a.,f.,g.,g.,d.6,g.,d#.6,g.,d.6,g.,1d#.6,1f.6";

// See Gottman's "The Feeling Wheel" in *notes*
static const char e_Joyful[] = "TakeOnMe:d=4,o=4,b=160:8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p";
static const char e_Powerful[] = "OdeToJoy:d=4,o=6,b=100:c,c,a_5,a5,g5,f5";
static const char e_Peaceful[] = "GivePeac:d=8,o=6,b=90:g5,f,16p,e,16p,d,2c,4p";
static const char e_Sad[] = "MusicOfT:d=8,o=5,b=100:4a#,4c#,4g#,4c#,f#,g#,a#";
static const char e_Mad[] = "Revolver:d=4,o=5,b=100:16a#6,16a#6,16c#7,16c#6,8d#6,8f#6,8a#6";
static const char e_Scared[] = "GodIsAGa:d=4,o=6,b=75:8b,8b,8b,8b,8c_7";

void Sound::begin() {
  Serial << "Sound::begin()" << endl;

  // start
  pinMode(PIN_BUZZER, OUTPUT);
  this->play(boot);
}

void Sound::play(songList song, boolean blocking) {

  switch(song) {
    case boot: rtttl::begin(PIN_BUZZER, s_boot); break;

    case morningTrain: rtttl::begin(PIN_BUZZER, s_morningTrain); break;
    case AxelF: rtttl::begin(PIN_BUZZER, s_AxelF); break;
    case RickRoll: rtttl::begin(PIN_BUZZER, s_RickRoll); break;
    case CrazyTrain: rtttl::begin(PIN_BUZZER, s_CrazyTrain); break;

    case Joyful: rtttl::begin(PIN_BUZZER, e_Joyful); break;
    case Powerful: rtttl::begin(PIN_BUZZER, e_Powerful); break;
    case Peaceful: rtttl::begin(PIN_BUZZER, e_Peaceful); break;
    case Sad: rtttl::begin(PIN_BUZZER, e_Sad); break;
    case Mad: rtttl::begin(PIN_BUZZER, e_Mad); break;
    case Scared: rtttl::begin(PIN_BUZZER, e_Scared); break;

    default: rtttl::begin(PIN_BUZZER, s_boot); break;
  }
  
  if( blocking ) {
    while( !rtttl::done() ) {
      rtttl::play();
      yield();
    }
  }
}

boolean Sound::update() {
  boolean playing = !rtttl::done();
  if( playing ) rtttl::play();
  return( playing );
}
