#ifndef Network_h
#define Network_h

#include <Arduino.h>

#include <painlessMesh.h>

#include <ArduinoJson.h>
#include <Streaming.h>

#define   MESH_SSID       "getalife"
#define   MESH_PASSWORD   "conwayIsWatching"
#define   MESH_PORT       5683 // LOVE

// mesh returns a std::list of nodes
typedef uint32_t Node;
typedef std::list<Node> Nodes;

class Network {
  public:
    void begin();

    void update();

    // exposed from painlessMesh: https://github.com/gmag11/painlessMesh
    boolean sendMessage(uint32_t nodeId, String & msg);
    boolean sendBroadcast(String & msg); // <- this is a bad idea and will crash a mesh of the size we're implementing.
    Nodes getNodeList();
    uint32_t getMeshSize();
    String getMyNodeId();
    uint32_t getNodeTime();
   String getMeshTopology();
    
    String getStatus();
    void setStatus(String & msg);

    // contain the average number of distance tests 
    void setDistanceTestInterval(uint32_t minSec=2, uint32_t maxSec=4);

};

#endif
