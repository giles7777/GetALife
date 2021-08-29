// For ESP8266 Boards (definitions: https://arduino.esp8266.com/stable/package_esp8266com_index.json)
// Board: LOLIN(WEMOS) D1 mini Pro
// CPU Frequency: 80 MHz
// Flash Size: 16 Mb (~1Mb OTA)

// Wiring (updated 2/14/21)
// Used: A0:batt, D0:sleep, D4:builtin led, D5:buzzer, D7:LED, D2:PIR, D6:(reserved)
//
// D1 mini pro: bridge BAT-A0, bridge SLEEP; connect +5, +3.3, GND, D5, D7.
// LED: bridge D7 (not D4 default); connect +5, +3.3, GND, D7.
// Buzzer: none (D5 default);  connect +5, +3.3, GND, D5.
///
// I2C Cable: D1 (SCL) D2 (SDA)
//    PIR: bridge D2 (not D3 default)
//
// deprecated:
// Relay: bridge D6 (not D1 default)

// Flash Size: 16 Mb (~1Mb OTA)

// connect to MQTT broker with http://www.hivemq.com/demos/websocket-client/
// broker.mqttdashboard.com

// subscribe to topics:
// GaL-in/#
// GaL-out/#

// https://gitlab.com/painlessMesh/painlessMesh/-/wikis/home

// libraries used in the modules need to be included here.
#include <painlessMesh.h>
// which also includes
//#include <TaskScheduler.h>
//#include <ESPAsyncTCP.h>
// we use also:
#include <ArduinoJson.h> // everyone
#include <Streaming.h> // everyone
#include <StreamUtils.h> // power
#include <EEPROM.h> // power
#include <NonBlockingRtttl.h> // sound
#include <Metro.h> // sound

// modules definitions
#include "Light.h"
#include "Motion.h"
#include "Network.h"
#include "Power.h"
#include "Sound.h"

// module instantiation
Light light;
Motion motion;
Network network;
Power power;
Sound sound;

// prototype definition for mesh callback handling
void handleMeshMessage(uint32_t from, String & msg);

void setup() {
  delay(300); // don't delete this; a pause at startup can help unbrick a device
  Serial.begin(115200);
  Serial.setTimeout(10);
  Serial << endl << endl;
  Serial << "*** setup() begins ***" << endl;

  // useful random number initiation
  randomSeed(ESP.getChipId());

  // start our modules
  light.begin();
  Serial << "setup() light status " << light.getStatus() << endl;
  motion.begin();
  Serial << "setup() motion status " << motion.getStatus() << endl;
  network.begin();
  Serial << "setup() network status " << network.getStatus() << endl;
  power.begin();
  Serial << "setup() power status " << power.getStatus() << endl;
  sound.begin();
  Serial << "setup() sound status " << sound.getStatus() << endl;

  Serial << "Send '?' on the serial for status." << endl;
  Serial << "Send '{\"get\":\"<module>\"}' on the serial for transmission spoofs" << endl;
  Serial << "*** setup() complete ***" << endl;
}

void loop() {
  // each module is expecte to provide an update method
  light.update();
  motion.update();
  network.update();
  power.update();
  sound.update();

  // we can also spoof messgaes from the serial line
  if (Serial.available()) {
    String msg = Serial.readString();
    handleMeshMessage(network.getMyNodeId().toInt(), msg);
  }

  static Metro updateEach(5000);

  if( updateEach.check() ) {
    Serial << "Heap: " << ESP.getFreeHeap() << endl;  
    updateEach.reset();
  }
}

void handleMeshMessage(uint32_t from, String & msg) {
  // each module is expected to process a message and, optionally, act on it.
  Serial << "handleMeshMessage(): from " << from << " msg " << msg << endl;

  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, msg);
  if (!err) {
    // we can process requests from a specific node, replying with a status update per module
    String retMsg = "";
    if ( doc["get"] == "light" ) retMsg = light.getStatus();
    else if ( doc["get"] == "motion" ) retMsg = motion.getStatus();
    else if ( doc["get"] == "network" ) retMsg = network.getStatus();
    else if ( doc["get"] == "power" ) retMsg = power.getStatus();
    else if ( doc["get"] == "sound" ) retMsg = sound.getStatus();
    if( retMsg != "" ) {
      network.sendMessage(from, retMsg);
      return;
    } 
  }

  // check directives
  light.setStatus(msg);
  motion.setStatus(msg);
  network.setStatus(msg);
  power.setStatus(msg);
  sound.setStatus(msg);
}
