
// https://arduino-esp8266.readthedocs.io/en/latest/PROGMEM.html#how-do-i-declare-arrays-of-strings-in-progmem-and-retrieve-an-element-from-it
const char s_chirp[] PROGMEM = "chirp:d=3000,o=2,b=180:a,e,a,e,a,e,a,e,a,e,a,e,a,e,a,e";

// many more in the .zip files
const char s_morningTrain[] PROGMEM = "Morningt:d=8,o=6,b=140:16a#,16p,4c#,d#,p,f,p,f#,p,g#,16p,f#,16p,a#,16p,a#,p,d#,16p,f,p,f#,p,g#,p,a#,p,g,16p,16a#,4p,a#,p,d#,p,16f,16p,f#,p,g#,p,a#,p,g#,16p,16a#,4p,a#,p,4a#,a#,p,a,16p,g#,p,f#,16p,g#";
const char s_boot[] PROGMEM = "boot:d=10,o=6,b=180:c,e,g";
const char s_AxelF[] PROGMEM = "Axel-F:d=4,o=5,b=125:g,8a#.,16g,16p,16g,8c6,8g,8f,g,8d.6,16g,16p,16g,8d#6,8d6,8a#,8g,8d6,8g6,16g,16f,16p,16f,8d,8a#,2g,p,16f6,8d6,8c6,8a#,g,8a#.,16g,16p,16g,8c6,8g,8f,g,8d.6,16g,16p,16g,8d#6,8d6,8a#,8g,8d6,8g6,16g,16f,16p,16f,8d,8a#,2g";
const char s_RickRoll[] PROGMEM = "Together:d=8,o=5,b=225:4d#,f.,c#.,c.6,4a#.,4g.,f.,d#.,c.,4a#,2g#,4d#,f.,c#.,c.6,2a#,g.,f.,1d#.,d#.,4f,c#.,c.6,2a#,4g.,4f,d#.,f.,g.,4g#.,g#.,4a#,c.6,4c#6,4c6,4a#,4g.,4g#,4a#,2g#";
const char s_CrazyTrain[] PROGMEM = "CrazyTra:d=4,o=5,b=355:g.,g.,d.6,g.,d#.6,g.,d.6,g.,c.6,a#.,a.,a#.,c.6,a#.,a.,f.,g.,g.,d.6,g.,d#.6,g.,d.6,g.,c.6,a#.,a.,a#.,c.6,a#.,a.,f.,g.,g.,d.6,g.,d#.6,g.,d.6,g.,c.6,a#.,a.,a#.,c.6,a#.,a.,f.,g.,g.,d.6,g.,d#.6,g.,d.6,g.,1d#.6,1f.6";

// See Gottman's "The Feeling Wheel" in *notes*
const char e_Joyful[] PROGMEM = "TakeOnMe:d=4,o=4,b=160:8f#5,8f#5,8f#5,8d5,8p,8b,8p,8e5,8p";
const char e_Powerful[] PROGMEM = "OdeToJoy:d=4,o=6,b=100:c,c,a_5,a5,g5,f5";
const char e_Peaceful[] PROGMEM = "GivePeac:d=8,o=6,b=90:g5,f,16p,e,16p,d,2c,4p";
const char e_Sad[] PROGMEM = "MusicOfT:d=8,o=5,b=100:4a#,4c#,4g#,4c#,f#,g#,a#";
const char e_Mad[] PROGMEM = "Revolver:d=4,o=5,b=100:16a#6,16a#6,16c#7,16c#6,8d#6,8f#6,8a#6";
const char e_Scared[] PROGMEM = "GodIsAGa:d=4,o=6,b=75:8b,8b,8b,8b,8c_7";

// Initialize Table of Strings
const char* const song_table[] PROGMEM = { 
    s_chirp, 
    s_morningTrain, s_boot, s_AxelF, s_RickRoll, s_CrazyTrain,
    e_Joyful, e_Powerful, e_Peaceful, e_Sad, e_Mad, e_Scared
};
