#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
//#include "ieee80211_structs.h"
#include <Streaming.h>
#include <Metro.h>
#include <FastLED.h>
#include <LinkedList.h>


// https://www.espressif.com/sites/default/files/documentation/esp-now_user_guide_en.pdf

// REPLACE WITH RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
byte wifiChannel = 1;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  uint8_t controlVersion;
  uint8_t controlRebroadcastCount;

  uint8_t gameState;
  uint8_t gameRule;
  uint8_t gameGenerationInterval;

  uint8_t lightBrightness;
  uint8_t lightSparkleRate;
  uint8_t lightPaletteIndex;

  uint8_t soundPlayRate;
  uint8_t soundSongIndex;
} struct_message;

// Create a struct_message called myData
struct_message myData;

unsigned long lastTime = 0;
unsigned long timerDelay = 2000;  // send readings timer

// for display
char addrCharBuff[] = "00:00:00:00:00:00\0";

void mac2str(const uint8_t* ptr, char* string) {
  sprintf(string, "%02x:%02x:%02x:%02x:%02x:%02x", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
}

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  mac2str(mac_addr, addrCharBuff);

  Serial << "Sent: " << WiFi.macAddress() << " -> " << addrCharBuff << " = ";

  if (sendStatus == 0) Serial << "OK" << endl;
  else Serial << "FAIL" << endl;
}

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac_addr, uint8_t *incomingData, uint8_t len) {
  mac2str(mac_addr, addrCharBuff);

  Serial << "Received: " << WiFi.macAddress() << " <- " << addrCharBuff << " = ";
  Serial << len << endl;
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  // Register "peer" or we can't send to it.
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, wifiChannel, NULL, 0);

  // set me up
  myData.controlVersion = 1;
  myData.controlRebroadcastCount = 0;
  myData.gameState = random8(1);
  myData.gameRule = 101;
  myData.gameGenerationInterval = 5;
  myData.lightBrightness = 255;
  myData.lightSparkleRate = 50;
  myData.lightPaletteIndex = 1;
  myData.soundPlayRate = 1;
  myData.soundSongIndex = 1;
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Set values to send
    myData.gameState = !myData.gameState;

    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    lastTime = millis();
    myData.controlRebroadcastCount = 0;
  }
}
