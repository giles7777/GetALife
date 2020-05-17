// For ESP32.  WEMOS LOLIN32
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#include <LinkedList.h>
#include <ArduinoJson.h>
#include <Metro.h>
#include <Streaming.h>

// Let's define a class to store everything we need to know about neighbor nodes.
class Node {
  public:
    String MAC; // MAC address, so we can match broadcasts
    int32_t RSSI; // signal strength; larger numbers are good.
    uint16_t dist; // ranking by RSSI
};

// and a list to store them in.
LinkedList<Node*> Neighbors = LinkedList<Node*>();

// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html
// C:\Users\MikeD\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4\libraries\WiFi\src\
// C:\Users\MikeD\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.4\tools\sdk\include\esp32

#define MAX_DATA_LEN 250

const int wifiChannel = 0;
const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
String myMAC, mySSID;
String prefixSSID = String("GaL_");

void onReceiveData(const uint8_t *mac, const uint8_t *data, int len) {

  // let's assume we're moving messages around by JSON
  StaticJsonDocument<MAX_DATA_LEN * 3> doc; // ESP-NOW does have a payload limit

  DeserializationError error = deserializeJson(doc, data, len);

  // Test if parsing succeeded.
  if (error) {
    Serial.print("deserializeMsgPack() failed: ");
    Serial.println(error.c_str());
    return;
  }
  serializeJson(doc, Serial);

  Serial << " <- ";

  char dataString[50] = {0};
  // SoftAP is hardware MAC +1, and we need the hardware MAC
  sprintf(dataString, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]
         );
  String recMAC = dataString;
  recMAC.toUpperCase();

  Serial << recMAC;
  Serial << endl;

  int x = doc["count"];
  String from = doc["senderSSID"];
  // etc...

  // check neighbors.
  boolean isNear = isNeighbor(recMAC);

  // blink
  if( isNear ) {
    static boolean state = false;
    state = !state;
    digitalWrite(BUILTIN_LED, state);
  }
}

boolean isNeighbor(String &recMAC) {  
  Node *node;
  for (int i = 0; i < Neighbors.size(); ++i) {
    node = Neighbors.get(i);  
    Serial << " " << node->MAC << " " << recMAC << endl;
    if( node->MAC.compareTo(recMAC) == 0) {
      Serial << "  Neighbor=" << node->dist << " RSSI=" << node->RSSI << endl;
      return(true);
    }
  }
  return(false);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  pinMode(BUILTIN_LED, OUTPUT);

//  btStop();

  //  WiFi.mode(WIFI_STA);
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();

  myMAC = String(WiFi.macAddress()); // hardware MAC +1
  Serial << "WiFi STA MAC: " << myMAC << " <<- will broadcast at this mac" << endl;

  mySSID = prefixSSID + myMAC;
  //  mySSID.replace(":", "");
  //  mySSID = prefixSSID + mySSID;
  int hidden = 0;
  // long password required or the AP will not start.
  int ret = WiFi.softAP(mySSID.c_str(), "8675309dontlosemynumber", wifiChannel, hidden);
  Serial << "Wifi softAP startup: " << ret << endl;
  Serial << "WiFi softAP SSID: " << mySSID << endl;

  // ESP32 STA_AP mode creates AP with MAC offset +1.
  Serial << "WiFi SoftAP MAC: " << WiFi.softAPmacAddress() << " <<- will make an AP at this mac" << endl;

  // ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(onReceiveData);

  // register peer
  esp_now_peer_info_t peerInfo;

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = wifiChannel;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

}

void findNeighbors() {
  Serial << "Neighbors.... ";

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
      boolean isGaL = String(WiFi.SSID(i)).startsWith(prefixSSID);
      
      if( isGaL ) {
        // add a new node.
        Node *newNeighbor = new Node();
        newNeighbor->MAC = String(WiFi.SSID(i));
        newNeighbor->MAC.replace(prefixSSID,"");
        newNeighbor->RSSI = WiFi.RSSI(i);
        Neighbors.add(newNeighbor);
      }
    }

    // What do we have?
    Neighbors.sort(compareRSSI);
    Node *node;
    Serial << "=\tN\tRSSI\tMAC" << endl;
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

void loop() {
  static Metro scanInterval(10000);
  if ( scanInterval.check() ) {
    findNeighbors();
  }

  static Metro sendInterval(1000);
  if ( sendInterval.check() ) {
    // send data
    static int x = 10;
    x++;

    StaticJsonDocument<MAX_DATA_LEN> doc;
    doc["senderSSID"] = mySSID;
    doc["count"] = x;

    static char buffer[MAX_DATA_LEN];
    serializeJson(doc, buffer);

    Serial << myMAC << " -> ";

    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &buffer, MAX_DATA_LEN);

    if (result == ESP_OK) {
      Serial << buffer << endl;
    }
    else {
      Serial.println("Error sending the data");
    }
  }

}
