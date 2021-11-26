#include <MeshGnome.h>
#include <Arduino.h> // basics
#include <ESP8266WiFi.h> // wifi 
#include <espnow.h> // esp-now
#include "ieee80211_structs.h" // RSSI
#include <Streaming.h> // ease outputs
#include <FastLED.h> // lights
#include <map>
#include <Schedule.h>

// This simple example just blinks the builtin LED.  Upload this code
// to multiple nodes, and they will synchronize via WiFi.
//
// As a demonstration of MeshSyncMem, you can enter a new number in
// the Arduino Monitor.  This will set the number of blinks between
// pauses to the value you specify, and propagate it to all other
// nodes.
//
// As a demonstration of MeshSyncSketch, you can increase the version
// number passed to sketchUpdate.  If you upload this to one node, the
// new sketch will be propagated to all other reachable nodes.  To
// observe this via the builtin LED, you could also toggle the value
// of ON_IS_LOW so that blinks will be on instead of off or vice
// versa.
// builtin

#define LED_OFF HIGH
#define LED_ON LOW

// wifi
#define WIFI_CHAN     1
#define MAC_SIZE      6

// lights
// https://www.wemos.cc/en/latest/d1_mini_shiled/rgb_led.html
#define LED_TYPE      WS2812B
#define COLOR_ORDER   GRB
#define NUM_LEDS      7
#define PIN_RGB       D7  // NOT default (which is D4 aka BUILTIN_LED; doh!)
// builtin
#define LED_OFF HIGH
#define LED_ON LOW

uint8_t myAddress[MAC_SIZE], senderAddress[MAC_SIZE];
uint8_t broadcastAddress[MAC_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct EthStruct {
  uint8_t ethaddr[6];

  bool operator<(const EthStruct& rhs) const { return memcmp(ethaddr, rhs.ethaddr, 6) < 0; }
  bool operator==(const EthStruct& rhs) const { return memcmp(ethaddr, rhs.ethaddr, 6) == 0; }
};

struct LocalShareData {
  int zero_or_one = 0;
};

MeshSyncMem blinkcount;
MeshSyncMem constant;
MeshSyncSketch sketchUpdate(230 /* sketch version */);
MeshSyncTime syncedTime;
LocalPeriodicStruct<LocalShareData> shareLocal;
DispatchProto protos[] = {  //
  {1 /* protocol id */, &sketchUpdate},
  {2 /* protocol id */, &blinkcount},
  {3 /* protocol id */, &syncedTime},
  {4 /* protocol id */, &shareLocal},
  };

CRGB leds[NUM_LEDS];
bool updatingCode = false;
bool transmittingCode = false;
size_t transmitOffset = 0;
size_t transmitLength = 1;
size_t lastTransmitOffset = 0;
size_t updateOffset = 0;
size_t updateLength = 0;

std::map<EthStruct, LocalShareData> othersData;
int cur_zero_or_one = 0;

// If true, set LED_BUILTIN to LOW during blinks; otherwise set it to HIGH.
static constexpr bool ON_IS_LOW = true;

void builtinOff() {
  digitalWrite(BUILTIN_LED, LED_OFF);
}

void builtinOn() {
  digitalWrite(BUILTIN_LED, LED_ON);
}

void blinkBuiltin() {
  static boolean builtInLED = false;
  builtInLED = ! builtInLED;
  builtInLED ? builtinOn() : builtinOff();
}


void rssiHandler(const uint8_t* src, int8_t rssi) {
  //printf("I received a packet with rssi %d!\n", rssi);   // might break ota
}

void transmitProgressHandler(size_t offset, size_t length) {
  if (length > 0) {
    if (offset >= length) {
      transmittingCode = false;
    } else {
      transmittingCode = true;
    }
  }
  transmitOffset = offset;
  transmitLength = length;
}

void setup() {
  delay(300);
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // lights
  FastLED.addLeds<LED_TYPE, PIN_RGB, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  EspSnifferProtoDispatch.setRSSIHook(rssiHandler);
  sketchUpdate.setTransmitProgressHook(transmitProgressHandler);
  EspSnifferProtoDispatch.begin(protos);
  Serial.printf("BlinkCount startup complete, running sketch version %d\n",
                sketchUpdate.localVersion());


  static char buff[5];
  sprintf(buff,"%d",1);

  sketchUpdate.setReceiveProgressHook([](size_t offset, size_t length) {
    updateOffset = offset;
    updateLength = length;

    //Serial.printf("Got progress: offset: %d len: %d\n",updateOffset,updateLength);

    if (updateOffset < updateLength) {
      updatingCode = true;
    } else {
      updatingCode = false;
    }
  });

  sketchUpdate.setUpdateStopHook([](String reason) {
    Serial << "Stopped updating code: " << reason << endl;
    updateLength = 0;
    updatingCode = false;
  });
  
  blinkcount.update(0,buff);
  constant.update(0,"CONSTANT");

  for (byte i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
  FastLED.show();

  shareLocal.setTimestepHook([]() {
    if (updatingCode) {
      Serial.printf("Receiving: %d / %d\n", updateOffset, updateLength);
      byte show = 1 + (updateOffset * (NUM_LEDS - 1) / updateLength);

      for (byte i = 0; i < show; i++) leds[i] = CRGB::Red;
      for (byte i = show; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
      FastLED.show();
    } else if (transmitLength > 0 && transmittingCode) {
      Serial.printf("Transmitting: %d / %d\n", transmitOffset, transmitLength);
      byte show = 1 + (transmitOffset * (NUM_LEDS - 1) / transmitLength);

      for (byte i = 0; i < show; i++) leds[i] = CRGB::Blue;
      for (byte i = show; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
      FastLED.show();

      transmitOffset = transmitLength = 0;
    } else {
      size_t numOthers = othersData.size();
      size_t numOthersOnes = 0;
      for (const auto& others : othersData) {
        if (others.second.zero_or_one == 1) {
          ++numOthersOnes;
        }
      }

      CRGB col;
      if (cur_zero_or_one) {
        col.raw[0] = 0;
        col.raw[1] = 0;
        if (numOthers) {
          col.raw[2] = numOthersOnes * 255 / numOthers;
        } else {
          col.raw[2] = 0;
        }
        printf("raw[2] is %d\n", col.raw[2]);
      } else {
        if (numOthers) {
          col.raw[0] = (numOthers - numOthersOnes) * 255 / numOthers;
        } else {
          col.raw[1] = 0;
        }
        printf("raw[0] is %d\n", col.raw[0]);
        col.raw[1] = 0;
        col.raw[2] = 0;
      }
      static uint32_t lastCheck = 0;
      uint32_t now = millis();
      printf("%d neighbors seen, %u ms after last check\n", othersData.size(), now - lastCheck);
      lastCheck = now;
      for (byte i = 0; i < NUM_LEDS; i++) {
        if (i <= numOthers) {
          leds[i] = col;
        } else {
          leds[i] = CRGB::Black;
        }
      }
      FastLED.show();
      othersData.clear();
      cur_zero_or_one = random(0, 2);
    }
  });
  shareLocal.setReceivedHook([](const ProtoDispatchPktHdr* hdr, const LocalShareData& val) {
    EthStruct key;
    memcpy(key.ethaddr, hdr->src, 6);
    othersData.emplace(key, val);
  });
  shareLocal.setFillForTransmitHook([](LocalShareData* val) -> bool {
    val->zero_or_one = cur_zero_or_one;
    return true;
  });
  shareLocal.begin(&syncedTime, 2000 /* run every 2000 ms */);

  syncedTime.setAdjustHook([](int32_t adjustment) {
    schedule_function([=]() { Serial.printf("Time adjusted: %d\n", adjustment); });
  });
  syncedTime.setJumpHook([](int32_t adjustment) {
    schedule_function([=]() { Serial.printf("Time JUMPED: %d\n", adjustment); });
  });
  syncedTime.setReceiveHook(
      [](const ProtoDispatchPktHdr* hdr, int32_t offset, uint32_t syncedDuration) {
        String peer = etherToString(hdr->src);
        schedule_function([=]() {
          Serial.printf("Time of %s offset=%d duration=%u\n", peer.c_str(), offset, syncedDuration);
        });
      });
}

// Number of blinks remaining before pausing and resetting.
int blinkCountLeft = 0;

void blinkLED() {
  static bool on = false;
  static uint32_t lastChange = 0;
  if ((uint32_t(millis()) - lastChange) < 300) {
    return;
  }
  Serial.print(".");
  lastChange = millis();

  if (on) {
    digitalWrite(LED_BUILTIN, ON_IS_LOW ? LOW : HIGH);
    on = false;
    return;
  }

  --blinkCountLeft;
  if (blinkCountLeft >= 0) {
    digitalWrite(LED_BUILTIN, ON_IS_LOW ? HIGH : LOW);
    on = true;
    return;
  }

  if (blinkCountLeft < -4) {
    blinkCountLeft = blinkcount.localMetadata().toInt();
    //printf("Blinking %d times\n", blinkCountLeft);
  }
}

void loop() {
  blinkLED();
  EspSnifferProtoDispatch.espTransmitIfNeeded();
  
  if (Serial.available()) {
    int newBlinks = Serial.parseInt();
    if (!newBlinks) {
      return;
    }
    String oldBlinks = blinkcount.localMetadata();

    int oldVersion = blinkcount.localVersion();
    Serial.printf("Ok, blinks %s -> %d, configuration version=%d -> %d\n",
                  oldBlinks ? oldBlinks.c_str() : "(none)", newBlinks, oldVersion, oldVersion + 1);
                      
    blinkcount.update(oldVersion + 1, String(newBlinks));
  }

  EVERY_N_MILLISECONDS( 5000 ) {
    if (lastTransmitOffset == transmitOffset) {
      transmittingCode = false;
    }
    lastTransmitOffset = transmitOffset;
  }

  shareLocal.run();
}
