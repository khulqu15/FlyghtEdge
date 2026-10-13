// FlyghtEdge — 18 Home Assistant: every card appears in Home Assistant by itself.
//
// With the MQTT broker that Home Assistant uses, FlyghtEdge announces each widget through MQTT
// discovery: a toggle becomes a switch, a slider a number, a gauge or value a sensor. Nothing to write in
// Home Assistant's YAML. State goes to   <base>/state/<widget>   and commands come to   <base>/set/<widget>.
//
// Set BROKER, and the user and password of an MQTT user (Home Assistant: Settings > People, or the
// Mosquitto add-on). The broker, topic and Home Assistant switch can also be changed later in the
// dashboard under Connect > MQTT; what is saved there wins over the values below.
//
// Status: compiled for ESP32-S3; not yet run on a board.
// Tools > Partition Scheme: "No FS 4MB (2MB APP x2)" or use the partitions.csv here.

#include <FlyghtEdge.h>
#include <math.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

const char* BROKER = "mqtt://192.168.1.10:1883";   // your Home Assistant / Mosquitto host
const char* MQTT_USER = "homeassistant";
const char* MQTT_PASS = "change-me";

fn::Device dev("living-room");

void setup() {
  Serial.begin(115200);

  dev.mqtt().broker(BROKER).credentials(MQTT_USER, MQTT_PASS).baseTopic("flyght/{id}").homeAssistant(true);

  dev.page("Living room", "home")
      .toggle("lamp", "Lamp", [](bool on) { Serial.printf("lamp %d\n", on); }).icon("bulb")
      .slider("dim", "Brightness", 0, 100, 5, [](float v) { Serial.printf("dim %.0f\n", v); }).unit("%").icon("sun")
      .gauge("temp", "Temperature", "°C", 0, 40).icon("thermo");

  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 5000) return;
  last = millis();

  dev.ui().set("temp", 22.0f + 2.0f * sinf(millis() / 60000.0f));   // replace with a real sensor

  // Your own topic next to the automatic ones, e.g. a heartbeat for Home Assistant's availability.
  if (dev.mqtt().connected()) dev.mqtt().publish(dev.mqtt().base() + "/heartbeat", String(millis() / 1000));
}
