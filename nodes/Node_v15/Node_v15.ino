// File->Preferences->Additional Board Manager URLs:
//      https://arduino.esp8266.com/stable/package_esp8266com_index.json)
// Board: LOLIN(WEMOS) D1 mini Pro
// default settings seem fine. document changes here.

// Wiring (updated 2/14/21)
// Used: A0:batt, D0:sleep, D4:builtin led, D5:buzzer, D7:LED, D2:PIR, D6:(reserved)
//
// D1 mini pro: bridge BAT-A0, bridge SLEEP; connect +5, +3.3, GND, D5, D7.
// LED: bridge D7 (not D4 default); connect +5, +3.3, GND, D7.
// Buzzer: none (D5 default);  connect +5, +3.3, GND, D5.
///
// I2C Cable: D1 (SCL) D2 (SDA)
//    PIR: bridge D2 (not D3 default)
//
// deprecated:
// Relay: bridge D6 (not D1 default)

// Some light reading
// ESP8266 Core docs:
//    https://arduino-esp8266.readthedocs.io/en/latest/
// ESP-NOW docs:
//    https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html#frame-format
// 1-D cellular automata docs:
//    https://en.wikipedia.org/wiki/Elementary_cellular_automaton

#include <Arduino.h> // basics
#include <ESP8266WiFi.h> // wifi 
#include <espnow.h> // esp-now
#include "ieee80211_structs.h" // RSSI
#include <Streaming.h> // ease outputs
#include <Metro.h> // timers
//#define FASTLED_INTERRUPT_RETRY_COUNT 0 // ohshitbad
//#define FASTLED_ALLOW_INTERRUPTS 0 // ohshitbad
#include <FastLED.h> // lights
#include <LinkedList.h> // neighbors
#include <NonBlockingRtttl.h> // sound
#include <MeshGnome.h>
#include "sounds.h" // "music"

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

// sound
//#define PIN_BUZZER    D4
#define PIN_BUZZER    D5

typedef struct struct_message {
  // if revisions occur, it's important that these two are backward compatible
  uint8_t controlVersion = 1;             // increment to trigger OTA?
  uint8_t controlRebroadcastCount = 5;    // set >0 and this message will flood the network

  // game state information
  uint8_t gameState = 1;                  // dead or alive, but we could implement something else
  uint8_t gameRule = 101;                 // how to arbitrate the next generation
  uint8_t gameInterval = 10;               // seconds between updates

  // I/O settings.  # of entries per gameState;
  uint8_t lightBrightness[2] = {48, 255};      // 0-255 is off-full lighting for dead, alive
  uint8_t lightPaletteIndex[2] = {1, 3};       // palette for dead, alive
  // sound is DISABLED currently
  uint8_t soundInterval[2] = {4, 4};           // minutes between sounds for dead, alive
  uint8_t soundSongIndex[2] = {9, 6};          // song for dead, alive
} struct_message;

// storage for nodes we've heard from
// careful with size; scales linearly with mesh size (ca. 32)
class Node {
  public:
    uint8_t address[MAC_SIZE];
    int16_t RSSI;
    uint8_t state;
};

// and a list to store them in.
LinkedList<Node*> Neighbors = LinkedList<Node*>();

// Create a struct_message
struct_message myData, senderData;
uint8_t myAddress[MAC_SIZE], senderAddress[MAC_SIZE];
uint8_t broadcastAddress[MAC_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
boolean senderNew = false;
int32_t senderRSSI;

// for display
char addrCharBuff[] = "00:00:00:00:00:00\0";

// for lights
CRGB leds[NUM_LEDS];
const uint8_t PROGMEM gamma8[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

// for sound
char songBuffer[512];

// helpers
void mac2str(const uint8_t* ptr, char* string) {
  sprintf(string, "%02X:%02X:%02X:%02X:%02X:%02X", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
}

void showData(struct_message *m) {
  Serial << "Data:";
  Serial << " cV=" << m->controlVersion;
  Serial << " cRC=" << m->controlRebroadcastCount;
  Serial << " gS=" << m->gameState;
  Serial << " gR=" << m->gameRule;
  Serial << " gInt=" << m->gameInterval;
  Serial << " lBr=[" << m->lightBrightness[0] << "," << m->lightBrightness[1] << "]";
  Serial << " lPal=[" << m->lightPaletteIndex[0] << "," << m->lightPaletteIndex[1] << "]";
  Serial << " sInt=[" << m->soundInterval[0] << "," << m->soundInterval[1] << "]";
  Serial << " sInd=[" << m->soundSongIndex[0] << "," << m->soundSongIndex[1] << "]";
  Serial << endl;
}

int compareRSSI(Node *&a, Node *&b) {
  return a->RSSI < b->RSSI;
}

void showNode(Node *&a) {
  mac2str(a->address, addrCharBuff);
  Serial << "Node: " << addrCharBuff << " = " << a->state << " @ " << a->RSSI << endl;
}

void showNeighbors(uint16_t n) {
  n = n > Neighbors.size() ? Neighbors.size() : n;
  Serial << "Neighbors: " << n << " of " << Neighbors.size() << endl;

  for (uint16_t i = 0; i < n; ++i) {
    Node *node = Neighbors.get(i);
    Serial << "\t";
    showNode(node);
  }
}

void addSparkles(uint8_t sparkleRate) {
  // track our sparkliness; critical we do so.
  static boolean isSparkling = false;

  // only sparkle briefly.
  EVERY_N_MILLISECONDS( 25 ) {
    if ( isSparkling ) {
      for (byte i = 0; i < NUM_LEDS; i++) leds[i] -= CRGB::White;
      isSparkling = false;
    }
  }

  // decide if we want to sparkle again
  EVERY_N_MILLISECONDS( 50 ) {
    // every frameRate, we have a 1/sparkleRate probability of sparkling.
    if ( random16(sparkleRate) == 0 ) {
      for (byte i = 0; i < NUM_LEDS; i++) leds[i] += CRGB::White;
      isSparkling = true;
    }
  }
}


void nextGeneration() {
  if ( Neighbors.size() < 2 ) return;

  Neighbors.sort(compareRSSI);
  showNeighbors(8);
//  showNeighbors(255);

  // https://en.wikipedia.org/wiki/Elementary_cellular_automaton

  Node *left = Neighbors.get(0);
  Node *right = Neighbors.get(1);

  byte currentPattern = 0;
  bitWrite(currentPattern, 2, left->state); // am i write or right?
  bitWrite(currentPattern, 1, myData.gameState);
  bitWrite(currentPattern, 0, right->state);
  boolean newState = bitRead(myData.gameRule, currentPattern); // rule 101, usually.

  // BIG.
  if ( myData.gameState != newState ) {

    // lights flare
    // addSparkles(0);

    // sound?

    // all good
    myData.gameState = newState;
  }

  static byte deadCount = 0;
  if ( myData.gameState == false ) deadCount++;
  else deadCount = 0;

  if ( deadCount > 20 ) {
    myData.gameState = 1; // it's ALIVE!
    myData.gameRule++; // this rule clearly sucks
    myData.controlRebroadcastCount = 5; // force an update
  }

  // digitalWrite(BUILTIN_LED, !myData.gameState);

  // we need to maintain the neighbor list

  // 1. for memory reasons, we want to keep the list limited to a reasonble count (16?)
  while ( Neighbors.size() > 16 ) Neighbors.pop();

  // 2. for permanancy reasons, we want to drop neighbors that are quiet for a long time
  // if we don't, then a neighbor that drops off the network will "jam" at that distance.
  uint8_t i = random8(0, Neighbors.size() - 1);
  Node *node = Neighbors.get(i);
  node->RSSI --;

  // the interaction of these two maintenance approaches is that "lively" neighbors remain
}

void updateOrAddNeighbor() {
  // existing neighbor?
  for (uint16_t i = 0; i < Neighbors.size(); ++i) {
    Node *node = Neighbors.get(i);
    if ( memcmp(senderAddress, node->address, MAC_SIZE) == 0 ) {
      // this is the one.

      // exponential smoother on RSSI to reduce jitter/noise.
      const int32_t smooth = 3;
      int32_t newRSSI = ((int32_t)node->RSSI * smooth + senderRSSI) / (smooth + 1);
      node->RSSI = newRSSI;
      node->state = senderData.gameState;

      return;
    }
  }

  // or a new neighbor?
  if ( senderRSSI  > -100 && Neighbors.size() < 64 ) { // a girl's gotta have standards.. and memory.
    Node *node = new Node();
    memcpy(node->address, senderAddress, MAC_SIZE);
    node->RSSI = senderRSSI;
    node->state = senderData.gameState;
    Neighbors.add(node);
  }
}

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

void OnDataSent(uint8_t* mac_addr) {
  mac2str(mac_addr, addrCharBuff);

  Serial << "Sent: " << WiFi.macAddress() << " -> " << addrCharBuff;
  Serial << " status=";

  // blinky
  builtinOn();
}

int sendData(uint8_t *mac_addr, uint8_t *pkt, size_t maxlen) {
  static uint32_t rebroadCastInterval = 50;
  static Metro rebroadCast(rebroadCastInterval);
  if (rebroadCast.check() && myData.controlRebroadcastCount > 0) {
    myData.controlRebroadcastCount--;

    memcpy(mac_addr, broadcastAddress, 6);
    memcpy(pkt, &myData, sizeof(myData));

    Serial << "REBROAD: ";
    showData(&myData);

    // add variation in intervals, or we'll all clobber each other
    rebroadCast.interval(random16(rebroadCastInterval / 2, rebroadCastInterval * 2));
    rebroadCast.reset();

    OnDataSent(mac_addr);
    return sizeof(myData);
  }

  // run a generation?
  static uint32_t generationTime = (uint32_t)myData.gameInterval * 1000UL;
  EVERY_N_MILLISECONDS(generationTime) {
    nextGeneration();

    memcpy(mac_addr, broadcastAddress, 6);
    memcpy(pkt, &myData, sizeof(myData));
    showData(&myData);

    // add variation in intervals, or we'll all clobber each other
    generationTime = random16((uint32_t)myData.gameInterval * 1000UL / 2,
                              (uint32_t)myData.gameInterval * 1000UL * 2);

    OnDataSent(mac_addr);
    return sizeof(myData);
  }

  return -1;
}

// this needs to process very quickly.
void wifi_packet_handler(uint8_t *buff, uint16_t len) {
  // First layer: type cast the received buffer into our generic SDK structure
  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;

  // Second layer: define pointer to where the actual 802.11 packet is within the structure
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;

  // Third layer: define pointers to the 802.11 packet header, payload, frame control
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  const uint8_t *data = ipkt->payload;
  const wifi_header_frame_control_t *frame_ctrl = (wifi_header_frame_control_t *)&hdr->frame_ctrl;

  // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html#frame-format

  /*
     +-------------------------------------------------------------------- \---------------+
    | Espnow-header                                                        \              |
    |   -------------------------------------------------------------------------------   |
    |   | Element ID | Length | Organization Identifier | Type | Version |    Body    |   |
    |   -------------------------------------------------------------------------------   |
    |       1 byte     1 byte            3 bytes         1 byte   1 byte   0~250 bytes    |
    |                                                                                     |
    +-------------------------------------------------------------------------------------+

  */

  static const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  static const uint8_t EspressifOUI[] = {0x18, 0xfe, 0x34}; // 24, 254, 52
  static const uint8_t EspressifType = 4;

  // Only continue processing if this is an action frame containing the Espressif OUI.
  if (
    (wifi_mgmt_subtypes_t)frame_ctrl->subtype == ACTION && // action subtype match
    memcmp(data + 2, EspressifOUI, 3) == 0 && // OUI
    * (data + 2 + 3) == EspressifType && // Type
    ( memcmp(hdr->addr1, broadcastAddress, MAC_SIZE) == 0 || // broadcast
      memcmp(hdr->addr1, myAddress, MAC_SIZE) == 0 ) // or to me
  ) {

    //    byte len = (byte)data[1] - 5; // Length: The length is the total length of Organization Identifier, Type, Version (5) and Body.

    // copy and bail out
    memcpy((void*)&senderData, data + 7, sizeof(senderData)); // payload
    memcpy((void*)&senderAddress, hdr->addr2, sizeof(senderAddress)); // from
    // He spikes the ballllllll, ladies and gentlemen!
    senderRSSI = ppkt->rx_ctrl.rssi; // RSSI from transmission
    // Not easy.
    senderNew = true; // flag new data available

    // blinky
    builtinOff();
  }
}

MeshSyncSketch sketchUpdate(1 /* sketch version */);
CustomProto nodeV15Proto;
DispatchProto protos[] = {  //
  {1 /* protocol id */, &sketchUpdate},
  {2 /* protocol id */, &nodeV15Proto}};

void setup() {
  delay(50);
  // Serial setup
  Serial.begin(115200);
  delay(50);
  Serial << endl << endl << endl << "Startup." << endl;
  
  pinMode(BUILTIN_LED, OUTPUT);

  // message size cannot exceed 250 bytes
  if ( sizeof(myData) > 250 ) {
    Serial << "HALTING.  message size exceeds ESP-NOW limits!" << endl;
    while (1) yield();
  }

  nodeV15Proto.setSendIfNeededFunc(sendData);
  EspProtoDispatch.begin(protos);

  // set me up
  WiFi.macAddress(myAddress); // who am I?

  // what's up
  showData(&myData);

  // lights
  FastLED.addLeds<LED_TYPE, PIN_RGB, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}

void loop() {
  // new send?
  if ( senderNew ) {
    // not anymore
    senderNew = false;

    // update or add this neighbor
    updateOrAddNeighbor();

    if ( senderData.controlVersion > myData.controlVersion ) {
      // TO-DO: flash lights red and seek OTA?
      return;
    }

    if ( senderData.controlRebroadcastCount > 0 ) {
      // overwrite my data with this rebroadcast request.
      // "flooding"
      memcpy((void*)&myData, (void*)&senderData, sizeof(myData));
      showData(&myData);
      return;
    }

    return;
  }

  // rebroadcast to flood the network?
  static uint32_t rebroadCastInterval = 50;
  static Metro rebroadCast(rebroadCastInterval);
  // memory check
  EVERY_N_SECONDS( 10 ) {
    Serial << "Memory: free=" << ESP.getFreeHeap();
    Serial << " fragmentation=" << ESP.getHeapFragmentation();
    Serial << " max block=" << ESP.getMaxFreeBlockSize();
    Serial << endl;

    return;
  }

  // lights
  EVERY_N_MILLISECONDS( 50 ) {
    // light level
    static byte currentBright = 1;
    byte newBright = myData.lightBrightness[myData.gameState];

    const byte brightChange = 5;
    if ( currentBright < newBright ) currentBright += brightChange;
    if ( currentBright > newBright ) currentBright -= brightChange;
    FastLED.setBrightness(pgm_read_byte(&gamma8[currentBright]));

 //   Serial << newBright << "\t" << currentBright << "\t" << pgm_read_byte(&gamma8[currentBright]) << endl;

    // pallete
    static CRGBPalette16 currentPalette = PartyColors_p;
    CRGBPalette16 newPalette;
    switch ( myData.lightPaletteIndex[myData.gameState] ) {
      // http://fastled.io/docs/3.1/colorpalettes_8cpp_source.html
      case 0: newPalette = CloudColors_p; break;
      case 1: newPalette = LavaColors_p; break;
      case 2: newPalette = OceanColors_p; break;
      case 3: newPalette = ForestColors_p; break;
      case 4: newPalette = RainbowColors_p; break;
      case 5: newPalette = RainbowStripeColors_p; break;
      case 6: newPalette = PartyColors_p; break;
      case 7: newPalette = HeatColors_p; break;
      default: newPalette = Rainbow_gp; break;
    }
    const byte paletteChange = 32;
    nblendPaletteTowardPalette(currentPalette, newPalette, paletteChange);

    // track index
    static uint8_t index = 0;
    CRGB newColor = ColorFromPalette( currentPalette, index++, 255, LINEARBLEND );

    // adjust the array
    for (byte i = 0; i < NUM_LEDS; i++) {
      leds[i] = newColor;  // do
    }

    // track our changes
    FastLED.show();
    yield();
  }

/*
  // sound
  // if we're already playing, continue to do so
  if ( !rtttl::done() ) {
    rtttl::play();
    return;
  } else if ( myData.soundInterval[myData.gameState] > 0 ) {
    // maybe start another song?
    static Metro startSongTimer(1);
    if ( startSongTimer.check() ) {
      // load song
      switch ( myData.soundSongIndex[myData.gameState] ) {        
        // from sounds.h
        // I use swtich statements instead of indexing as there's no index protection in C
        case 0: strcpy_P(songBuffer, s_chirp); break;
        case 1: strcpy_P(songBuffer, s_morningTrain); break;
        case 2: strcpy_P(songBuffer, s_boot); break;
        case 3: strcpy_P(songBuffer, s_AxelF); break;
        case 4: strcpy_P(songBuffer, s_RickRoll); break;
        case 5: strcpy_P(songBuffer, s_CrazyTrain); break;
        case 6: strcpy_P(songBuffer, e_Joyful); break;
        case 7: strcpy_P(songBuffer, e_Powerful); break;
        case 8: strcpy_P(songBuffer, e_Peaceful); break;
        case 9: strcpy_P(songBuffer, e_Sad); break;
        case 10: strcpy_P(songBuffer, e_Mad); break;
        case 11: strcpy_P(songBuffer, e_Scared); break;
        default: strcpy_P(songBuffer, s_chirp); break;
      }

      // start it
      Serial << "Sound. Start. " << myData.soundSongIndex[myData.gameState] << endl;
      rtttl::begin(PIN_BUZZER, songBuffer);

      // reset timer
      if ( myData.soundInterval[myData.gameState] > 0 ) {
        uint32_t newInterval = 3600000UL / (uint32_t)(myData.soundInterval[myData.gameState]); // the average song interval.
        startSongTimer.interval((uint32_t)random16(newInterval / 2, newInterval * 2));
        Serial << "Sound. Reset and paused for ~" << newInterval << " of " << (uint32_t)(myData.soundInterval[myData.gameState]) << endl;
      } else {
        // reset the timer so we get an immediate play for the next song requested.
        startSongTimer.interval(1);
        Serial << "Sound. Reset and stopped." << endl;
      }

    }
  }
*/

}
