#include <MeshGnome.h>
#include <Arduino.h> // basics
#include <ESP8266WiFi.h> // wifi 
#include <espnow.h> // esp-now
#include <Streaming.h> // ease outputs
#include <FastLED.h> // lights
#include <map>
#include <Schedule.h>

// Animation/Lighting modes
#define SHOW_TRANSMIT false  // Show debug colors when updating code
#define SHOW_GAME_STATE false // Show direct game state instead of pretty color changes

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
  uint16_t gameSpeed = 1000;  // cmd t
  uint8_t podSize = 8;
  uint8_t minAlive = 2;
  uint8_t maxAlive = 3;
  uint8_t resurrectChance = 10;  // cmd r
  uint8_t gameType = 1;  // cmd g
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
MeshSyncSketch sketchUpdate(304); // Update these each code change to propigate

MeshSyncTime syncedTime;
LocalPeriodicStruct<LocalShareData> shareLocal;
DispatchProto protos[] = {  //
  {0 /* protocol id */, &sketchUpdate},
  {1 /* protocol id */, &syncedTime},
  {2 /* protocol id */, &gameConfig},
  {3 /* protocol id */, &shareLocal},
  };

char addrCharBuff[] = "00:00:00:00:00:00\0";

CRGB leds[NUM_LEDS];
bool updatingCode = false;
bool transmittingCode = false;
size_t transmitOffset = 0;
size_t transmitLength = 1;
size_t lastTransmitOffset = 0;
size_t updateOffset = 0;
size_t updateLength = 0;
uint16_t lastGameSpeed = 0;

// local state
std::map<EthStruct, Neighbor> neighbors;
uint8_t state = 0;
uint8_t colorIndex = random(255);
uint8_t destIndex = 0;
uint16_t fraction = 0;

CRGBPalette16 currentPalette;

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
      transmittingCode = false;
    } else {
      transmittingCode = true;
    }
  }
  transmitOffset = offset;
  transmitLength = length;
}

void testLights() {
  if (updatingCode || transmittingCode) return;

  static uint8_t idx = 0;

  /*
  CRGB aliveColor = ColorFromPalette( currentPalette, idx, 255, NOBLEND);

  Serial.printf("Index: %d\n", idx);
  Serial.print(aliveColor[0]); Serial.print("\t");
  Serial.print(aliveColor[1]); Serial.print("\t");
  Serial.println(aliveColor[2]);

  if (SHOW_TRANSMIT) {
    for (byte i = 0; i < NUM_LEDS; i++) {
      leds[i] = aliveColor;
    }
    FastLED.show();
  }
  idx = idx + 3;
  */
  if (idx % 2 == 0) {
    for (byte i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Blue;
    FastLED.show();
  } else {
    for (byte i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Red;
    FastLED.show();    
  }
}

void nextGeneration() {
  if (updatingCode || transmittingCode) return;

  //Serial.printf("Next gen:\n");
  //showNeighbors(8);

/*
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
*/ // TODO: Removed aging to test

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

    long roll = random(100);
    if (roll < gameConfig->resurrectChance) {
      state = 1;
    }
  
    if (state == 0) {
      Serial.printf("Dead\n");
      /*
      for (byte i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Red;
      FastLED.show();
      */
    } else {
      Serial.printf("Alive\n");
      /*
      for (byte i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Green;
      FastLED.show();
      */
    }
  } else if (gameConfig->gameType == 1) {
      int aliveNeighbors = 0;
      boolean newState = false;
      int16_t firstColorIndex = -1;
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
          if (firstColorIndex == -1 && colorIndex != n.colorIndex) {
            firstColorIndex = n.colorIndex;
          }
        }
      }
      Serial.printf("Neighbors: %d  alive: %d colorIndex: %d \n",neighbors.size(),aliveNeighbors, newColorIndex);
      if (aliveNeighbors >= gameConfig->minAlive && aliveNeighbors <= gameConfig->maxAlive) {
        newState = true;
        if (aliveNeighbors > 0) {
          newColorIndex = firstColorIndex;
        }
      }

      if (state != newState) {
        state = newState;
        fraction = 0;
      }
      //colorIndex = newColorIndex;
      
      static byte deadCount = 0;
      if ( state == false ) deadCount++;
      else deadCount = 0;
  
      long roll = random(255);
      if (roll < gameConfig->resurrectChance) {
        state = 1;
        destIndex = random(0,255);
        fraction = 0;
        Serial.printf("Ressurrection  roll: %d  color: %d\n",roll,colorIndex);
      }

      if (state == 0) {
/*        
        for (byte i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
        FastLED.show();
*/
        destIndex = 0;
      } else {
        Serial.printf("I'm alive!  color: %d\n",newColorIndex);
        /*
        CRGB aliveColor = ColorFromPalette( currentPalette, colorIndex, 255, LINEARBLEND);

        Serial.print(aliveColor[0]); Serial.print("\t");
        Serial.print(aliveColor[1]); Serial.print("\t");
        Serial.println(aliveColor[2]);
*/
        destIndex = newColorIndex;
/*        
        for (byte i = 0; i < NUM_LEDS; i++) {
          leds[i] = aliveColor;
        }
        FastLED.show();
        */
      }

  }
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
      updatingCode = true;
    } else {
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
      Serial.printf("Transmitting: %d / %d\n", transmitOffset, transmitLength);
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
  shareLocal.begin(&syncedTime, 2000 /* run every 2000 ms */);

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

//  currentPalette = Rainbow_gp;
  currentPalette = HeatColors_p;

  Serial.printf("Commands:  t - gameSpeed.  r - resurrectChance.  g - gameType\n");
}

void loop() {
  EspSnifferProtoDispatch.espTransmitIfNeeded();
  
  EVERY_N_MILLISECONDS( 5000 ) {
    if (lastTransmitOffset == transmitOffset) {
      transmittingCode = false;
    }
    lastTransmitOffset = transmitOffset;
  }

  if (lastGameSpeed != gameConfig->gameSpeed) {
    Serial.printf("New game speed: %d\n", gameConfig->gameSpeed);
    lastGameSpeed = gameConfig->gameSpeed;
    shareLocal.begin(&syncedTime, gameConfig->gameSpeed); 
  }
  shareLocal.run();

  const uint16_t animTime = 1000;
  
  // Local animations
  EVERY_N_MILLISECONDS( animTime ) {
    if (updatingCode || transmittingCode) {
      return;
    }
    
    if (state == 0) {
      for (byte i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
      }      
      FastLED.show();
    } else {
      if (SHOW_GAME_STATE) {
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
        if (idx > 255) c_idx = 255;
        if (idx < 0) c_idx = 0;
        else c_idx = idx;
  
        Serial.printf("colorIndex: %d  destIndex: %d  idx: %d  frac: %d  leds: %d\n",colorIndex,destIndex,c_idx,fraction,NUM_LEDS);
//        CRGB color = ColorFromPalette( currentPalette, c_idx, fraction, NOBLEND);    
        CRGB color;
//        color = ColorFromPalette( currentPalette, c_idx, 255, NOBLEND);
        color = ColorFromPalette( currentPalette, c_idx);  // TODO: This causes the red flashes
        color = CRGB::Blue;
        for (byte i = 0; i < NUM_LEDS; i++) {
          leds[i] = color;
        }
        FastLED.show();
      }
    }
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
    }
  }

}
