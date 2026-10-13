// FlyghtEdge — 20 Flow rules: automation drawn in the browser, with a relay and a switch.
//
// The sketch only declares the parts: a relay "Lamp" and an input "Button". The rules are drawn
// on the dashboard under Devices > Flows, by connecting blocks, and are saved on the device:
//   1. Add a trigger "Widget changes" and pick "Button".
//   2. Add an action "Set widget" and pick "Lamp".
//   3. Wire the trigger to the action, turn the flow on, press Run to test it.
// Try the same with "Every N seconds", a delay, or a condition on a sensor value.
//
// Wiring:   relay board IN -> RELAY pin (active-high);  a push button between BUTTON_PIN and GND
//           (the BOOT button works for a test).
// Flows are a Pro feature: with no licence key the 30-minute evaluation turns them on.
// Blocks cannot drive pins that a module owns, nor the flash or USB pins.
//
// Status: compiled for ESP32-S3; not yet run on a board.
// Tools > Partition Scheme: "No FS 4MB (2MB APP x2)" or use the partitions.csv here.

#include <FlyghtEdge.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

// ---- pins (the wiring diagram in docs/guides is generated from this block) ----
#if CONFIG_IDF_TARGET_ESP32
const int RELAY = 25, BUTTON_PIN = 0;
#elif CONFIG_IDF_TARGET_ESP32S3
const int RELAY = 4, BUTTON_PIN = 0;
#else  // ESP32-C3, C6, S2 ...
const int RELAY = 5, BUTTON_PIN = 9;
#endif

fn::Device dev("flow-rules");
fn::Relay lamp("lamp", "Lamp");
fn::DigitalSensor button(BUTTON_PIN, "button", "Button");
fn::Flows flows;

void setup() {
  Serial.begin(115200);

  dev.features().defaultOn("flows");   // on at the first boot; Settings > Plan & features can change it

  lamp.channel(RELAY, "Lamp").maxOn(3600);
  button.activeLow().labels("PRESSED", "released").debounce(40);

  dev.use(lamp).use(button).use(flows);
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() { dev.loop(); }
