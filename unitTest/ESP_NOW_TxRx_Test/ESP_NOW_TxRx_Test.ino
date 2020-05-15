// For ESP32
#include <esp_now.h>
#include <WiFi.h>
// For ESP8266
//#include <espnow.h>
//#include <ESP8266WiFi.h>

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
    boolean isGaL; // does the SSID name suggest the neighbor is a node in GaL?
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

  static boolean state = false;
  state = !state;
  digitalWrite(BUILTIN_LED, state);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  pinMode(BUILTIN_LED, OUTPUT);

  //  WiFi.mode(WIFI_STA);
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();

  myMAC = String(WiFi.macAddress()); // hardware MAC +1
  Serial << "WiFi MAC: " << myMAC << endl;

  mySSID = prefixSSID + myMAC;
  //  mySSID.replace(":", "");
  //  mySSID = prefixSSID + mySSID;
  int hidden = 0;
  // long password required or the AP will not start.
  int ret = WiFi.softAP(mySSID.c_str(), "8675309dontlosemynumber", wifiChannel, hidden);
  Serial << "softAP startup: " << ret << endl;
  Serial << "softAP SSID: " << mySSID << endl;
  Serial << "softAP MAC: " << WiFi.softAPmacAddress() << endl;
//  Serial << "softAP hostname: " << WiFi.softAPgetHostname() << endl;

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

    for (int i = 0; i < n; ++i) {
      // this will align with MAC in ESP-NOW
      char dataString[50] = {0};
      // SoftAP is hardware MAC +1, and we need the hardware MAC
      sprintf(dataString, "%02X:%02X:%02X:%02X:%02X:%02X",
              WiFi.BSSID(i)[0], WiFi.BSSID(i)[1], WiFi.BSSID(i)[2],
              WiFi.BSSID(i)[3], WiFi.BSSID(i)[4], WiFi.BSSID(i)[5] - 1
             );
      String recMAC = dataString;
      recMAC.toUpperCase();

      // add a new node.
      Node *newNeighbor = new Node();
      newNeighbor->MAC = recMAC;
      newNeighbor->RSSI = WiFi.RSSI(i);
      newNeighbor->isGaL = String(WiFi.SSID(i)).startsWith(prefixSSID);
      Neighbors.add(newNeighbor);

      /*
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        // BSSID will be MAC+1, so careful
        //        Serial.print(" [");
        //        Serial.print(WiFi.BSSIDstr(i));
        //        Serial.print("]");
        Serial.print(" [");
        Serial.print(recMAC);
        Serial.print("]");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        delay(10);
        }

      */
    }

    // What do we have?
    Neighbors.sort(compareRSSI);
    Node *node;
    Serial << "=\tN\tG?\tRSSI\tMAC" << endl;
    for (byte i = 0; i < Neighbors.size(); i++) {
      node = Neighbors.get(i);
      node->dist = i;
      Serial << "\t" << node->dist;
      Serial << "\t" << node->isGaL;
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
