#include <EspNowFloodingMesh.h>

#define ESP_NOW_CHANNEL 1
//AES 128bit
unsigned char secretKey[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

void espNowFloodingMeshRecv(const uint8_t *data, uint32_t len) {
  if (len > 0) {
    Serial.println((const char*)data);
  }
}
#include <EspNowFloodingMesh.h>

#define ESP_NOW_CHANNEL 1
#define ESP_NOW_BSID 0x112233
//AES 128bit
unsigned char secredKey[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

void espNowFloodingMeshRecv(const uint8_t *data, int len, uint32_t replyPrt) {
  if (len > 0) {
    Serial.print("Rec: ");
    Serial.println((const char*)data);
  }

}

void setup() {
  delay(100);

  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Setup. start.");

  pinMode(BUILTIN_LED, OUTPUT);

  //Set device in AP mode to begin with
  espNowFloodingMesh_RecvCB(espNowFloodingMeshRecv);
  espNowFloodingMesh_secredkey(secredKey);
  espNowFloodingMesh_begin(ESP_NOW_CHANNEL, ESP_NOW_BSID);
  espNowFloodingMesh_setToMasterRole(true, 3); //Set ttl to 3.

  Serial.println("Setup. end.");
}

void loop() {
  static unsigned long m = millis();
  if (m + 5000 < millis()) {
    char message[] = "MASTER HELLO MESSAGE";
    espNowFloodingMesh_send((uint8_t*)message, sizeof(message));
    m = millis();

    static boolean ledMode = false;
    ledMode = !ledMode;
    digitalWrite(BUILTIN_LED, ledMode);
  }
  espNowFloodingMesh_loop();
  delay(10);
}
