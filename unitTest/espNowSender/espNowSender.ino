
#include <ESP8266WiFi.h>
#include <espnow.h>

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

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status from " );
  Serial.print(WiFi.macAddress());
  Serial.print(" = ");
  if (sendStatus == 0) {
    Serial.print("Delivery success");
  }
  else {
    Serial.print("Delivery fail");
  }
  Serial.print(" with size ");
  Serial.println(sizeof(myData));
}

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
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


  myData.controlRebroadcastCount = 5;
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
