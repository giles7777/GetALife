// Nodes are designed to be really dumb, because we STRONLY wish to avoid OTA programming to adjust them.
// If you can think of ANY way to solve a problem with Head instead of adjusting Node, do so.

// https://github.com/PhracturedBlue/ESP8266MQTTMesh/tree/master/examples/ESP8266MeshHelloWorld

#include <ESP8266MQTTMesh.h>
#include <FS.h>
#include <Streaming.h>
#include <LinkedList.h>
#include <Metro.h>

#ifndef LED_PIN
#define LED_PIN LED_BUILTIN
#endif

// upstream network
wifi_conn    networks[]       = { \
                                  WIFI_CONN("Looney", "TinyandTooney", NULL, 0), \
                                  NULL, \
                                };

// local mesh
const char*  mesh_ssid        = "getalife";
const char*  mesh_password    = "getalife";
const int    mesh_port        = 1884;

// change to locally defined broker at some point
const char*  mqtt_server      = "broker.hivemq.com";
const int    mqtt_port        = 1883;
const char*  mqtt_in_topic    = "GaL-in/";
const char*  mqtt_out_topic   = "GaL-out/";

// for OTA
#define      FIRMWARE_ID        0x1337
#define      FIRMWARE_VER       "0.1"

// me
String ID  = String(ESP.getChipId(), 16); // in hex; importatnt to match with library

// Note: All of the '.set' options below are optional.  The default values can be
// found in ESP8266MQTTMeshBuilder.h
ESP8266MQTTMesh mesh = ESP8266MQTTMesh::Builder(networks, mqtt_server, mqtt_port)
                       .setVersion(FIRMWARE_VER, FIRMWARE_ID)
                       .setMeshSSID(mesh_ssid)
                       .setMeshPassword(mesh_password)
                       .setMeshPort(mesh_port)
                       .setTopic(mqtt_in_topic, mqtt_out_topic)
                       .build();
void setup() {

  Serial.begin(115200);
  delay(1000); //This is only here to make it easier to catch the startup messages.  It isn't required
  Serial.println();

  Serial << "Startup with ID=" << ID << endl;

  randomSeed(ESP.getChipId());
  
  mesh.setCallback(callback);
  mesh.begin();

  pinMode(BUILTIN_LED, OUTPUT);

  Serial.println("Setup complete.");
}


void loop() {

  if (! mesh.connected())
    return;

  static Metro helloInterval((uint32_t)random(30, 90) *1000UL);
  if (helloInterval.check()) {
    static uint32_t cnt = 0;
    String msg = ID + " cnt: " + cnt++;
    mesh.publish("hello", msg.c_str());
  }
}

void callback(const char *topic, const char *msg) {

  String Topic = String(topic);
  String Msg = String(msg);

  if( Topic.endsWith("onboardLED") ) doOnboardLED(Topic, Msg);
  if( Topic.endsWith("ping") ) doPing(Topic, Msg);
}

void doOnboardLED(String Topic, String Msg) {
  digitalWrite(BUILTIN_LED, Msg.toInt());

  mesh.publish("onboardLED", Msg.c_str());
}

void doPing(String Topic, String Msg) {
  static uint32_t cnt = 0;
  String msg = String(cnt++);
  
  mesh.publish("ping", msg.c_str());
}
