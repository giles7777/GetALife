#include "Network.h"

int wifiChannel = 9;
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void Network::begin() {
  Serial << "Network::begin()" << endl;
  
  // https://arduino-esp8266.readthedocs.io/en/latest/reference.html
  pinMode(PIN_LED, OUTPUT);

  //  WiFi.mode(WIFI_STA);
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();

  // ESP8266 makes a new MAC for AP mode, and we'll send ESP-NOW packets from that MAC
  myMAC = String(WiFi.softAPmacAddress());
  mySSID = prefixSSID + myMAC;
  
  int hidden = 0;
  // long password required or the AP will not start.
  int ret = WiFi.softAP(mySSID.c_str(), "8675309dontlosemynumber", wifiChannel, hidden);

  Serial << "WiFi softAP startup: " << ret << endl;
  Serial << "WiFi softAP SSID: " << mySSID << endl;
  Serial << "WiFi SoftAP MAC: " << myMAC << " <<- will make an AP at this mac" << endl;

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onReceiveData);

  // register peer
  if (esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, wifiChannel, NULL, 0) != ESP_OK ) {
    Serial.println("Failed to add peer");
    return;
  }

}

// ESP-NOW inbound packet handler
uint8_t rcvBuffer[MAX_DATA_LEN];
String rcvMAC;
boolean rcvNew = false;

void onReceiveData(uint8_t *mac, uint8_t *data, uint8_t len) {
  // this function is called from a high-priority WiFi stack, and needs to be
  // fast.  copy out the message and flag for later processing in update.

  // SoftAP is hardware MAC +1, and we need the hardware MAC
  char dataString[50] = {0};
  sprintf(dataString, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2],
          mac[3], mac[4], mac[5]
         );
  rcvMAC = dataString;
  rcvMAC.toUpperCase();

  memcpy(rcvBuffer, data, len);

  rcvNew = true;
}

boolean Network::update() {

  if ( rcvNew ) {
    memcpy(inData, rcvBuffer, MAX_DATA_LEN);
    inFrom = rcvMAC;
    rcvNew = false;

    toggleLED();

    return (true);
  }
  return (false);
}

void Network::send() {
  esp_now_send(broadcastAddress, outData, MAX_DATA_LEN);

  toggleLED();
}

void Network::toggleLED() {
  static boolean state = false;
  state = !state;
  digitalWrite(BUILTIN_LED, state);
}
