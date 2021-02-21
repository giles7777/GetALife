dodds//************************************************************
// this is a simple example that uses the painlessMesh library to
// connect to a another network and relay messages from a MQTT broker to the nodes of the mesh network.
// To send a message to a mesh node, you can publish it to "painlessMesh/to/12345678" where 12345678 equals the nodeId.
// To broadcast a message to all nodes in the mesh you can publish it to "painlessMesh/to/broadcast".
// When you publish "getNodes" to "painlessMesh/to/gateway" you receive the mesh topology as JSON
// Every message from the mesh which is send to the gateway node will be published to "painlessMesh/from/12345678" where 12345678
// is the nodeId from which the packet was send.
//************************************************************

// Using http://www.hivemq.com/demos/websocket-client/ 
// GaL-in/2732081158 x
// GaL-in/broadcast x
// GaL-in/gateway getNodes
// GaL-out/#

#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <Metro.h>

#define   MESH_PREFIX     "getalife"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

#define   STATION_SSID     "Looney"
#define   STATION_PASSWORD "TinyandTooney"

#define HOSTNAME "GaL_MQTT_Bridge"

// Prototypes
void receivedCallback( const uint32_t &from, const String &msg );
void mqttCallback(char* topic, byte* payload, unsigned int length);

IPAddress getlocalIP();

IPAddress myIP(0, 0, 0, 0);
IPAddress mqttBroker(35, 158, 189, 129); // 35.158.189.129

painlessMesh  mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttBroker, 1883, mqttCallback, wifiClient);

String outTopic = "GaL-out/";
String outTopicGateway = "GaL-out/gateway";
String inTopicAll = "GaL-in/#";
String myId = "Gal_Head_Node_" + String(ESP.getChipId());

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();

  // see logger.hpp for options
  mesh.setDebugMsgTypes( ERROR | STARTUP | MESH_STATUS | CONNECTION | COMMUNICATION | GENERAL | MSG_TYPES);  // set before init() so that you can see startup messages

  // Channel set to 6. Make sure to use the same channel for your mesh and for you other
  // network (STATION_SSID)
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6 );
  mesh.onReceive(&receivedCallback);

  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname(HOSTNAME);

  // Bridge node, should (in most cases) be a root node. See [the wiki](https://gitlab.com/painlessMesh/painlessMesh/wikis/Possible-challenges-in-mesh-formation) for some background
  mesh.setRoot(true);
  // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
  mesh.setContainsRoot(true);
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
      String msg = myId + " is " + mesh.getNodeId();
      mqttClient.publish(outTopicGateway.c_str(), msg.c_str());
    } else {
      Serial.println("No MQTT connection...");
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
