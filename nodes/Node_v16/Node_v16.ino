#include <MeshGnome.h>
#include <Arduino.h> // basics
#include <ESP8266WiFi.h> // wifi 
#include <espnow.h> // esp-now
#include <Streaming.h> // ease outputs
#include <FastLED.h> // lights
#include <map>
#include <Schedule.h>

// Animation/Lighting modes
#define SHOW_TRANSMIT true  // Show debug colors when updating code

#define LED_OFF HIGH
#define LED_ON LOW

// wifi
#define WIFI_CHAN     1
#define MAC_SIZE      6

// lights
// https://www.wemos.cc/en/latest/d1_mini_shiled/rgb_led.html
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
#define NUM_LEDS      7
#define PIN_RGB       D7  // NOT default (which is D4 aka BUILTIN_LED; doh!)
// builtin
#define LED_OFF HIGH
#define LED_ON LOW

struct Config {
  uint16_t gameSpeed = 2000;  // cmd t
  uint8_t podSize = 8;
  uint8_t minAlive = 2;
  uint8_t maxAlive = 3;
  uint8_t resurrectChance = 50;  // cmd r
  uint8_t gameType = 1;  // cmd g
  uint8_t palette = 12;  // cmd p
  boolean showGameState = false;  // cmd d - debug
};

struct EthStruct {
  uint8_t ethaddr[6];

  bool operator<(const EthStruct& rhs) const { return memcmp(ethaddr, rhs.ethaddr, 6) < 0; }
  bool operator==(const EthStruct& rhs) const { return memcmp(ethaddr, rhs.ethaddr, 6) == 0; }
};

struct LocalShareData {
  uint8_t state = 0;
  uint8_t colorIndex = 0;
};

struct Neighbor {
    uint8_t address[MAC_SIZE];
    int16_t rssi = 0;
    uint8_t state = 0;
    uint8_t colorIndex = 0;
};


MeshSyncStruct<Config> gameConfig;
MeshSyncSketch sketchUpdate(342); // Update these each code change to propigate

MeshSyncTime syncedTime;
LocalPeriodicStruct<LocalShareData> shareLocal;
DispatchProto protos[] = {  //
  {0 /* protocol id */, &sketchUpdate},
  {1 /* protocol id */, &syncedTime},
  {2 /* protocol id */, &gameConfig},
  {3 /* protocol id */, &shareLocal},
  };

char addrCharBuff[] = "00:00:00:00:00:00 \0";
uint16_t buff;

CRGB leds[NUM_LEDS];
bool updatingCode = false;
bool transmittingCode = false;
size_t transmitOffset = 0;
size_t transmitLength = 1;
size_t lastTransmitOffset = 0;
size_t updateOffset = 0;
size_t updateLength = 0;
uint16_t lastGameSpeed = 0;
uint8_t lastPalette = 0;


// local state
std::map<EthStruct, Neighbor> neighbors;
uint8_t state = 0;
uint8_t colorIndex = random(255);
uint8_t destIndex = 0;
uint16_t fraction = 0;


CRGBPalette16 currentPalette;

#define PALETTES      16


// Palletes
// Gradient palette "girlcat_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/rc/tn/girlcat.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( girlcat_gp ) {  //  Yellow to green
    0, 173,229, 51,
   73, 139,199, 45,
  140,  46,176, 37,
  204,   7, 88,  5,
  255,   0, 24,  0};

// Gradient palette "bhw1_05_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw1/tn/bhw1_05.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 8 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw1_05_gp ) { // Green to purple
    0,   1,221, 53,
  255,  73,  3,178};

// Gradient palette "bhw1_sunconure_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw1/tn/bhw1_sunconure.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw1_sunconure_gp ) {
    0,  20,223, 13,
  160, 232, 65,  1,
  252, 232,  5,  1,
  255, 232,  5,  1};

 // Gradient palette "Sunset_Real_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/atmospheric/tn/Sunset_Real.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 28 bytes of program space.

DEFINE_GRADIENT_PALETTE( Sunset_Real_gp ) {
    0, 120,  0,  0,
   22, 179, 22,  0,
   51, 255,104,  0,
   85, 167, 22, 18,
  135, 100,  0,103,
  198,  16,  0,130,
  255,   0,  0,160};

  // Gradient palette "jet_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/h5/tn/jet.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 64 bytes of program space.

DEFINE_GRADIENT_PALETTE( jet_gp ) {
    0,   0,  0,123,
   17,   0,  0,255,
   33,   0, 11,255,
   51,   0, 55,255,
   68,   0,135,255,
   84,   0,255,255,
  102,   6,255,255,
  119,  41,255,123,
  135, 120,255, 44,
  153, 255,255,  7,
  170, 255,255,  0,
  186, 255,135,  0,
  204, 255, 55,  0,
  221, 255, 11,  0,
  237, 255,  0,  0,
  255, 120,  0,  0};

 // Gradient palette "spellbound_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/rc/tn/spellbound.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 84 bytes of program space.

DEFINE_GRADIENT_PALETTE( spellbound_gp ) {
    0, 232,235, 40,
   12, 157,248, 46,
   25, 100,246, 51,
   45,  53,250, 33,
   63,  18,237, 53,
   81,  11,211,162,
   94,  18,147,214,
  101,  43,124,237,
  112,  49, 75,247,
  127,  49, 75,247,
  140,  92,107,247,
  150, 120,127,250,
  163, 130,138,252,
  173, 144,131,252,
  186, 148,112,252,
  196, 144, 37,176,
  211, 113, 18, 87,
  221, 163, 33, 53,
  234, 255,101, 78,
  247, 229,235, 46,
  255, 229,235, 46};

// Gradient palette "bhw1_28_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/bhw/bhw1/tn/bhw1_28.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 32 bytes of program space.

DEFINE_GRADIENT_PALETTE( bhw1_28_gp ) {
    0,  75,  1,221,
   30, 252, 73,255,
   48, 169,  0,242,
  119,   0,149,242,
  170,  43,  0,242,
  206, 252, 73,255,
  232,  78, 12,214,
  255,   0,149,242};

// Gradient palette "Blue_Cyan_Yellow_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/nd/basic/tn/Blue_Cyan_Yellow.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 20 bytes of program space.

DEFINE_GRADIENT_PALETTE( Blue_Cyan_Yellow_gp ) {
    0,   0,  0,255,
   63,   0, 55,255,
  127,   0,255,255,
  191,  42,255, 45,
  255, 255,255,  0};

// Gradient palette "purplefly_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/rc/tn/purplefly.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 16 bytes of program space.

DEFINE_GRADIENT_PALETTE( purplefly_gp ) {
    0,   0,  0,  0,
   63, 239,  0,122,
  191, 252,255, 78,
  255,   0,  0,  0};


// If true, set LED_BUILTIN to LOW during blinks; otherwise set it to HIGH.
static constexpr bool ON_IS_LOW = true;

void builtinOff() {
  digitalWrite(BUILTIN_LED, LED_OFF);
}

void builtinOn() {
  digitalWrite(BUILTIN_LED, LED_ON);
}

void blinkBuiltin() {
  static boolean builtInLED = false;
  builtInLED = ! builtInLED;
  builtInLED ? builtinOn() : builtinOff();
}

// helpers
void mac2str(const uint8_t* ptr, char* string) {
  sprintf(string, "%02X:%02X:%02X:%02X:%02X:%02X", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
}

int compareRssi(Neighbor a, Neighbor b) {
  return a.rssi < b.rssi;
}


void showNeighbor(Neighbor a) {
  mac2str(a.address, addrCharBuff);
  Serial << "Node: " << addrCharBuff << " = " << a.state << " @ " << a.rssi << endl;
}

void showNeighbors(uint16_t n) {
  n = n > neighbors.size() ? neighbors.size() : n;
  Serial << "Neighbors: " << n << " of " << neighbors.size() << endl;

  auto it = neighbors.begin();
  uint16_t cnt = 0;
  for ( auto it = neighbors.begin(); it != neighbors.end(); ++it ) {
    Neighbor node = it->second;
    Serial << "\t";
    showNeighbor(node);
    
    cnt++;
    if (cnt >= n) break; 
  }
}

void rssiHandler(const uint8_t* src, int8_t rssi) {
  // Don't update neighbors during code update
  if (updatingCode || transmittingCode) {
    return;
  }

  static unsigned long lastUpdate = millis();

  if (millis() - lastUpdate < 250) {
    return;
  }
  lastUpdate = millis();

  schedule_function([=](){

    EthStruct key;
    memcpy(key.ethaddr, src, MAC_SIZE);

    Neighbor node = neighbors[key];
    memcpy(node.address, src, MAC_SIZE);
    const int32_t smooth = 3;
    int32_t newRSSI = ((int32_t)node.rssi * smooth + rssi) / (smooth + 1);
    mac2str(node.address, addrCharBuff);
    //Serial.printf("Got rssi: addr: %s wire: %d  old: %d new: %d \n", addrCharBuff, rssi, node.rssi, newRSSI);
    node.rssi = newRSSI;
    neighbors[key] = node;

  });
}

void transmitProgressHandler(size_t offset, size_t length) {
  if (length > 0) {
    if (offset >= length) {
      if (transmittingCode) {
        Serial.printf("Ending transmit code\n");
      }
      transmittingCode = false;
    } else {
      if (!transmittingCode) {
        Serial.printf("Starting transmit code\n");
      }
      transmittingCode = true;
    }
  }
  transmitOffset = offset;
  transmitLength = length;
}

void testLights() {
  if (updatingCode || transmittingCode) return;

  static uint8_t idx = 0;

  if (idx % 2 == 0) {
    for (byte i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Blue;
    FastLED.show();
  } else {
    for (byte i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
    FastLED.show();    
  }
  idx++;
}

void setPalette(uint8_t pattern) {
  if (pattern > PALETTES - 1) {
    pattern = PALETTES - 1;
  }
  
  switch (pattern) {
    case 0:   currentPalette = LavaColors_p;          break;  // orange, red, black and yellow), 
    case 1:   currentPalette = CloudColors_p;         break;  // blue and white
    case 2:   currentPalette = OceanColors_p;         break;  // blue, cyan and white
    case 3:   currentPalette = ForestColors_p;        break;  // greens and blues
    case 4:   currentPalette = RainbowColors_p;       break;  // standard rainbow animation
    case 5:   currentPalette = PartyColors_p;         break;  // red, yellow, orange, purple and blue
    case 6:   currentPalette = HeatColors_p;          break;  // red, orange, yellow and white
    case 7:   currentPalette = girlcat_gp;            break;  // Yellow to green
    case 8:   currentPalette = bhw1_05_gp;            break;  // Green to purple
    case 9:   currentPalette = bhw1_sunconure_gp;     break;  // Green to red
    case 10:  currentPalette = Sunset_Real_gp;        break;  // Real atmospheric sunset colors
    case 11:  currentPalette = jet_gp;                break;  // Blue to red, 15 segments
    case 12:  currentPalette = spellbound_gp;         break;  // Spellbound paisley ramp
    case 13:  currentPalette = bhw1_28_gp;            break;  // Purple blue waves
    case 14:  currentPalette = Blue_Cyan_Yellow_gp;   break;  // Blue Cyan Yellow 
    case 15:  currentPalette = purplefly_gp;          break;  // Purplefly purple to yellow
  }
}

void nextGeneration() {
  if (updatingCode || transmittingCode) return;

  //Serial.printf("Next gen:\n");
  //showNeighbors(8);

  // Age out neighbors
  EthStruct key;
  std::vector<EthStruct> to_remove;
  
  for (auto itr = neighbors.begin(); itr != neighbors.end(); ++itr) {
    Neighbor node = itr->second;
    node.rssi -= 1;
    memcpy(key.ethaddr, node.address, 6);

    neighbors[key] = node;

    if (node.rssi < -128) {
      to_remove.push_back(key);    
    }
  }
  
  for(auto it = to_remove.begin(); it != to_remove.end(); it++ ) {
    //printf("Removing neighbor\n");
    neighbors.erase(*it);
  }

  std::vector<Neighbor> sorted;
  for (auto itr = neighbors.begin(); itr != neighbors.end(); ++itr)
    sorted.push_back(itr->second);

  std::sort (sorted.begin(), sorted.end(), compareRssi); 

  if (gameConfig->gameType == 0) {
    if (neighbors.size() < 2) return;

    Neighbor left = sorted.at(0);
    Neighbor right = sorted.at(1);
  
    byte currentPattern = 0;
    bitWrite(currentPattern, 2, left.state); // am i write or right?
    bitWrite(currentPattern, 1, state);
    bitWrite(currentPattern, 0, right.state);
    boolean newState = bitRead(101, currentPattern); // rule 101, usually.
  
    state = newState;
  
    static byte deadCount = 0;
    if ( state == false ) deadCount++;
    else deadCount = 0;

    long roll = random(255);
    if (roll < gameConfig->resurrectChance) {
      state = 1;
    }
  
    if (state == 0) {
      Serial.printf("Dead\n");
    } else {
      Serial.printf("Alive\n");
    }

    if (state != newState) {
        state = newState;
        fraction = 0;
      }
  } else if (gameConfig->gameType == 1) {
      int aliveNeighbors = 0;
      uint8_t newState = 0;
      boolean colorSet = false;
      int16_t firstColorIndex = random(255);
      int16_t newColorIndex = colorIndex;

      // Min seems to be declared in vector library, I want math...
      uint8_t len = sorted.size();
      if (gameConfig->podSize < len) 
        len = gameConfig->podSize;

      uint16_t color = 0;
      for (int i=0; i < len; i++) {
        Neighbor n = sorted.at(i);
        if (n.state == 1) {
          aliveNeighbors++;
          if (!colorSet && colorIndex != n.colorIndex) {
            colorSet = true;
            firstColorIndex = n.colorIndex;
          }
        }
      }
      //Serial.printf("Neighbors: %d  alive: %d colorIndex: %d \n",neighbors.size(),aliveNeighbors, newColorIndex);
      if (aliveNeighbors >= gameConfig->minAlive && aliveNeighbors <= gameConfig->maxAlive) {
        newState = 1;
        newColorIndex = firstColorIndex;
      }

      //colorIndex = newColorIndex;
      
      static byte deadCount = 0;
      if ( newState == 0 ) deadCount++;
      else deadCount = 0;
  
      long roll = random(255);
      if (!newState && roll < gameConfig->resurrectChance) {
        newState = 1;
        newColorIndex = random(0,255);
        Serial.printf("Ressurrection  roll: %d  color: %d\n",roll,newColorIndex);
      } else {
        newState = 0;
      }

      if (newState == 0) {        
      } else {
        Serial.printf("I'm alive!  color: %d\n",newColorIndex);

        destIndex = newColorIndex;
      }

      if (state != newState) {
        Serial.printf("State changed:  old: %d  new: %d\n",state,newState);
        state = newState;
        fraction = 0;
      }

  }
}

void printParams() {
  Serial.printf("ShowGameState: %d GameSpeed: %d  resurrectChance: %d  gameType: %d  pallete: %d\n", gameConfig->showGameState, gameConfig->gameSpeed, gameConfig->resurrectChance, gameConfig->gameType, gameConfig->palette);
}


void setup() {
  delay(300);
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  EspSnifferProtoDispatch.addProtocol(0, &sketchUpdate);
  EspSnifferProtoDispatch.begin();
  Serial.printf("Sketch version %d checking for failsafe upgrade\n", sketchUpdate.localVersion());
  while (!sketchUpdate.upToDate()) {
    EspSnifferProtoDispatch.espTransmitIfNeeded();
    yield();
  }
  Serial.println("Failsafe check complete; no new version immediately available.");
  EspSnifferProtoDispatch.addProtocol(1, &syncedTime);
  EspSnifferProtoDispatch.addProtocol(2, &gameConfig);
  EspSnifferProtoDispatch.addProtocol(3, &shareLocal);

  Serial.printf("Node startup complete, running sketch version %d\n",
                sketchUpdate.localVersion());
                
  // lights
  FastLED.addLeds<LED_TYPE, PIN_RGB, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  EspSnifferProtoDispatch.setRSSIHook(rssiHandler);
  sketchUpdate.setTransmitProgressHook(transmitProgressHandler);


  sketchUpdate.setReceiveProgressHook([](size_t offset, size_t length) {
    updateOffset = offset;
    updateLength = length;

    //Serial.printf("Got progress: offset: %d len: %d\n",updateOffset,updateLength);

    if (updateOffset < updateLength) {
      if (!updatingCode) {
        Serial.printf("Starting update code\n");
      }
      updatingCode = true;
    } else {
      if (updatingCode) {
         Serial.printf("Ending update code\n");
      }
      updatingCode = false;
    }
  });

  sketchUpdate.setUpdateStopHook([](String reason) {
    Serial << "Stopped updating code: " << reason << endl;
    updateLength = 0;
    updatingCode = false;
  });
  
  for (byte i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
  FastLED.show();

  shareLocal.setTimestepHook([]() {
    if (updatingCode) {
      Serial.printf("Receiving: %d / %d\n", updateOffset, updateLength);
      byte show = 1 + (updateOffset * (NUM_LEDS - 1) / updateLength);

      if (SHOW_TRANSMIT) {
        for (byte i = 0; i < show; i++) leds[i] = CRGB::Red;
        for (byte i = show; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
        FastLED.show();
      }
    } else if (transmitLength > 0 && transmittingCode) {
      Serial.printf("Transmitting: %d / %d  last: %d\n", transmitOffset, transmitLength,  lastTransmitOffset);
      byte show = 1 + (transmitOffset * (NUM_LEDS - 1) / transmitLength);

      if (SHOW_TRANSMIT) {
        for (byte i = 0; i < show; i++) leds[i] = CRGB::Blue;
        for (byte i = show; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
        FastLED.show();
      }
      transmitOffset = transmitLength = 0;
    } else {
      //testLights();
      nextGeneration();
    }
  });
  
  shareLocal.setReceivedHook([](const ProtoDispatchPktHdr* hdr, const LocalShareData& val) {
    EthStruct key;
    memcpy(key.ethaddr, hdr->src, 6);

    Neighbor node = neighbors[key];
    memcpy(node.address, hdr->src, MAC_SIZE);
    node.state = val.state;
    node.colorIndex = val.colorIndex;
    neighbors[key] = node;
  });
  
  shareLocal.setFillForTransmitHook([](LocalShareData* val) -> bool {
    val->state = state;
    val->colorIndex = colorIndex;
    return true;
  });
  shareLocal.begin(&syncedTime, gameConfig->gameSpeed);

/*
  syncedTime.setAdjustHook([](int32_t adjustment) {
    //schedule_function([=]() { Serial.printf("Time adjusted: %d\n", adjustment); });
  });
  syncedTime.setJumpHook([](int32_t adjustment) {
    //schedule_function([=]() { Serial.printf("Time JUMPED: %d\n", adjustment); });
  });

  syncedTime.setReceiveHook(
      [](const ProtoDispatchPktHdr* hdr, int32_t offset, uint32_t syncedDuration) {
        String peer = etherToString(hdr->src);
        schedule_function([=]() {
          //Serial.printf("Time of %s offset=%d duration=%u\n", peer.c_str(), offset, syncedDuration);
        });
  });
  */

  setPalette(gameConfig->palette);

  Serial.printf("Commands:  d - debug(0-1).  t - gameSpeed(0-1).  r - resurrectChance(0-255).  g - gameType(0-1).  p - palette(0-16)\n");
  printParams();
}

void loop() {
  EspSnifferProtoDispatch.espTransmitIfNeeded();
  
  EVERY_N_MILLISECONDS( 30000 ) {
    if (transmittingCode && (lastTransmitOffset == transmitOffset)) {
      Serial.printf("*** Stalled on transmit, reset transmitting code  last: %d  current: %d\n ***",lastTransmitOffset,transmitOffset);
      transmittingCode = false;
    }
    lastTransmitOffset = transmitOffset;
  }

  if (lastGameSpeed != gameConfig->gameSpeed) {
    Serial.printf("New game speed: %d\n", gameConfig->gameSpeed);
    lastGameSpeed = gameConfig->gameSpeed;
    shareLocal.begin(&syncedTime, gameConfig->gameSpeed); 
  }
  if (lastPalette != gameConfig->palette) {
    Serial.printf("New palette: %d\n", gameConfig->palette);
    setPalette(gameConfig->palette);
    lastPalette = gameConfig->palette;
  }
  shareLocal.run();

  const uint16_t animTime = 100;

  // Local animations
  EVERY_N_MILLISECONDS( animTime ) {
    if (updatingCode || transmittingCode) {
      return;
    }

    
  schedule_function([=](){
    if (updatingCode || transmittingCode) {
      return;
    }
    if (state == 0) {
      if (gameConfig->showGameState) {
        for (byte i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB::Red;
        }

        FastLED.show();
      } else {    
        uint8_t oldFraction = fraction;
            
        if (fraction > 255) {
          fraction = 255;
        } else {
          uint8_t jump = (uint8_t) ((float) animTime / gameConfig->gameSpeed * 255);
          fraction += jump;
          if (fraction > 255) {
            fraction = 255;
          }
        }

        // Lower lights.  Keep some brightness to avoid center light being glitchy
        uint8_t bright = 255 - fraction;
        if (bright < 0) {
          bright = 0;
        }
        if (oldFraction != fraction) {
          if (bright == 0) {
            for (byte i = 0; i < NUM_LEDS; i++) {
              leds[i] = CRGB::Black;
      
            }      
            FastLED.show();
          } else {
            Serial.printf("dead colorIndex: %d  bright: %d\n",colorIndex,bright);
            CRGB color = ColorFromPalette( currentPalette, colorIndex, bright, LINEARBLEND);    
    
            for (byte i = 0; i < NUM_LEDS; i++) {
              leds[i] = color;
      
            }      
            FastLED.show();
          }
        }
      }
  } else {
      if (gameConfig->showGameState) {
        for (byte i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB::Blue;
        }

        FastLED.show();
      } else {
        if (fraction >= 255) {
          fraction = 255;
        } else {
          uint8_t jump = (uint8_t) ((float) animTime / gameConfig->gameSpeed * 255);
          fraction += jump;
          Serial.printf("Jump: %d  fraction: %d\n", jump,fraction);
          if (fraction > 255) fraction = 255;
        }
    
        float frac = ((float)fraction / 255);
        
        int16_t idx = colorIndex +  (destIndex - colorIndex) * frac;
        uint8_t c_idx = 0;
        if (idx >= 255) {
          c_idx = 255;
        }
        if (idx < 0) {
          c_idx = 0;
        }
        else c_idx = idx;

        if (fraction > 250) {
          colorIndex = destIndex;
        }
        Serial.printf("colorIndex: %d  destIndex: %d  idx: %d  frac: %d  leds: %d\n",colorIndex,destIndex,c_idx,fraction,NUM_LEDS);
        CRGB color = ColorFromPalette( currentPalette, c_idx, fraction, LINEARBLEND);    
        for (byte i = 0; i < NUM_LEDS; i++) {
          leds[i] = color;
        }
        FastLED.show();
      }
    }
  });
  }

  if (Serial.available()) {
    int cmd = Serial.read();
    if (cmd == 't') {
      int newGameSpeed = Serial.parseInt();
      if (newGameSpeed < 33) {
        Serial.printf("Invalid gameSpeed < 33\n");
        return;
      }
  
      uint16_t oldGameSpeed = gameConfig->gameSpeed;
      int oldVersion = gameConfig.localVersion();
      gameConfig->gameSpeed = newGameSpeed;
      gameConfig.push();
      Serial.printf("Ok, gameSpeed %d -> %d, configuration version=%d -> %d\n", oldGameSpeed, newGameSpeed,
                    oldVersion, gameConfig.localVersion());
      printParams();

    } else if (cmd == 'r') {
      int newResurrectChance = Serial.parseInt();
      if (newResurrectChance < 0 || newResurrectChance > 255) {
        Serial.printf("Invalid resurrectChance.  0-255\n");
        return;
      }
  
      uint16_t oldResurrectChance = gameConfig->resurrectChance;
      int oldVersion = gameConfig.localVersion();
      gameConfig->resurrectChance = newResurrectChance;
      gameConfig.push();
      Serial.printf("Ok, ressurrectChance %d -> %d, configuration version=%d -> %d\n", oldResurrectChance, newResurrectChance,
                    oldVersion, gameConfig.localVersion());
      printParams();

    } else if (cmd == 'g') {
      int newGameType = Serial.parseInt();
      if (newGameType < 0 || newGameType > 1) {
        Serial.printf("Invalid gameType: %d\n",newGameType);
        return;
      }
  
      uint16_t oldGameType = gameConfig->gameType;
      int oldVersion = gameConfig.localVersion();
      gameConfig->gameType = newGameType;
      gameConfig.push();
      Serial.printf("Ok, gameType %d -> %d, configuration version=%d -> %d\n", oldGameType, newGameType,
                    oldVersion, gameConfig.localVersion());
      printParams();
    } else if (cmd == 'p') {
      int newPalette = Serial.parseInt();
      if (newPalette < 0 || newPalette > PALETTES) {
        Serial.printf("Invalid palette: %d\n",newPalette);
        return;
      }
  
      uint16_t oldPalette = gameConfig->palette;
      int oldVersion = gameConfig.localVersion();
      gameConfig->palette = newPalette;
      gameConfig.push();
      Serial.printf("Ok, palette %d -> %d, configuration version=%d -> %d\n", oldPalette, newPalette,
                    oldVersion, gameConfig.localVersion());
      printParams();
    } else if (cmd == 'd') {
      int val = Serial.parseInt();
      if (val < 0 || val > 1) {
        Serial.printf("Invalid debug: %d\n",val);
        return;
      }

      boolean newDebug = (val == 0 ? false : true);
      boolean oldDebug = gameConfig->showGameState;
      int oldVersion = gameConfig.localVersion();
      gameConfig->showGameState = newDebug;
      gameConfig.push();
      Serial.printf("Ok, debug %b -> %b, configuration version=%d -> %d\n", oldDebug, newDebug,
                    oldVersion, gameConfig.localVersion());
      printParams();
    }
  }
}
