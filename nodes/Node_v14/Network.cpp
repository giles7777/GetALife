#include "Network.h"

// This is a sprawling pile of crap.  C++ turns into a shitshow when you merge a library with callback function pointers and you want to house those callbacks as methods from an instantiated class.
// It's just a nightmare of arcane compiler errors (if you're lucky) and linker spew (if you're not).
//
// By all means, clean it up.

// Task to blink the number of nodes
#define   LED             BUILTIN_LED       // GPIO number of connected LED, ON ESP-12 IS GPIO2
#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define TASK_BLINK_NODES_ITERATIONS ((mesh.getNodeList().size() + 1) * 2)
#define TASK_BLINK_NODES_DELAY (BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000)
Task blinkNoNodes;
bool onFlag = false;

// Prototypes
extern void handleMeshMessage(uint32_t from, String & msg);
void newConnection(uint32_t nodeId);
void changedConnection();
void nodeTimeAdjusted(int32_t offset);
void distanceReceived(uint32_t from, int32_t delay);
void sendDistanceTest();

// take care that the messages don't exceed ~1400 bytes.
painlessMesh mesh;
// to control tasks
Scheduler userScheduler;

// Task to poll for delays
Task taskSendDistanceTest(TASK_SECOND * 5, TASK_FOREVER, &sendDistanceTest);
// serialied JSON node list with distances
String distance;
// eventually, we're going to publish the distance information as a serialized JSON object, so it makese sense to target that deliverable.
// we could keep it as a linked list or something, but then we'd have to sort and merge when the network structure changed.
// we could keep it as a JSON document, but the authors of that library admonish against that due to memory leaks.

// distance constants
#define DISTANCE_INIT 20000UL // default distance to assume
#define DISTANCE_SMOOTH 20UL // exponential smoothing to reduce jitter
uint32_t delayTestMin, delayTestMax;

void Network::begin() {
  Serial << "Network::begin()" << endl;

  pinMode(LED, OUTPUT);

  mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages

  // TODO: we really should set the WiFi channel of the Mesh to match the AP.
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);

  // defined in main .ino
  mesh.onReceive(&handleMeshMessage);
  // these are handled internally
  mesh.onNewConnection(&newConnection);
  mesh.onChangedConnections(&changedConnection);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjusted);
  mesh.onNodeDelayReceived(&distanceReceived);

  // This node and all other nodes should ideally know the mesh contains a root, so call this on all nodes
  mesh.setContainsRoot(true);

  userScheduler.addTask( taskSendDistanceTest );
  taskSendDistanceTest.enable();
  this->setDistanceTestInterval();

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

  DynamicJsonDocument doc(4096);
  JsonObject d = doc.createNestedObject("distance");
  d[this->getMyNodeId()] = 0; // I'm right here.
  serializeJson(doc, distance);
}

void Network::update() {
  mesh.update();
  digitalWrite(LED, !onFlag);
}

boolean Network::sendMessage(uint32_t nodeId, String & msg) {
  Serial << "Network::sendMessage() to " << nodeId << " -> " << msg << endl;

  mesh.sendSingle(nodeId, msg);
}
boolean Network::sendBroadcast(String & msg) {
  Serial << "Network::sendBroadcast() SHAME/BLAME -> " << msg << endl;

  // I very nearly did not expose this function. Broadcast messaging is very, very likely to fail on the scale of the mesh we're talking about.  
  mesh.sendBroadcast(msg);
}
Nodes Network::getNodeList() {
  return ( mesh.getNodeList() );
}
uint32_t Network::getMeshSize() {
  return ( mesh.getNodeList().size() );
}
String Network::getMyNodeId() {
  return ( String( mesh.getNodeId() ) );
}
uint32_t Network::getNodeTime() {
  return ( mesh.getNodeTime() );
}
String Network::getMeshTopology() {
  return ( mesh.subConnectionJson() );
}
String Network::getStatus() {
  return ( distance );
}

void Network::setStatus(String & msg) {
  DynamicJsonDocument doc(2048);
  if(doc.capacity() == 0) {
    Serial << "Network::setStatus() out of memory." << endl;
    return;
  }
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial << "Network::setStatus() error: " << err.c_str();
    Serial << " status: " << this->getStatus() << endl;
    return;
  } else {
    Serial << "Network::setStatus() memory usage " << doc.memoryUsage() << endl;
  }

  //  if ( ! doc["distance"].isNull() ) distance = msg;
}

void Network::setDistanceTestInterval(uint32_t minSec, uint32_t maxSec) {
  delayTestMin = minSec;
  delayTestMax = maxSec;
  Serial << "Network::setDistanceTestInterval() " << minSec << " to " << maxSec << endl;
}

void sendDistanceTest() {
  // need to keep the network relatively uncluttered, so we poll with sendSingle on an interval.

  // node list
  Nodes nodes = mesh.getNodeList();

  // make sure we have at least one node to talk to
  if ( nodes.size() < 1 ) return;

  // pick a random member of the network
  int index = random(nodes.size());
  auto node = std::next(nodes.begin(), index);

  // show what we do
  Serial << "Network::sendDelayTest() to " << *node << endl;

  // reset to a random interval so we spread out network activity and keep it out-of-sync
  uint32_t numNodes = mesh.getNodeList().size();
  uint32_t newInterval = numNodes * (uint32_t)random(TASK_SECOND * delayTestMin, TASK_SECOND * delayTestMax);
  Serial << "Network::sendDelayTest() node count " << numNodes << " new test interval " << newInterval << " ms" << endl;
  taskSendDistanceTest.setInterval(newInterval);

  // send a packet to measure network trip delay.
  mesh.startDelayMeas(*node);

  // try to do nothing while we wait for the response
}

void newConnection(uint32_t nodeId) {
  Serial << "Network::newConnection() new nodeId " << nodeId << endl;
  //  Serial.printf("newConnectionCallback: \n%s\n", mesh.subConnectionJson(true).c_str());

  changedConnection();
}

void changedConnection() {
  // node list
  Nodes nodes = mesh.getNodeList();

  Serial << "Network::changedConnection() node count " << nodes.size() << endl;

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, distance);
  if (err) {
    Serial << "Network::changedConnection() error: " << err.c_str();
    return;
  }
  JsonObject d = doc["distance"];

  std::list<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    // maybe add nodes
    String Id = String(*node);
    if ( d[Id].isNull() ) d[Id] = DISTANCE_INIT;
    node++;
  }

  // store it
  serializeJson(doc, distance);

  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations(TASK_BLINK_NODES_ITERATIONS);
  blinkNoNodes.enableDelayed(TASK_BLINK_NODES_DELAY);
}

void nodeTimeAdjusted(int32_t offset) {
  Serial << "Network::nodeTimeAdjusted() time " << mesh.getNodeTime() << " offset " << offset << endl;
}

void distanceReceived(uint32_t from, int32_t delay) {
  Serial.printf("Network::distanceReceived() delay to node %u is %d us\n", from, delay);

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, distance);
  if (err) {
    Serial << "Network::distanceReceived() error: " << err.c_str();
    return;
  }
  JsonObject d = doc["distance"];

  String Id = String(from);
  uint32_t dist = d[Id];

  // spurious reading from nodeTime time after sync?
  if ( dist > 0 && delay > dist * 50UL ) return;

  // start smoothing after a predefined time
  static uint32_t smooth = 1;
  static uint32_t startTime = millis();
  if ( smooth == 1 && (millis() - startTime) > (60UL * 1000UL) ) smooth++;
  if ( smooth++ > DISTANCE_SMOOTH ) smooth = DISTANCE_SMOOTH;

  // update distance
  dist = (dist * (smooth - 1UL) + delay) / smooth;
  d[Id] = dist;

  // store it
  distance = ""; // careful. Strings get appended.
  serializeJson(doc, distance);
}
