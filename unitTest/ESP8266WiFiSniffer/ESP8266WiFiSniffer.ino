#ifdef ESP8266
extern "C" {
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
}
#endif

int ap_channel = 1; // AP channel
char ap_mac[6] = { 0x34, 0xCD, 0xBE, 0x56, 0x64, 0xA0 }; // AP MAC

struct RxControl {
  signed rssi: 8;
  unsigned rate: 4;
  unsigned is_group: 1;
  unsigned: 1;
  unsigned sig_mode: 2;
  unsigned legacy_length: 12;
  unsigned damatch0: 1;
  unsigned damatch1: 1;
  unsigned bssidmatch0: 1;
  unsigned bssidmatch1: 1;
  unsigned MCS: 7;
  unsigned CWB: 1;
  unsigned HT_length: 16;
  unsigned Smoothing: 1;
  unsigned Not_Sounding: 1;
  unsigned: 1;
  unsigned Aggregation: 1;
  unsigned STBC: 2;
  unsigned FEC_CODING: 1;
  unsigned SGI: 1;
  unsigned rxend_state: 8;
  unsigned ampdu_cnt: 8;
  unsigned channel: 4;
  unsigned: 12;
};

struct LenSeq {
  uint16_t length;
  uint16_t seq;
  uint8_t address3[6];
};

struct sniffer_buf {
  struct RxControl rx_ctrl;
  uint8_t buf[36];
  uint16_t cnt;
  struct LenSeq lenseq[1];
};

struct sniffer_buf2 {
  struct RxControl rx_ctrl;
  uint8_t buf[112];
  uint16_t cnt;
  uint16_t len;
};
unsigned long time_ = 0;

// Callback function
void ICACHE_FLASH_ATTR promisc_cb(uint8 *buf, uint16 len) {
  if (len == 12) {
    struct RxControl *sniffer = (struct RxControl*) buf;
  } else if (len == 128) {
    struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
    int i = 0;
    // Check MAC
    for (i = 0; i < 6; i++) if (sniffer->buf[i + 10] != ap_mac[i]) return;
    Serial.printf("RSSI: %2d (%d ms)\n", sniffer->rx_ctrl.rssi, millis() - time_);
    time_ = millis();
  } else {
    struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
    int i = 0;
    // Check MAC
    for (i = 0; i < 6; i++) if (sniffer->buf[i + 10] != ap_mac[i]) return;
    Serial.printf("RSSI: %2d (%d ms)\n", sniffer->rx_ctrl.rssi, millis() - time_);
    time_ = millis();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(" -> WiFi Sniffer");
  Serial.println(" -> Initializing!\n\n");
  wifi_set_opmode(0x1);
  wifi_set_channel(ap_channel);
  wifi_promiscuous_enable(0);
  wifi_set_promiscuous_rx_cb(promisc_cb);
  wifi_promiscuous_enable(1);
  Serial.println(" -> Init finished!\n\n");
  time_ = millis();
}

void loop() {
  // Loop
}
