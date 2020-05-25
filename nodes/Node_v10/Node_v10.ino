// For ESP8266
// LOLIN(WEMOS) D1 R2 & mini
// 80 MHz
// 4 Mb (3 Mb OTA)
// 

#include <Metro.h>
#include <Streaming.h>

#define FASTLED_ALLOW_INTERRUPTS 0
//#define FASTLED_INTERRUPT_RETRY_COUNT 3
#include <FastLED.h>
FASTLED_USING_NAMESPACE
#include "Light.h"

#include "Power.h"

#include "Motion.h"

#include <NonBlockingRtttl.h>
#include "Sound.h"

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <LinkedList.h>
#include <ArduinoJson.h>
#include "Network.h"

// not at all clear what we're doing for neighbors, so here's a stab at it.

// Let's define a class to store everything we need to know about neighbor nodes.
class Node {
  public:
    String MAC; // MAC address, so we can match broadcasts
    int16_t RSSI; // signal strength; larger numbers are good.
    uint8_t dist; // ranking by RSSI
};

// and a list to store them in.
LinkedList<Node*> Neighbors = LinkedList<Node*>();

// instantiate peripherals
Light lights;
Power power;
Motion motion;
Sound sound;
Network network;

void setup() {  
  // delay before proceeding as there may be reprogramming requests.
  delay(1000);
  // no, seriously.  don't disable this.
  // with the RST pin tied to D0, you'll likely jam the uC if you
  // remove this delay.

  Serial.begin(115200);
  Serial << endl << endl;

  // start up peripherals
  lights.begin();
  power.begin();
  motion.begin();
  sound.begin();
  network.begin();
}

void loop() {
  // update the peripherals
  lights.update();
  power.update();
  sound.update();
  
  if( network.update() ) {
    StaticJsonDocument<MAX_DATA_LEN> doc;
    DeserializationError error = deserializeJson(doc, network.inData, MAX_DATA_LEN);
 
    // Test if parsing succeeded.
    if (error) {
      Serial.print("deserializeMsgPack() failed: ");
      Serial.println(error.c_str());
      return;
    }
    // show what was received.
    serializeJson(doc, Serial);

    // and from whom
    Serial << " <- " << network.inFrom;
    
    // and are they in the neightbors list?
    boolean isN = isNeighbor(network.inFrom);
    Serial << " Gal? " << isN << endl;

    // if not, then we need to re-up our neighbor list
    if( ! isN ) findNeighbors();
  }

  if( motion.update() ) {
    Serial << "Motion: " << motion.isMotion() << endl;
  }

  EVERY_N_SECONDS( 5 ) {
    StaticJsonDocument<MAX_DATA_LEN> doc;
    static int x = 10;
    doc["count"] = x++;
    serializeJson(doc, network.outData);
    network.send();
  }
  
  EVERY_N_MILLISECONDS( 20 ) {
    static byte i = 0;
    static byte minBright = 16;
    byte bright = qadd8(scale8(cubicwave8(i++), 255-minBright), minBright);
    lights.setBrightness(bright);
  }

  EVERY_N_MINUTES( 10 ) {
    Serial << "Battery voltage=" << power.batteryVoltage() << endl;
  }

  EVERY_N_MINUTES( 60 ) {
    Serial << "Shutting down after 1 hour" << endl;
    lights.setBrightness(0);
    power.deepSleep(0); // forever
  }

}

boolean isNeighbor(String &recMAC) {  
  Node *node;
  for (int i = 0; i < Neighbors.size(); ++i) {
    node = Neighbors.get(i);  
//    Serial << " " << node->MAC << " " << recMAC << endl;
    if( node->MAC.compareTo(recMAC) == 0) {
//      Serial << "  Neighbor=" << node->dist << " RSSI=" << node->RSSI << endl;
      return(true);
    }
  }
  return(false);
}

void findNeighbors() {
  Serial << endl << "Neighbors.... ";

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("none found.");
  } else {
    Serial.print(n);
    Serial.println(" found.");

    // clear list
    while ( Neighbors.size() > 0 ) {
      Node * drop = Neighbors.pop();
      delete drop; // I guess?
    }
    
    // add valid GaL entries
    for (int i = 0; i < n; ++i) {
      // is this a GaL device?
      boolean isGaL = String(WiFi.SSID(i)).startsWith(network.prefixSSID);
      
      if( isGaL ) {
        // add a new node.
        Node *newNeighbor = new Node();
        newNeighbor->MAC = String(WiFi.SSID(i));
        newNeighbor->MAC.replace(network.prefixSSID,"");
        newNeighbor->RSSI = WiFi.RSSI(i);
        Neighbors.add(newNeighbor);
      }
    }

    // What do we have?
    Neighbors.sort(compareRSSI);
    Node *node;
    Serial << " \tN\tRSSI\tMAC" << endl;
    for (byte i = 0; i < Neighbors.size(); i++) {
      node = Neighbors.get(i);
      node->dist = i;
      Serial << "\t" << node->dist;
      Serial << "\t" << node->RSSI;
      Serial << "\t" << node->MAC << endl;
    }
    Serial << endl;
  }
}

int compareRSSI(Node *&a, Node *&b) {
  return a->RSSI < b->RSSI;
}

int compareMAC(Node *&a, Node *&b) {
  return a->MAC.compareTo(b->MAC);
}

// hardware locations
/*
  Orient board with module facing you, reset button on the top (right)
  Reset/notch/right side; reading down from notch on module side
  Device  Pin   GPIO  Function
        3V3   3.3V  3.3V
  Buzz  D8    15  IO, 10k Pull-down, SS
  RGB   D7    13  IO, MOSI
  PIR   D6    12  IO, MISO
        D5    14  IO, SCK
  Sleep D0    16  IO
  Batt  A0    A0  Analog input, max 3.3V  A0
  Sleep RST   RST Reset

  not-reset/not-notched/left side; reading down
  Device  Pin   GPIO  Function
      5V    5V    5V
      G     GND Ground
  LED D4    2   IO, 10k Pull-up, BUILTIN_LED
      D3    0   IO, 10k Pull-up
  RGB D2    4   IO, SDA
      D1    5   IO, SCL
      TX    TXD TXD
      RX    RXD   RXD
*/
