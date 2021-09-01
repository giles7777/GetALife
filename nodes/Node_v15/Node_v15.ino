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
#include <FastLED.h> // lights
#include <LinkedList.h> // neighbors
#include <NonBlockingRtttl.h> // sound
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
#define PIN_BUZZER    D5

typedef struct struct_message {
  uint8_t controlVersion;
  uint8_t controlRebroadcastCount;

  uint8_t gameState;
  uint8_t gameRule;
  uint8_t gameGenerationPerSecond;

  uint8_t lightBrightness;
  uint8_t lightSparkleRate;
  uint8_t lightPaletteIndex;

  uint8_t soundPerHour;
  uint8_t soundSongIndex;
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
  Serial << " gGps=" << m->gameGenerationPerSecond;
  Serial << " lBr=" << m->lightBrightness;
  Serial << " lSR=" << m->lightSparkleRate;
  Serial << " lPI=" << m->lightPaletteIndex;
  Serial << " sPph=" << m->soundPerHour;
  Serial << " sSI=" << m->soundSongIndex;
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

  for (uint16_t i = 0; i < Neighbors.size(); ++i) {
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
    addSparkles(0);
    // and off and on   
    myData.lightBrightness = newState ? 255 : 0;
    
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
  if ( senderRSSI  > -100 ) { // a girl's gotta have standards.. and memory.
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

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  mac2str(mac_addr, addrCharBuff);

  Serial << "Sent: " << WiFi.macAddress() << " -> " << addrCharBuff;
  Serial << " status=";

  if (sendStatus == 0) Serial << "OK" << endl;
  else Serial << "FAIL" << endl;

  // blinky
  builtinOn();
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

void setup() {
  delay(10);
  // Serial setup
  Serial.begin(115200);

  pinMode(BUILTIN_LED, OUTPUT);

  // message size cannot exceed 250 bytes
  if ( sizeof(myData) > 250 ) {
    Serial << "HALTING.  message size exceeds ESP-NOW limits!" << endl;
    while (1) yield();
  }

  // Wifi setup
  wifi_set_channel(1);
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(0);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(OnDataSent);
  //  esp_now_register_recv_cb(OnDataRecv); // overridden by the promiscuous handler

  // Set sniffer callback
  wifi_set_promiscuous_rx_cb(wifi_packet_handler);
  wifi_promiscuous_enable(1);

  // Register "peer" or we can't send to it.
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, WIFI_CHAN, NULL, 0);

  // set me up
  WiFi.macAddress(myAddress); // who am I?
  myData.controlVersion = 1; // expect to trigeer OTA?
  myData.controlRebroadcastCount = 0; // for flooding operations, 5?
  myData.gameState = random8(1); // live or let die?
  myData.gameRule = 101; // see notes in generation code
  myData.gameGenerationPerSecond = 2; // change is hard
  myData.lightBrightness = 255; // larger is more
  myData.lightSparkleRate = 255; // inverse function; larger is lower
  myData.lightPaletteIndex = 1; // swirls of color
  myData.soundPerHour = 1; // perhaps play a sound?
  myData.soundSongIndex = 1; // sounds.h

  // force changes to the network by asking for rebroadcasts
  // i.e. flooding?
  myData.controlRebroadcastCount = 5; // spam it up, asking for rebroadcast

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
      memcpy((void*)&myData, (void*)&senderData, sizeof(myData));
      showData(&myData);
      return;
    }

    // copy out any look-and-feel information so all nodes are sync'd in these respects
    // NOTE: comment out to retain own, except rebroadcasts
    myData.gameGenerationPerSecond = senderData.gameGenerationPerSecond;
    // myData.lightBrightness = senderData.lightBrightness;
    myData.lightSparkleRate = senderData.lightSparkleRate;
    myData.lightPaletteIndex = senderData.lightPaletteIndex;
    myData.soundPerHour = senderData.soundPerHour;
    myData.soundSongIndex = senderData.soundSongIndex;

    return;
  }

  // rebroadcast to flood the network?
  static const uint32_t rebroadCastInterval = 30;
  static Metro rebroadCast(rebroadCastInterval);
  if ( rebroadCast.check() && myData.controlRebroadcastCount > 0 ) {
    myData.controlRebroadcastCount--;
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    // add variation in intervals, or we'll all clobber each other
    rebroadCast.interval(random16(rebroadCastInterval / 2, rebroadCastInterval * 2));
    rebroadCast.reset();

    return;
  }

  // run a generation?
  static uint32_t generationTime = (uint32_t)myData.gameGenerationPerSecond * 1000UL;
  EVERY_N_MILLISECONDS( generationTime ) {
    nextGeneration();

    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    showData(&myData);

    // add variation in intervals, or we'll all clobber each other
    generationTime = random16((uint32_t)myData.gameGenerationPerSecond * 1000UL / 2, (uint32_t)myData.gameGenerationPerSecond * 1000UL * 2);

    return;
  }

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
    FastLED.setBrightness(myData.lightBrightness);

    // pallete
    CRGBPalette16 palette;
    switch ( myData.lightPaletteIndex ) {
      // http://fastled.io/docs/3.1/colorpalettes_8cpp_source.html
      case 0: palette = CloudColors_p; break;
      case 1: palette = LavaColors_p; break;
      case 2: palette = OceanColors_p; break;
      case 3: palette = ForestColors_p; break;
      case 4: palette = RainbowColors_p; break;
      case 5: palette = RainbowStripeColors_p; break;
      case 6: palette = PartyColors_p; break;
      case 7: palette = HeatColors_p; break;
      default: palette = Rainbow_gp; break;
    }

    // what was done last round
    static uint8_t index = 0;
    static CRGB lastColor = CRGB::Black;

    // what is done this round
    index += 1;
    CRGB newColor = ColorFromPalette( palette, index, 255, LINEARBLEND );

    // adjust the array
    for (byte i = 0; i < NUM_LEDS; i++) {
      leds[i] -= lastColor; // undo
      leds[i] += newColor;  // do
    }

    // track our changes
    lastColor = newColor;
    FastLED.show();
  }

  // sparkle?
  if ( myData.lightSparkleRate > 0 ) {

    addSparkles(myData.lightSparkleRate);

  }

  // sound

  // if we're already playing, continue to do so
  if ( !rtttl::done() ) {
    rtttl::play();
    return;
  } else if ( myData.soundPerHour > 0 ) {
    // maybe start another song?
    static Metro startSongTimer(1);
    if ( startSongTimer.check() ) {
      // load song
      switch ( myData.soundSongIndex ) {
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
      rtttl::begin(PIN_BUZZER, songBuffer);

      // reset timer
      if ( myData.soundPerHour > 0 ) {
        uint32_t newInterval = 3600000UL / (uint32_t)myData.soundPerHour; // the average song interval.
        startSongTimer.interval((uint32_t)random16(newInterval / 2, newInterval * 2));
      } else {
        // reset the timer so we get an immediate play for the next song requested.
        startSongTimer.interval(1);
      }

    }
  }

}
