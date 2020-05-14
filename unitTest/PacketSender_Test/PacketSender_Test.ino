#include "freertos/FreeRTOS.h"

#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"

#include "nvs_flash.h"
#include "string.h"
#include "driver/gpio.h"

#include <Streaming.h>
#include <Metro.h>

uint8_t channel = 10; // relatively clear channel?

/*
   This is the (currently unofficial) 802.11 raw frame TX API,
   defined in esp32-wifi-lib's libnet80211.a/ieee80211_output.o

   This declaration is all you need for using esp_wifi_80211_tx in your own application.
*/
esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

static wifi_country_t wifi_country = {.cc = "CN", .schan = 1, .nchan = 13}; //Most recent esp32 library struct

typedef struct {
  unsigned frame_ctrl: 16;
  unsigned duration_id: 16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl: 16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_sender_init(void);
static void wifi_sender_set_channel(uint8_t channel);
static const char *wifi_sender_packet_type2str(wifi_promiscuous_pkt_type_t type);

esp_err_t event_handler(void *ctx, system_event_t *event) {
  return ESP_OK;
}

void send_packet() {
  // a place for everything under the sun
  uint8_t buffer[1024];

  // break it on down
  wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buffer;
  wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  // packet type: data=0x80,0x00?
  hdr->frame_ctrl = (0x80 << 8);
  
  // sender
  hdr->addr2[0] = 8; hdr->addr2[1] = 6; hdr->addr2[2] = 7;
  hdr->addr2[3] = 5; hdr->addr2[4] = 3; hdr->addr2[5] = 9;

  // recipient
  hdr->addr1[0] = 255; hdr->addr1[1] = 255; hdr->addr1[2] = 255;
  hdr->addr1[3] = 255; hdr->addr1[4] = 255; hdr->addr1[5] = 255;

  // payload
  char payload[255];
  strcpy(payload, "Testing");
  ipkt = (wifi_ieee80211_packet_t *)payload;

  // send
  esp_wifi_80211_tx(WIFI_IF_AP, buffer, sizeof(wifi_promiscuous_pkt_t) + strlen(payload), false);

}

void wifi_sender_set_channel(uint8_t channel)
{
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

void wifi_sender_init(void)
{
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

//  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );

  // set dummy AP to specify a channel and get WiFi hardware up
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  wifi_config_t ap_config = {};
  strcpy((char*)ap_config.ap.ssid, "esp32-beaconspam");
  ap_config.ap.ssid_len = 0;
  strcpy((char*)ap_config.ap.password, "dummypassword");
  ap_config.ap.channel = 10;
  ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
  ap_config.ap.ssid_hidden = 1;
  ap_config.ap.max_connection = 4;
  ap_config.ap.beacon_interval = 60000;

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

  esp_wifi_set_promiscuous(true);
//  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial << endl << endl << "setup...";
  wifi_sender_init();
  Serial << "... complete" << endl;
  send_packet();
}

void loop() {
}
