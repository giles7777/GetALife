#include "Sound.h"

// 1. find a MIDI file for the sound you want.

// birds
// https://robertinventor.tripod.com/tunes/birdsong.htm

// 3. convert MIDI To RTTL:
// http://midi.mathewvp.com/midi2RTTL.php
static const char s_coalTit[] = "coal_tit_-:d=4,o=5,b=120:5p,16b8,31d.9,15p,71c9,35p.,333a8,333b8,250c#9,33d.9,19p,111f#9,250d9,63c9,34p.,333b8,67c#9,29d9,16p,167c9,91b8,31p.,333c9,29d.9,16p,333d9,200c#9,42c9,28p,333b8,200c9,33d.9,16p,250c#9,59b8,31p.,333b8,125c#9,33d.9,29p.,77g9,333c#9,63b8";
static const char s_tawnyOwl[] = "tawny_owl_:d=4,o=5,b=120:p,p,1000g,17p,40d7,500p,37g8,250p,1000f,125p,1000g,111p,500f,1p,48g#6,333g6,500f#6,250p,100g#6,500f6,111g6,333p,83g6,p,43g#6,31g6,333p,200f#6,1000g6,333f#6,333p,167p,125f#6,5p,83g6,50f#6,15g.6,43g#6,25f#6,63a6,200g6,91f#6,143g#6,12a6,111g6,33g6,33f#.6,17f#6,71f6,200g6";
static const char s_nuthatch[] = "nuthatch_-:d=4,o=5,b=120:12p,143c#8,83d8,91d#8,143d#8,83e8,100e8,167f8,56g8,5p,125c#8,143d8,59d#8,167d#8,77e8,53f8,111f8,500f#8,26g8,9p.,77f#7,167c#8,50d8,71d#8,91e8,167e8,250f8,250f8,200f#8,125f8,37f#8,6p,100c#8,125d8,125d#8,83d#8,250e8,71e8,167f8,200f8,200f#8,111g8,45g8,63g#8,p,111c#8,100d8,77d8,111d#8,250e8,250e8,143f8,143f8,77f#8,45f#8";
// wrap in {sound:{"song":""}}

// many more in the .zip files
static const char s_morningTrain[] = "Morningt:d=8,o=6,b=140:16a#,16p,4c#,d#,p,f,p,f#,p,g#,16p,f#,16p,a#,16p,a#,p,d#,16p,f,p,f#,p,g#,p,a#,p,g,16p,16a#,4p,a#,p,d#,p,16f,16p,f#,p,g#,p,a#,p,g#,16p,16a#,4p,a#,p,4a#,a#,p,a,16p,g#,p,f#,16p,g#";
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

  this->setSong("chirp:d=3000,o=2,b=180:a,e,a,e,a,e,a,e,a,e,a,e,a,e,a,e");
  this->setFrequency();
  this->setCount();
}

String Sound::getStatus() {
  DynamicJsonDocument doc(2048);
  JsonObject s = doc.createNestedObject("sound");

  s["song"] = this->song;
  s["frequency"] = this->songsPerMinute;
  s["count"] = this->songCount;
  
  String ret;
  serializeJson(doc, ret);
  return (ret);
}

void Sound::setStatus(String & msg) {
  DynamicJsonDocument doc(2048);
  if(doc.capacity() == 0) {
    Serial << "Sound::setStatus() out of memory." << endl;
    return;
  }
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial << "Sound::setStatus() error: " << err.c_str();
    Serial << " status: " << this->getStatus() << endl;
    return;
  } else {
    Serial << "Sound::setStatus() memory usage " << doc.memoryUsage() << endl;
  }
  
  JsonObject s = doc["sound"];

  if ( ! s["song"].isNull() ) this->setSong( s["song"].as<String>() );
  if ( ! s["frequency"].isNull() ) this->setFrequency( s["frequency"].as<uint32_t>() );
  if ( ! s["count"].isNull() ) this->setCount( s["count"].as<uint32_t>() );
}

void Sound::setSong(String song) {
  Serial << "Sound::setSong() " << song << endl;
  this->song = song;
}
void Sound::setFrequency(uint32_t songsPerMinute) {
  Serial << "Sound::setFrequency() " << songsPerMinute << endl;
  this->songsPerMinute = songsPerMinute;
}
void Sound::setCount(uint32_t songCount) {
  Serial << "Sound::setCount() " << songCount << endl;
  this->songCount = songCount;
}

void Sound::update() {
  // if we're already playing, continue to do so
  if( !rtttl::done() ) {
    rtttl::play();
    return;  
  }

  /// track the interval
  static Metro playInterval(1);
  
  // maybe start another song?
  if( playInterval.check() & this->songCount > 0 ) {
    rtttl::begin(PIN_BUZZER, this->song.c_str());
    this->songCount --;

    if( this->songsPerMinute > 0 ) {
      float newInterval = 60000.0 / (float)this->songsPerMinute; // the average song interval.
      newInterval = newInterval * (float)random(50,150+1) / 100.0;
      Serial << "Sound::update() new interval " << (uint32_t)newInterval << endl;
      playInterval.interval((uint32_t)newInterval);
    }

    // reset the timer so we get an immediate play for the next song requested.
    if( songCount == 0 ) playInterval.interval(1);
  }

}
