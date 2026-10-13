// FlyghtEdge — 22 Greenhouse: a DHT sensor, a fan and a pump, with an alarm that starts the fan.
//
// A complete small project:
//   * a DHT22 reads temperature and humidity every 5 s;
//   * when the temperature reaches 30 C the "alarm" card turns on and the fan relay starts; it stops
//     again at 27 C (hysteresis, so it does not chatter);
//   * the fan and the pump are also toggles on the dashboard, for manual control;
//   * the pump switches itself off after 5 minutes whatever you do.
//
// Wiring:   DHT22 data -> DHT_PIN (10 k pull-up to 3.3 V if the module has none), VCC 3.3 V, GND
//           relay board IN1 -> FAN_PIN, IN2 -> PUMP_PIN, relay VCC on its own supply, common GND.
// Put a flyback diode across every coil you drive yourself. Mains wiring is for qualified people only.
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
const int DHT_PIN = 4, FAN_PIN = 25, PUMP_PIN = 26;
#elif CONFIG_IDF_TARGET_ESP32S3
const int DHT_PIN = 4, FAN_PIN = 5, PUMP_PIN = 6;
#else  // ESP32-C3, C6, S2 ...
const int DHT_PIN = 4, FAN_PIN = 5, PUMP_PIN = 6;
#endif

fn::Device dev("greenhouse");
fn::Dht dht(DHT_PIN, fn::Dht::Type::Dht22);
fn::Relay outputs("relay", "Greenhouse");   // channel 0 = fan, channel 1 = pump

void setup() {
  Serial.begin(115200);

  outputs.channel(FAN_PIN, "Fan");
  outputs.channel(PUMP_PIN, "Pump").maxOn(300);

  // alarm on the first reading (temperature): on at 30 C, off at 27 C; the callback runs the fan
  dht.every(5000).alarm(30, 27, [](bool hot) { outputs.set(0, hot); });

  dev.use(dht).use(outputs);
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() { dev.loop(); }
