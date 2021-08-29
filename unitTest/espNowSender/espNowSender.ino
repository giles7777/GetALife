
#include <ESP8266WiFi.h>
#include <espnow.h>

// https://www.espressif.com/sites/default/files/documentation/esp-now_user_guide_en.pdf

// REPLACE WITH RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
byte wifiChannel = 1;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  byte a,b,c;
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

  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_COMBO, wifiChannel, NULL, 0);
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    // Set values to send
    myData.a=1;
    myData.b=250;
    myData.c=42;

    
    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    lastTime = millis();
  }
}
