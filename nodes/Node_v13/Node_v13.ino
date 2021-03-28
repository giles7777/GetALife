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
// GaL-out/gateway/nodes    JSON format network status
// GaL-out/<nodeID>         JSON format <nodeID> status

// https://arduinojson.org/v6/doc/

// Input Topic              Payload
// GaL-in/broadcast         broadcast Payload from gateway to every node
// GaL-in/<nodeID>          target Payload from gateway to node <nodeID>

// Using http://www.hivemq.com/demos/websocket-client/
// GaL-in/2732081158 x
// GaL-in/broadcast x
// GaL-in/gateway getNodes
// GaL-out/#

// https://gitlab.com/painlessMesh/painlessMesh/-/wikis/home

#include <LinkedList.h>
#include <Streaming.h>
#include <painlessMesh.h>
#include "Light.h"
#include "Motion.h"

// some gpio pin that is connected to an LED...
// on my rig, this is 5, change to the right number of your LED.
#define   LED             BUILTIN_LED       // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define   MESH_SSID       "getalife"
#define   MESH_PASSWORD   "conwayIsWatching"
#define   MESH_PORT       5683 // LOVE

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;

// node list
typedef uint32_t Node;
typedef std::list<Node> Nodes;
Nodes nodes;

// track our status/state in a JSON that's easy to publish
StaticJsonDocument<4096> status;

// a place for us to work with JSON i/o
StaticJsonDocument<4096> buffer;

// distance constant
#define DISTANCE_INIT 20000UL // default distance to assume
#define DISTANCE_SMOOTH 20UL // exponential smoothing to reduce jitter

// devices
Light light;
Motion motion;

void sendDelayTest() ; // Prototype
void publishStatus() ; // Prototype

// Task to poll for delays
Task taskSendDelayTest(TASK_SECOND * 5, TASK_FOREVER, &sendDelayTest );

// Task to publish status
Task taskPublish(TASK_SECOND * 10, TASK_FOREVER, &publishStatus );

// Task to blink the number of nodes
#define TASK_BLINK_NODES_ITERATIONS ((mesh.getNodeList().size() + 1) * 2)
#define TASK_BLINK_NODES_DELAY (BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000)
Task blinkNoNodes;
bool onFlag = false;

#define dbgPrintln(lvl, msg) if (((lvl) & (EMMDBG_LEVEL)) == (lvl)) Serial.println("[" + String(FPSTR(__func__)) + "] " + msg)

// take care that the messages don't exceed ~1400 bytes.

void setup() {
  delay(300);
  Serial.begin(115200);

  pinMode(LED, OUTPUT);

  light.begin();
  motion.begin();

  mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
  mesh.setContainsRoot(true);

  userScheduler.addTask( taskSendDelayTest );
  taskSendDelayTest.enable();

  userScheduler.addTask( taskPublish );
  taskPublish.enable();

  blinkNoNodes.set(BLINK_PERIOD, TASK_BLINK_NODES_ITERATIONS, []() {
    // If on, switch off, else switch on
    if (onFlag)
      onFlag = false;
    else
      onFlag = true;
    blinkNoNodes.delay(BLINK_DURATION);

    if (blinkNoNodes.isLastIteration()) {
      // Finished blinking. Reset task for next run
      // blink number of nodes (including this node) times
      blinkNoNodes.setIterations(TASK_BLINK_NODES_ITERATIONS);
      // Calculate delay based on current mesh time and BLINK_PERIOD
      // This results in blinks between nodes being synced
      blinkNoNodes.enableDelayed(TASK_BLINK_NODES_DELAY);
    }
  });
  userScheduler.addTask(blinkNoNodes);
  blinkNoNodes.enable();

  randomSeed(ESP.getChipId());

  // track our status
  status["msgType"] = "nodeStatus";

  status["distance"][String(mesh.getNodeId())] = 0; // I'm right here.

  status["config"]["ver"] = "0.1";
  JsonArray distanceInterval = status["config"].createNestedArray("dI"); // (min,max) interval to do distance pings
  distanceInterval.add(5); // min
  distanceInterval.add(10); // max
  JsonArray pubInterval = status["config"].createNestedArray("pI"); // (min,max) interval to publish status
  pubInterval.add(5); // min
  pubInterval.add(10); // max
  JsonArray skills = status["config"].createNestedArray("skills"); // capabilities
  skills.add("light");
  skills.add("sound");
  skills.add("motion");

  // set lighting
  buffer.clear();
  buffer["light"]["bright"] = 255;
  buffer["light"]["interval"] = 100;
  buffer["light"]["increment"] = 1;
  buffer["light"]["blend"] = LINEARBLEND;
  CRGBPalette16 palette = CloudColors_p;
  for ( byte i = 0; i < 16; i++ ) {
    buffer["light"]["palette"][i][0] = palette.entries[i].red;
    buffer["light"]["palette"][i][1] = palette.entries[i].green;
    buffer["light"]["palette"][i][2] = palette.entries[i].blue;
  }
  serializeJsonPretty(buffer, Serial);
  Serial << endl;

}

void loop() {
  mesh.update();
  light.update();
  motion.update();

  digitalWrite(LED, !onFlag);
}

void sendDelayTest() {
  // need to keep the network relatively uncluttered, so we poll with sendSingle on an interval.

  // make sure we have at least one node to talk to
  if ( mesh.getNodeList().size() < 1 ) return;

  // pick a random member of the network
  int index = random(mesh.getNodeList().size());
  auto node = std::next(nodes.begin(), index);

  // show what we do
  Serial << "sendDelayTest: to " << *node << endl;

  // reset to a random interval so we spread out network activity and keep it out-of-sync
  uint32_t newInterval = random(status["config"]["dI"][0], status["config"]["dI"][1]);
  Serial << "sendDelayTestInterval: " << newInterval << endl;
  taskSendDelayTest.setInterval(TASK_SECOND * newInterval);

  // send a packet to measure network trip delay.
  mesh.startDelayMeas(*node);

  // try to do nothing while we wait for the response
}


void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("receivedCallback: from %u msg=%s\n", from, msg.c_str());

  DeserializationError err = deserializeJson(buffer, msg);
  if (err) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());

    Serial << "Light related message options:" << endl;
    buffer.clear();
    buffer["light"]["bright"] = 255;
    buffer["light"]["interval"] = 100;
    buffer["light"]["increment"] = 1;
    buffer["light"]["blend"] = LINEARBLEND;
    CRGBPalette16 palette = CloudColors_p;
    for ( byte i = 0; i < 16; i++ ) {
      buffer["light"]["palette"][i][0] = palette.entries[i].red;
      buffer["light"]["palette"][i][1] = palette.entries[i].green;
      buffer["light"]["palette"][i][2] = palette.entries[i].blue;
    }
    serializeJson(buffer, Serial);
    Serial << endl;

    return;
  }

  // check directives
  if ( ! buffer["light"]["bright"].isNull() ) light.setBrightness( buffer["light"]["bright"] );
  if ( ! buffer["light"]["interval"].isNull() ) light.setFrameRate( buffer["light"]["interval"] );
  if ( ! buffer["light"]["increment"].isNull() ) light.setColorIncrement( buffer["light"]["increment"] );
  if ( ! buffer["light"]["blend"].isNull() ) light.setBlending( buffer["light"]["blend"] );
  if ( ! buffer["light"]["palette"].isNull() ) {
    CRGBPalette16 palette;
    for ( byte i = 0; i < 16; i++ ) {
      palette.entries[i].red = buffer["light"]["palette"][i][0];
      palette.entries[i].green = buffer["light"]["palette"][i][1];
      palette.entries[i].blue = buffer["light"]["palette"][i][2];
    }
    light.setPalette( palette );
  }
  if ( ! buffer["motion"]["timeout"].isNull() ) motion.setTriggerTimeout( buffer["motion"]["timeout"] );

}

void newConnectionCallback(uint32_t nodeId) {
  // update who we know
  updateNeighbors();

  Serial.printf("newConnectionCallback: new nodeId = %u\n", nodeId);
  Serial.printf("newConnectionCallback: \n%s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback() {
  // update who we know
  updateNeighbors();

  //  Serial.printf("changedConnectionCallback: \n%s\n", mesh.subConnectionJson(true).c_str());

  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");

  std::list<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
}

void updateNeighbors() {
  // node list
  nodes = mesh.getNodeList();

  // see if we need to add a node?
  for (Nodes::iterator id = nodes.begin(); id != nodes.end(); id++) {
    String Id = String(*id);
    Serial << "updateNeighbors: adding " << Id << endl;

    // if this isn't in our list, add it with a starting distance
    JsonVariant dist = status["distance"][Id];
    if ( dist.isNull() ) status["distance"][Id] = DISTANCE_INIT;
  }

  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations(TASK_BLINK_NODES_ITERATIONS);
  blinkNoNodes.enableDelayed(TASK_BLINK_NODES_DELAY);
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("nodeTimeAdjustedCallback: time = %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void publishStatus() {
  // make sure we have someone to talk to.
  if ( mesh.getNodeList().size() < 1 ) return;

  Serial << "publishStatus: ";
  String output;
  serializeJson(status, output);
  Serial << mesh.sendBroadcast(output) << "\t";
  Serial << output << endl;

  // reset to a random interval so we spread out network activity and keep it out-of-sync
  uint32_t newInterval = random(status["config"]["pI"][0], status["config"]["pI"][1]);
  Serial << "publishInterval: " << newInterval << endl;
  taskPublish.setInterval(TASK_SECOND * newInterval);
}


void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("delayReceivedCallback: delay to node %u is %d us\n", from, delay);

  // distance to neighbor
  String Id = String(from);
  uint32_t dist = status["distance"][Id];

  // spurious reading from nodeTime time after sync?
  if ( dist > 0 && delay > dist * 50UL ) return;

  // start smoothing after a predefined time
  static uint32_t smooth = 1;
  static uint32_t startTime = millis();
  if ( smooth == 1 && (millis() - startTime) > (60UL * 1000UL) ) smooth++;
  if ( smooth++ > DISTANCE_SMOOTH ) smooth = DISTANCE_SMOOTH;

  // update distance
  dist = (dist * (smooth - 1UL) + delay) / smooth;
  status["distance"][Id] = dist;
}
