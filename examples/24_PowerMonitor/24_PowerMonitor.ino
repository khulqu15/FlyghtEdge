// FlyghtEdge — 24 Power monitor: mains voltage, current, power and energy, with a cut-off.
//
// A PZEM-004T v3 module measures an AC load (up to 100 A with its coil, 10 A on the small module) and
// reports over a UART: voltage, current, power, energy, frequency and power factor, each a card.
//   * an over-voltage alarm turns on at 250 V and off at 245 V;
//   * if the current stays above LIMIT_A, the contactor relay is opened (the load is cut) and stays
//     open until you close it again from the dashboard.
//
// Wiring:   PZEM TX -> PZEM_RX pin, PZEM RX -> PZEM_TX pin, PZEM 5 V and GND (the module is opto-isolated
//           from the mains side; keep the AC terminals covered). Contactor relay IN -> RELAY pin.
// MAINS VOLTAGE IS LETHAL. Use a certified enclosure and a qualified electrician. Energy is shown in kWh.
//
// This is monitoring and a convenience cut-off, not a protection device: use a breaker or fuse sized
// for the circuit.
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
const int PZEM_RX = 16, PZEM_TX = 17, RELAY = 25;
#elif CONFIG_IDF_TARGET_ESP32S3
const int PZEM_RX = 18, PZEM_TX = 17, RELAY = 4;
#else  // ESP32-C3, C6, S2 ...
const int PZEM_RX = 10, PZEM_TX = 0, RELAY = 5;
#endif

const float LIMIT_A = 8.0f;   // cut the load above this current

fn::Device dev("power-monitor");
fn::Pzem pzem(PZEM_RX, PZEM_TX);
fn::Relay contactor("load", "Load");

void setup() {
  Serial.begin(115200);

  contactor.channel(RELAY, "Load").bootOn();   // the load is on at start; the current check can open it
  pzem.every(2000).alarm(250, 245);            // alarm on the first reading: voltage

  dev.use(pzem).use(contactor);
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 1000) return;
  last = millis();

  const float a = pzem.amps();
  if (!isnan(a) && a > LIMIT_A && contactor.isOn(0)) {
    contactor.set(0, false);
    Serial.printf("[power] %.1f A above the limit, load cut\n", a);
  }
}
