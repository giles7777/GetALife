
// https://github.com/PhracturedBlue/ESP8266MQTTMesh/tree/master/examples/ESP8266MeshHelloWorld

#include <ESP8266MQTTMesh.h>
#include <FS.h>
#include <Streaming.h>
#include <LinkedList.h>
#include <Metro.h>

#ifndef LED_PIN
#define LED_PIN LED_BUILTIN
#endif

// upstream network
wifi_conn    networks[]       = { \
                                  WIFI_CONN("Looney", "TinyandTooney", NULL, 0), \
                                  NULL, \
                                };

// local mesh
const char*  mesh_ssid        = "getalife";
const char*  mesh_password    = "getalife";
const int    mesh_port        = 1884;

// change to locally defined broker at some point
const char*  mqtt_server      = "broker.hivemq.com";
const int    mqtt_port        = 1883;
const char*  mqtt_in_topic    = "GaL-in/";
const char*  mqtt_out_topic   = "GaL-out/";

// for OTA
#define      FIRMWARE_ID        0x1337
#define      FIRMWARE_VER       "0.1"

// me
String ID  = String(ESP.getChipId(), 16); // in hex; importatnt to match with library

// Note: All of the '.set' options below are optional.  The default values can be
// found in ESP8266MQTTMeshBuilder.h
ESP8266MQTTMesh mesh = ESP8266MQTTMesh::Builder(networks, mqtt_server, mqtt_port)
                       .setVersion(FIRMWARE_VER, FIRMWARE_ID)
                       .setMeshSSID(mesh_ssid)
                       .setMeshPassword(mesh_password)
                       .setMeshPort(mesh_port)
                       .setTopic(mqtt_in_topic, mqtt_out_topic)
                       .build();

// Let's define a class to store everything we need to know about neighbor nodes.
class Node {
  public:
    char ID[10] = ""; // Chip ID, so we can match broadcasts; tempting to use String type... don't.

    uint32_t pingTime = 9999; // time to response; smaller numbers are closer.
    uint32_t pingUpdate = 0; // time of last update
    uint8_t dist = 0; // ranking by ping time

    uint8_t rule = 110; // ECA rule; 54 is good too.
    boolean alive = false; // ECA state
};

// and a list to store them in.
LinkedList<Node*> Neighbors = LinkedList<Node*>();

void callback(const char *topic, const char *msg);
void showNeighbors();
void addNeighbor(String recID);
int neighborIndex(String recID);
int comparePingTime(Node *&a, Node *&b);
int comparePingUpdate(Node *&a, Node *&b);
void setup();
void loop();

void setup() {

  Serial.begin(115200);
  delay(1000); //This is only here to make it easier to catch the startup messages.  It isn't required
  Serial.println();

  Serial << "Startup with ID=" << ID << endl;

  mesh.setCallback(callback);
  mesh.begin();

  pinMode(BUILTIN_LED, OUTPUT);

  Serial.println("Setup complete.");
}


void loop() {

  if (! mesh.connected())
    return;

  /*
    static Metro heartbeatInterval(5000UL);
    ifheartbeatInterval(.check()) {
      static uint32_t cnt = 0;
      String msg = "hello from " + ID + " cnt: " + cnt++;
      mesh.publish(ID.c_str(), msg.c_str());
    }
  */
  static Metro announceInterval(10UL * 1000UL);
  if (announceInterval.check()) {
    static uint32_t cnt = 0;
    String Msg = String(cnt++);
    mesh.publish_node(topic_announce, Msg.c_str());
  }

  static Metro neighborsInterval(5UL * 1000UL);
  if (neighborsInterval.check() && Neighbors.size() > 0) {
    // sort by ping interval to establish neighbor to update
    Neighbors.sort(comparePingUpdate);
    Node *node = Neighbors.get(0);
    String Target = String(node->ID);

    mesh.publish_node(topic_howfar, Target.c_str());
    node->pingUpdate = millis(); // record that we sent a ping now
  }

}

void callback(const char *topic, const char *msg) {

  String Topic = String(topic);
  String Msg = String(msg);

  if ( Topic.endsWith(topic_announce) ) {
    String Sender = Topic;
    Sender.remove(Topic.indexOf("/"));
    Serial << "[callback] topic_announce from [" << Sender << "]" << endl;

    addNeighbor(Sender);
  } else if ( Topic.endsWith(topic_howfar) ) {
    Serial << Topic << "=" << Msg << endl;
  } else if ( Topic.endsWith(topic_thisfar) ) {
    Serial << Topic << "=" << Msg << endl;
  } else if (0 == Topic.compareTo(ID)) {
    Serial << "[callback] instructions to me [" << Msg << "]" << endl;

    if ( Msg.equals("led off") ) {
      digitalWrite(BUILTIN_LED, HIGH);
    } else if ( Msg.equals("led on") ) {
      digitalWrite(BUILTIN_LED, LOW);
    }
  }
  // else if ( Msg.startsWith("howfar") ) {
  //      Msg.replace("howfar", ""); // ID that wants a response
  //      String Ret = "thisfar" + ID;
  //
  //      Serial << "[callback] sending topic [" << Msg << "] message [" << Ret << "]" << endl;
  //      mesh.publish_node(Msg.c_str(), Ret.c_str()); // snap back
  //    } else if ( Msg.startsWith("thisfar") ) {
  //      Msg.replace("thisfar", ""); // ID that's responded
  //      int ind = neighborIndex(Msg);
  //
  //      if ( ind > - 1) {
  //        // record the elapsed time.
  //        Node *node = Neighbors.get(ind);
  //        node->pingTime = millis() - node->pingUpdate;
  //
  //        // sort by RSSI to establish closest neighbors
  //        Neighbors.sort(comparePingTime);
  //        for (byte i = 0; i < Neighbors.size(); i++) {
  //          node = Neighbors.get(i);
  //          node->dist = i;
  //        }
  //
  //        // What do we have?
  //        showNeighbors();
  //
  //      }
  else {
    Serial << "[callback] UNKNOWN topic: [" << Topic << "] msg: [" << Msg << "]" << endl;
  }

}

int neighborIndex(String recID) {
  Node *node;
  for (int i = 0; i < Neighbors.size(); ++i) {
    node = Neighbors.get(i);
    if ( recID.compareTo(node->ID) == 0 ) {
      return (i);
    }
  }
  return (-1); // unknown
}

void addNeighbor(String recID) {
  if ( recID == ID ) {
    Serial << "[addNeighbor] we are " << recID << endl;
  } else if ( neighborIndex(recID) > -1 ) {
    Serial << "[addNeighbor] we aleady know " << recID << endl;
  } else {
    Serial << "[addNeighbor] new neighbor " << recID << endl;
    // add a new node.
    Node *newNeighbor = new Node();
    strcpy(newNeighbor->ID, recID.c_str());
    Neighbors.add(newNeighbor);

    // What do we have?
    showNeighbors();

  }
}

void showNeighbors() {
  Node *node;
  Serial << "Uptime, minutes: " << millis() / 1000 / 60 << endl;
  Serial << " \tN\tL?\tLast\tRule\tping\tID" << endl;

  // and neighbors
  for (byte i = 0; i < Neighbors.size(); i++) {
    node = Neighbors.get(i);
    Serial << "\t" << node->dist;
    Serial << "\t" << node->alive;
    Serial << "\t" << (millis() - node->pingUpdate) / 1000 / 60;
    Serial << "\t" << node->rule;
    Serial << "\t" << node->pingTime;
    Serial << "\t" << node->ID;
    Serial << endl;
  }
  Serial << endl;
}

int comparePingTime(Node *&a, Node *&b) {
  return a->pingTime < b->pingTime; // sort by shortest ping time
}

int comparePingUpdate(Node *&a, Node *&b) {
  return a->pingUpdate > b->pingUpdate; // sort by longest ping update time
}
