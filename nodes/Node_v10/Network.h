#ifndef Network_h
#define Network_h

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <espnow.h>

#include <LinkedList.h>
#include <ArduinoJson.h>

#include <Streaming.h>

// https://randomnerdtutorials.com/esp-now-esp8266-nodemcu-arduino-ide/
// p.74 https://www.espressif.com/sites/default/files/documentation/2c-esp8266_non_os_sdk_api_reference_en.pdf
// https://www.espressif.com/sites/default/files/documentation/esp-now_user_guide_en.pdf

// C:\Users\MikeD\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.7.1/tools/sdk/include/espnow.h

#define ESP_OK 0

// C:\Users\MikeD\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.7.1/tools/sdk/include/
#define MAX_DATA_LEN 250

#define PIN_LED BUILTIN_LED

class Network {
  public:
    void begin();

    // if this function returns TRUE
    boolean update();
    // then contents are stored here
    uint8_t inData[MAX_DATA_LEN]; // contents
    String inFrom; // MAC address  

    // load broadcast payload here:
    uint8_t outData[MAX_DATA_LEN]; // contents
    // and send
    void send();

    String prefixSSID = String("GaL_");
    
  private:
    // blinky
    void toggleLED();

    String myMAC, mySSID;

};

// ESP-NOW inbound packet handler
void onReceiveData(uint8_t *mac, uint8_t *data, uint8_t len);

#endif
