// https://arduino-esp8266.readthedocs.io/en/latest/
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html#frame-format
// https://en.wikipedia.org/wiki/Elementary_cellular_automaton

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "ieee80211_structs.h"
#include <Streaming.h>
#include <Metro.h>
#include <FastLED.h>
#include <LinkedList.h>

byte wifiChannel = 1;
#define MAC_SIZE 6

typedef struct struct_message {
  uint8_t controlVersion;
  uint8_t controlRebroadcastCount;

  uint8_t gameState;
  uint8_t gameRule;
  uint8_t gameGenerationInterval;

  uint8_t lightBrightness;
  uint8_t lightSparkleRate;
  uint8_t lightPaletteIndex;

  uint8_t soundPlayRate;
  uint8_t soundSongIndex;
} struct_message;

// storage for nodes we've heard from
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

void mac2str(const uint8_t* ptr, char* string) {
  sprintf(string, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
}

void showData(struct_message *m) {
  Serial << "Data:";
  Serial << " cV=" << m->controlVersion;
  Serial << " cRC=" << m->controlRebroadcastCount;
  Serial << " gS=" << m->gameState;
  Serial << " gR=" << m->gameRule;
  Serial << " gGI=" << m->gameGenerationInterval;
  Serial << " lB=" << m->lightBrightness;
  Serial << " lSR=" << m->lightSparkleRate;
  Serial << " lPI=" << m->lightPaletteIndex;
  Serial << " sPR=" << m->soundPlayRate;
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

void nextGeneration() {
  if ( Neighbors.size() < 2 ) return;

  Neighbors.sort(compareRSSI);
  showNeighbors(8);

  // https://en.wikipedia.org/wiki/Elementary_cellular_automaton 

  Node *left = Neighbors.get(0);
  Node *right = Neighbors.get(1);

  byte currentPattern = 0;
  bitWrite(currentPattern, 2, left->state);
  bitWrite(currentPattern, 1, myData.gameState);
  bitWrite(currentPattern, 0, right->state);
  boolean newState = bitRead(myData.gameRule, currentPattern);

  myData.gameState = newState;

  static byte deadCount = 0;
  if ( myData.gameState == false ) deadCount++;
  else deadCount = 0;

  if ( deadCount > 20 ) {
    myData.gameState = 1; // it's ALIVE!
    myData.gameRule++; // this rule clearly sucks
    myData.controlRebroadcastCount = 5; // force an update
  }

  digitalWrite(BUILTIN_LED, !myData.gameState);

  // we need to maintain the neighbor list
  
  // 1. for memory reasons, we want to keep the list limited to a reasonble count (16?)
  while( Neighbors.size() > 16 ) Neighbors.pop();

  // 2. for permanancy reasons, we want to drop neighbors that are quiet for a long time
  // if we don't, then a neighbor that drops off the network will "jam" at that distance.
  uint8_t i = random8(0, Neighbors.size()-1);
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
      int32_t newRSSI = node->RSSI;
      newRSSI *= smooth;
      newRSSI += (int32_t)senderRSSI;
      newRSSI /= smooth + 1;
      node->RSSI = newRSSI;
      node->state = senderData.gameState;
      
      return;
    }
  }

  // or a new neighbor
  Node *node = new Node();
  memcpy(node->address, senderAddress, MAC_SIZE);
  node->RSSI = senderRSSI;
  node->state = senderData.gameState;     
  Neighbors.add(node);
}


// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  mac2str(mac_addr, addrCharBuff);

  Serial << "Sent: " << WiFi.macAddress() << " -> " << addrCharBuff;
  Serial << " status=";

  if (sendStatus == 0) Serial << "OK" << endl;
  else Serial << "FAIL" << endl;
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
    senderRSSI = ppkt->rx_ctrl.rssi; // RSSI from transmission
    senderNew = true; // flag new data available
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
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, wifiChannel, NULL, 0);

  // set me up
  WiFi.macAddress(myAddress);
  myData.controlVersion = 1;
  myData.controlRebroadcastCount = 0;
  myData.gameState = random8(1);
  myData.gameRule = 101;
  myData.gameGenerationInterval = 5;
  myData.lightBrightness = 255;
  myData.lightSparkleRate = 50;
  myData.lightPaletteIndex = 1;
  myData.soundPlayRate = 1;
  myData.soundSongIndex = 1;

  // force changes to the network by asking for rebroadcasts
  myData.gameGenerationInterval = 1;
  myData.controlRebroadcastCount = 5;
  
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
      return;
    }

    // copy out any look-and-feel information so all nodes are sync'd in those respects
    myData.gameGenerationInterval = senderData.gameGenerationInterval;
    myData.lightBrightness = senderData.lightBrightness;
    myData.lightSparkleRate = senderData.lightSparkleRate;
    myData.lightPaletteIndex = senderData.lightPaletteIndex;
    myData.soundPlayRate = senderData.soundPlayRate;
    myData.soundSongIndex = senderData.soundSongIndex;

    //    mac2str(senderAddress, addrCharBuff);
    //    Serial << senderData.gameState << " from " << addrCharBuff << " rssi " << senderRSSI << endl;

    return;
  }

  // rebroadcast to flood the network?
  const uint32_t rebroadCastInterval = 30;
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
  static Metro generation((uint32_t)myData.gameGenerationInterval * 1000UL);
  if ( generation.check() ) {
    nextGeneration();

    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    showData(&myData);

    // add variation in intervals, or we'll all clobber each other
    generation.interval(random16((uint32_t)myData.gameGenerationInterval * 1000UL / 2, (uint32_t)myData.gameGenerationInterval * 1000UL * 2));
    generation.reset();

    return;
  }

  // memory check
  static Metro status(10000UL);
  if ( status.check() ) {
    Serial << "Memory: free=" << ESP.getFreeHeap();
    Serial << " fragmentation=" << ESP.getHeapFragmentation();
    Serial << " max block=" << ESP.getMaxFreeBlockSize();
    Serial << endl;
    
    status.reset();

    return;
  }



}
