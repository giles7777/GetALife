// For ESP8266 Boards (definitions: https://arduino.esp8266.com/stable/package_esp8266com_index.json)
// Board: LOLIN(WEMOS) D1 mini Pro
// CPU Frequency: 80 MHz
// Flash Size: 16 Mb (~1Mb OTA)

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


#include <Metro.h>
#include <Streaming.h>

#define FASTLED_ALLOW_INTERRUPTS 0
//#define FASTLED_INTERRUPT_RETRY_COUNT 3
#include <FastLED.h>
FASTLED_USING_NAMESPACE
#include "Light.h"

#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <EEPROM.h>
#include "Power.h"

#include "Motion.h"

#include <NonBlockingRtttl.h>
#include "Sound.h"

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <LinkedList.h>
#include "Network.h"


// not at all clear what we're doing for neighbors, so here's a stab at it.

// Let's define a class to store everything we need to know about neighbor nodes.
class Node {
  public:
    String MAC=""; // MAC address, so we can match broadcasts
    int16_t RSSI=-99; // signal strength; larger numbers are good.
    uint8_t dist=0; // ranking by RSSI
    uint8_t rule=110; // ECA rule; 54 is good too.
    boolean alive=false; // ECA state
    uint32_t lastUpdate=0; // time of last update
    uint16_t voltage=0; // last voltage reading
};

void showNeighbors();
void findNeighbors();
int neighborIndex(String &recMAC);
void nextGeneration();
int compareRSSI(Node *&a, Node *&b);
int compareMAC(Node *&a, Node *&b);
void randomStart();
void setup();
void loop();

// following: https://en.wikipedia.org/wiki/Elementary_cellular_automaton
boolean myState = false;
byte myRule = 110;
// Rules
// 119: appears stable with 4 nodes

// and a list to store them in.
LinkedList<Node*> Neighbors = LinkedList<Node*>();

// instantiate peripherals
Light lights;
Power power;
Motion motion;
Sound sound;
Network network;


void randomStart() {
  int i = random(0,100);
  if( i<60 ) myState = true;
}

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

  // initialize start
  randomStart();
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
//    serializeJson(doc, Serial);
     
    // and are they in the neightbors list?
    int nn = neighborIndex(network.inFrom);

    // and from whom
//    Serial << " <- #" << nn << "\t" << network.inFrom << endl;
    
    // if not, then we need to re-up our neighbor list
    if( nn == -1 ) { 
      findNeighbors();
      return;
    } 

    String s1 = String("ECA_v1");
    String s2 = doc["packet"];
    if( s1.equals(s2) ){
      // if they are, take their state information
      Node *node = Neighbors.get(nn);
      node->rule = doc["rule"];
      node->alive = doc["alive"];
      node->voltage = doc["voltage"];
      node->lastUpdate = millis();
      
      // and shift my rules, if needed.
      myRule = node->rule;
      
      return;
    } 
  }

  if( motion.update() ) {
    Serial << "Motion: " << motion.isMotion() << endl;
    if( motion.isMotion() ) myState = true;  // Rez!
  }

  EVERY_N_MILLISECONDS( 20 ) {
    static byte i = 0;
    static byte minAlive = 32;
    static byte maxAlive = 255;
    
    byte bright = myState ? qadd8(scale8(cubicwave8(i++), maxAlive-minAlive), minAlive) : 0;
 
    lights.setBrightness(bright);
  }
  
  EVERY_N_SECONDS( 1 ) {
    StaticJsonDocument<MAX_DATA_LEN> doc;

    doc["packet"] = String("ECA_v1");
    doc["rule"] = myRule;
    doc["alive"] = myState;
    doc["voltage"] = power.batteryVoltage();
        
    serializeJson(doc, network.outData);
    network.send();
  }


  EVERY_N_SECONDS( 5 ) {
    nextGeneration();

    showNeighbors();
  }

  EVERY_N_MINUTES( 5 ) {
    Serial << "Battery voltage=" << power.batteryVoltage() << endl;

    findNeighbors();
  }

  EVERY_N_MINUTES( 9999999 ) {
    Serial << "Shutting down after 1 hour" << endl;
    lights.setBrightness(0);
    power.deepSleep(0); // forever
  }

}

void nextGeneration() {
  
  if ( Neighbors.size() < 2 ) return;
  
  Node *left = Neighbors.get(0);
  Node *right = Neighbors.get(1);

  byte currentPattern = 0;
  bitWrite(currentPattern, 2, left->alive);
  bitWrite(currentPattern, 1, myState);
  bitWrite(currentPattern, 0, right->alive);
  boolean newState = bitRead(myRule, currentPattern);
/*
  Serial << " l=" << left->alive;
  Serial << " c=" << myState;
  Serial << " r=" << right->alive;
  Serial << " cP=" << currentPattern;
  Serial << " nS=" << newState << endl;
*/  
  myState = newState;

  static byte deadCount = 0;
  if( myState == false ) deadCount++;
  else deadCount=0;

  if( deadCount > 5 ) {
    randomStart(); // it's ALIVE!
    myRule++; // this rule clearly sucks
  }
}

int neighborIndex(String &recMAC) {  
  Node *node;
  for (int i = 0; i < Neighbors.size(); ++i) {
    node = Neighbors.get(i);  
    if( node->MAC.compareTo(recMAC) == 0) {
      return(i);
    }
  }
  return(-1);
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

    // sort by RSSI to establish closest neighbors
    Neighbors.sort(compareRSSI);
    Node *node;
    for (byte i = 0; i < Neighbors.size(); i++) {
      node = Neighbors.get(i);
      node->dist = i;
    }
    
    // What do we have?
    showNeighbors();
  }
}

void showNeighbors() {
    Node *node;
    Serial << "Uptime, minutes: " << millis()/1000/60 << endl;
    Serial << " \tN\tL?\tV\tLast\tRule\tRSSI\tMAC" << endl;

    // me first
    Serial << "\t" << "me";
    Serial << "\t" << myState;
    Serial << "\t" << power.batteryVoltage();
    Serial << "\t" << "NA";
    Serial << "\t" << myRule;
    Serial << "\t" << "NA";
    Serial << "\t" << network.myMAC;
    Serial << endl;

    // and neighbors
    for (byte i = 0; i < Neighbors.size(); i++) {
      node = Neighbors.get(i);
      Serial << "\t" << node->dist;
      Serial << "\t" << node->alive;
      Serial << "\t" << node->voltage;
      Serial << "\t" << (millis() - node->lastUpdate)/1000/60;      
      Serial << "\t" << node->rule;
      Serial << "\t" << node->RSSI;
      Serial << "\t" << node->MAC;
      Serial << endl;
    }
    Serial << endl;  
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
