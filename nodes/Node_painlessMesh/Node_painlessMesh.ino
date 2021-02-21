m//************************************************************
// this is a simple example that uses the easyMesh library
//
// 1. blinks led once for every node on the mesh
// 2. blink cycle repeats every BLINK_PERIOD
// 3. sends a silly message to every node on the mesh at a random time between 1 and 5 seconds
// 4. prints anything it receives to Serial.print
//
//
//************************************************************

// https://gitlab.com/painlessMesh/painlessMesh/-/wikis/home

#include <LinkedList.h>
#include <Streaming.h>
#include <painlessMesh.h>

// some gpio pin that is connected to an LED...
// on my rig, this is 5, change to the right number of your LED.
#define   LED             BUILTIN_LED       // GPIO number of connected LED, ON ESP-12 IS GPIO2

#define   BLINK_PERIOD    3000 // milliseconds until cycle repeat
#define   BLINK_DURATION  100  // milliseconds LED is on for

#define   MESH_SSID       "getalife"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
std::list<uint32_t> nodes;
std::list< std::pair<uint32_t,uint32_t> > neighbors; // first is nodeId, second is delay

void sendDelayTest() ; // Prototype

// Task to poll for delays
#define TASK_SEND_INTERVAL random(TASK_SECOND * 5, TASK_SECOND * 15)
Task taskSendDelayTest(TASK_SEND_INTERVAL, TASK_FOREVER, &sendDelayTest );

// Task to blink the number of nodes
#define TASK_BLINK_NODES_ITERATIONS ((mesh.getNodeList().size() + 1) * 2)
#define TASK_BLINK_NODES_DELAY (BLINK_PERIOD - (mesh.getNodeTime() % (BLINK_PERIOD * 1000)) / 1000)
Task blinkNoNodes;
bool onFlag = false;

#define dbgPrintln(lvl, msg) if (((lvl) & (EMMDBG_LEVEL)) == (lvl)) Serial.println("[" + String(FPSTR(__func__)) + "] " + msg)

void setup() {
  Serial.begin(115200);

  pinMode(LED, OUTPUT);

  mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  userScheduler.addTask( taskSendDelayTest );
  taskSendDelayTest.enable();

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
}

void loop() {
  mesh.update();
  digitalWrite(LED, !onFlag);
}

void sendDelayTest() {
  // need to keep the network relatively uncluttered, so we poll with sendSingle on an interval.

  // make sure we have at least one node to talk to
  if ( mesh.getNodeList().size() < 1 ) return;

  // pick a random member of the network
  int index = random(mesh.getNodeList().size());
  auto node = std::next(nodes.begin(), index);

  // send a packet to measure network trip delay.
  mesh.startDelayMeas(*node);

  // show what we did
  Serial << "sendDelayTest: to " << *node << endl;

  // reset to a random interval
  taskSendDelayTest.setInterval(TASK_SEND_INTERVAL);
}


void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("receivedCallback: from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations(TASK_BLINK_NODES_ITERATIONS);
  blinkNoNodes.enableDelayed(TASK_BLINK_NODES_DELAY);

  Serial.printf("newConnectionCallback: new nodeId = %u\n", nodeId);
  Serial.printf("newConnectionCallback: \n%s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback() {
  // Reset blink task
  onFlag = false;
  blinkNoNodes.setIterations(TASK_BLINK_NODES_ITERATIONS);
  blinkNoNodes.enableDelayed(TASK_BLINK_NODES_DELAY);

  // reset list
  nodes = mesh.getNodeList();

  // merge lists
//  nodes.sort();
//  neighbors.sort();
//  neighbors.merge(nodes);
  
  Serial.printf("changedConnectionCallback: \n%s\n", mesh.subConnectionJson(true).c_str());

  /*
    Serial.printf("Num nodes: %d\n", nodes.size());
    Serial.printf("Connection list:");

    std::list<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      Serial.printf(" %u", *node);
      node++;
    }
    Serial.println();
    calc_delay = true;
  */
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("nodeTimeAdjustedCallback: time = %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("delayReceivedCallback: delay to node %u is %d us\n", from, delay);


  std::list<Node>::iterator node = Neighbors.begin();
  while (node != Neighbors.end()) {
    if ( node->nodeId == from) {
      // smooth noise
      const uint32_t smooth = 200UL;
      node->delay = (node->delay * (smooth - 1UL) + (uint32_t)delay) / smooth;

      // What do we have?
      showNeighbors();
    }
    node++;
  }
  return(-1);


  int ind = neighborIndex(from);
  if ( ind == -1 ) {
    // add a new node.
    Node *newNeighbor = new Node();
    newNeighbor->nodeId = from;
    newNeighbor->delay = delay;
    Neighbors.push_front(&newNeighbor);
    delete(newNeighbor);
  } else {
    // find the node
    auto node = std::next(Neighbors.begin(), ind);

    // smooth noise
    const uint32_t smooth = 200UL;
    node->delay = (node->delay * (smooth - 1UL) + (uint32_t)delay) / smooth;
  }
  // What do we have?
  showNeighbors();
}

int neighborIndex(uint32_t rNode) {
  std::list<Node>::iterator node = Neighbors.begin();
  while (node != Neighbors.end()) {
    if ( node->nodeId == rNode) {
      return (i);
    }
    node++;
  }
  return(-1);
}

void showNeighbors() {
  Serial << "showNeighbors:\tnodeID\tDelay" << endl;

  // me first
  Serial << "\t\t" << mesh.getNodeId();
  Serial << "\t\t" << 0;
  Serial << endl;

  std::list<Node>::iterator node = Neighbors.begin();
  while (node != Neighbors.end()) {
    Serial << "\t\t" << node->nodeId;
    Serial << "\t\t" << node->delay;
    Serial << endl;

    node++;
  }
}
