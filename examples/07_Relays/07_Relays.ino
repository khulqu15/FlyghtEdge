// FlyghtEdge — 07 Relays: a relay board with up to 16 channels, every pin movable from the dashboard.
//
// Each channel is a card on the "Relays" page. An admin opens the three dots on a card > Settings to
// move a pin without recompiling; the saved pin wins over the one below. The dialog only offers pins
// that can work (no flash or USB pins, no input-only pins for an output) and asks for the device name
// before it moves a pin that switches a load.
//
//   Pump      active-high board, switches itself off after 10 minutes
//   Valve     active-low (low-level trigger) board
//   Gate      latching (bistable) relay: one pin sets it, another resets it
//   Forward / Reverse   interlock group 1: they are never on at the same time
//
// Default pins are picked per chip; change them to match your wiring.
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

#if CONFIG_IDF_TARGET_ESP32
const int PUMP = 4, VALVE = 16, GATE_SET = 17, GATE_RESET = 18, FWD = 19, REV = 21;
#else
const int PUMP = 4, VALVE = 5, GATE_SET = 6, GATE_RESET = 7, FWD = 15, REV = 16;
#endif

fn::Device dev("relay-board");
fn::Relay relays("relay", "Relay board");

void setup() {
  Serial.begin(115200);
  relays.channel(PUMP, "Pump").maxOn(600);
  relays.channel(VALVE, "Valve", fn::Relay::Type::ActiveLow);
  relays.latching(GATE_SET, GATE_RESET, "Gate");
  relays.channel(FWD, "Forward").interlock(1);
  relays.channel(REV, "Reverse").interlock(1);

  dev.license(FN_LICENSE_KEY);
  dev.use(relays);
  dev.begin();
}

void loop() { dev.loop(); }
