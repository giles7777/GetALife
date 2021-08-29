
#include <ESP8266WiFi.h>
#include <espnow.h>

// https://www.espressif.com/sites/default/files/documentation/esp-now_user_guide_en.pdf

// might look at https://github.com/yoursunny/WifiEspNow

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
  char a[32];
  int b;
  float c;
  String d;
  bool e;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Char: ");
  Serial.println(myData.a);
  Serial.print("Int: ");
  Serial.println(myData.b);
  Serial.print("Float: ");
  Serial.println(myData.c);
  Serial.print("String: ");
  Serial.println(myData.d);
  Serial.print("Bool: ");
  Serial.println(myData.e);
  Serial.println();
}

// https://www.esp32.com/viewtopic.php?t=13889
// /home/pi/.arduino15/packages/esp8266/hardware/esp8266/3.0.2/libraries/ESP8266WiFi
// https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-reference/wifi/esp_wifi.html

// https://github.com/espressif/esp-idf/blob/8e4a8e17037c51ce2452e55b5b75bbfaecb25838/components/esp32/include/esp_wifi_types.h#L192
typedef struct {
    signed rssi:8;            /**< signal intensity of packet */
    unsigned rate:5;          /**< data rate */
    unsigned :1;              /**< reserve */
    unsigned sig_mode:2;      /**< 0:is not 11n packet; 1:is 11n packet */
    unsigned :16;             /**< reserve */
    unsigned mcs:7;           /**< if is 11n packet, shows the modulation(range from 0 to 76) */
    unsigned cwb:1;           /**< if is 11n packet, shows if is HT40 packet or not */
    unsigned :16;             /**< reserve */
    unsigned smoothing:1;     /**< reserve */
    unsigned not_sounding:1;  /**< reserve */
    unsigned :1;              /**< reserve */
    unsigned aggregation:1;   /**< Aggregation */
    unsigned stbc:2;          /**< STBC */
    unsigned fec_coding:1;    /**< if is 11n packet, shows if is LDPC packet or not */
    unsigned sgi:1;           /**< SGI */
    unsigned noise_floor:8;   /**< noise floor */
    unsigned ampdu_cnt:8;     /**< ampdu cnt */
    unsigned channel:4;       /**< which channel this packet in */
    unsigned :12;             /**< reserve */
    unsigned timestamp:32;    /**< timestamp */
    unsigned :32;             /**< reserve */
    unsigned :32;             /**< reserve */
    unsigned sig_len:12;      /**< It is really lenth of packet */
    unsigned :12;             /**< reserve */
    unsigned rx_state:8;      /**< rx state */
} wifi_pkt_rx_ctrl_t;

typedef struct {
    wifi_pkt_rx_ctrl_t rx_ctrl;
    uint8_t payload[0];       /**< ieee80211 packet buff, The length of payload is described by sig_len */
} wifi_promiscuous_pkt_t;

typedef enum {
    WIFI_PKT_CTRL,  /**< control type, receive packet buf is wifi_pkt_rx_ctrl_t */
    WIFI_PKT_MGMT,  /**< management type, receive packet buf is wifi_promiscuous_pkt_t */
    WIFI_PKT_DATA,  /**< data type, receive packet buf is wifi_promiscuous_pkt_t */
    WIFI_PKT_MISC,  /**< other type, receive packet buf is wifi_promiscuous_pkt_t */
} wifi_promiscuous_pkt_type_t;

void promiscuous_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {

  // All espnow traffic uses action frames which are a subtype of the mgmnt frames so filter out everything else.
  if (type != WIFI_PKT_MGMT)
    return;

  static const uint8_t ACTION_SUBTYPE = 0xd0;
  static const uint8_t ESPRESSIF_OUI[] = {0x18, 0xfe, 0x34};

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  // Only continue processing if this is an action frame containing the Espressif OUI.
  if ((ACTION_SUBTYPE == (hdr->frame_ctrl & 0xFF)) &&
      (memcmp(hdr->oui, ESPRESSIF_OUI, 3) == 0)) {

    int rssi = ppkt->rx_ctrl.rssi;
    Serial.print("RSSI: ");
    Serial.println(rssi);
  }
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(OnDataRecv);

  // and set a sniffer so we can get at the RSSI info in the header
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&promiscuous_rx_cb);
}

void loop() {

}
