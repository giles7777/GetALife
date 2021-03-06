// For ESP8266 Boards (definitions: https://arduino.esp8266.com/stable/package_esp8266com_index.json)
// Board: LOLIN(WEMOS) D1 mini Pro
// CPU Frequency: 80 MHz
// Flash Size: 16 Mb (~1Mb OTA)

// connect to MQTT broker with http://www.hivemq.com/demos/websocket-client/
// broker.mqttdashboard.com

// subscribe to topics:
// GaL-in/#
// GaL-out/#

// Output Topic             Payload
// GaL-out/gateway/nodes    JSON format network topology
// GaL-out/<nodeID>         JSON format <nodeID> message

// Input Topic              Message     Note  
// GaL-in/gateway           getNodes    
// GaL-in/broadcast         <Payload>   send a payload to mesh.  probabably a bad idea.
// GaL-in/<nodeID>          <Payload>   send a payload to a specific node.  much better idea.
// Payload:
// these <Payload> will get:
//    {"get":"light"}
//    {"get":"motion"}
//    {"get":"network"}
//    {"get":"power"}
//    {"get":"sound"}
// these <Payload> will set:
//    {"light":{"bright":128,"interval":50,"increment":50,"blend":1,"palette":[[0,100,0],[0,100,0],[85,107,47],[0,100,0],[0,128,0],[34,139,34],[107,142,35],[0,128,0],[46,139,87],[102,205,170],[50,205,50],[154,205,50],[144,238,144],[124,252,0],[102]]}}
//    {"motion":{"sensor":true}}
//    {"network":{"2732082441":0,"2732082495":7156}}
//    {"power":{"sleep":0,"battery":4}}
//    {"sound":{"song":"chirp:d=3000,o=2,b=180:a,e,a,e,a,e,a,e,a,e,a,e,a,e,a,e","frequency":0,"count":0}}

#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <Metro.h>

// change these to your home SSID
#define   STATION_SSID        "vodafoneCACC80"
#define   STATION_PASSWORD    "DzUjYekeyvDU2EN4"

// change this to project-local MQTT broker
#define   MQTT_BROKER         "broker.mqttdashboard.com"
#define   MQTT_PORT           1883

// mesh information
#define   MESH_SSID       "getalife"
#define   MESH_PASSWORD   "conwayIsWatching"
#define   MESH_PORT       5683 // LOVE

// Prototypes
void receivedCallback( const uint32_t &from, const String &msg );
void mqttCallback(char* topic, byte* payload, unsigned int length);
IPAddress getlocalIP();

// who am i?
IPAddress myIP(0, 0, 0, 0);
//IPAddress mqttBroker(18, 158, 45, 163); // ping broker.mqttdashboard.com

painlessMesh  mesh;
WiFiClient wifiClient;
//PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);
PubSubClient mqttClient(wifiClient);

String outTopic = "GaL-out/";
String outTopicGateway = "GaL-out/gateway";
String inTopicAll = "GaL-in/#";
String myId = "Gal_Head_Node_" + String(ESP.getChipId());

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println();
getNode
  pinMode(BUILTIN_LED, OUTPUT);

  // see logger.hpp for options
//  mesh.setDebugMsgTypes( ERROR | STARTUP | MESH_STATUS | CONNECTION | COMMUNICATION | MSG_TYPES);  // set before init() so that you can see startup messages
  mesh.setDebugMsgTypes( ERROR | STARTUP | MESH_STATUS | CONNECTION | MSG_TYPES);  // set before init() so that you can see startup messages

  // Channel set to 6. Make sure to use the same channel for your mesh and for you other
  // network (STATION_SSID)
  mesh.init( MESH_SSID, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);

  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname(myId.c_str());

  // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
  mesh.setRoot(true);
  // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
  mesh.setContainsRoot(true);

  // broker/MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(1400); // hard limit in mesh library?

}

void loop() {
  mesh.update();
  mqttClient.loop();

  if (myIP != getlocalIP()) {
    myIP = getlocalIP();
    Serial.println("My IP is " + myIP.toString());

    if (mqttClient.connect(myId.c_str())) {
      String msg = "Ready and subscribed " + inTopicAll;
      mqttClient.publish(outTopicGateway.c_str(), msg.c_str());

      mqttClient.subscribe(inTopicAll.c_str());
      Serial.println("Subscribed MQTT: " + inTopicAll);
    }
  }

  static Metro heartbeat(10000UL);
  if ( heartbeat.check() ) {
    if ( mqttClient.connected() ) {
      String tp = outTopicGateway + "/nodes";
      mqttClient.publish(tp.c_str(), mesh.subConnectionJson().c_str());
      digitalWrite(BUILTIN_LED, LOW);
    } else {
      Serial.println("No MQTT connection...");
      digitalWrite(BUILTIN_LED, HIGH);
    }
  }
}

void receivedCallback( const uint32_t &from, const String &msg ) {
  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
  String topic = outTopic + String(from);
  mqttClient.publish(topic.c_str(), msg.c_str());
}

void mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  String Topic = topic;
  Topic.trim(); // cull trailing \n

  char* cleanPayload = (char*)malloc(length + 1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length + 1);
  String Msg = String(cleanPayload);
  free(cleanPayload);
  Msg.trim(); // cull trailing \n

  Serial.println(String("<- Topic [") + Topic + String("] Msg [") + Msg + String("]"));

  String targetStr = Topic.substring(inTopicAll.length() - 1); // drop the "#"
  if (targetStr == "gateway") {
    if (Msg == "getNodes") {
      auto nodes = mesh.getNodeList(true);
      String str;
      for (auto && id : nodes)
        str += String(id) + String(" ");

      mqttClient.publish(outTopicGateway.c_str(), str.c_str());
      Serial.println("-- network=" + str);
    }
  } else if (targetStr == "broadcast") {
    Serial.println("-> broadcast=" + Msg);
    mesh.sendBroadcast(Msg);
  } else {
    uint32_t target = strtoul(targetStr.c_str(), NULL, 10);
    if (mesh.isConnected(target)) {
      Serial.println("-> target=" + Msg);
      mesh.sendSingle(target, Msg);
    } else {
      String msg = "target [" + String(target) + "] not found.";
      Serial.println(msg);
      mqttClient.publish(outTopicGateway.c_str(), msg.c_str());
    }
  }
}

IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}
