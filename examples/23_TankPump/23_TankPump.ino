// FlyghtEdge — 23 Tank pump: keep a water tank between two levels.
//
// An ultrasonic sensor on top of the tank measures the distance to the water. The sketch turns that
// into a level in percent and runs the pump like this (when "Auto" is on):
//   level below START_BELOW  -> pump on
//   level above STOP_ABOVE   -> pump off
// Two things protect the pump: a float switch in the SOURCE (it must report water, or the pump is
// kept off: dry running destroys it) and a maximum run time of 20 minutes. If the pump stops because
// of that limit, Auto is switched off and the card says so: something is wrong (a leak, an empty
// source), and the sketch does not start it again by itself.
//
// Wiring:   ultrasonic TRIG -> TRIG_PIN, ECHO -> ECHO_PIN through a divider (echo is 5 V), VCC 5 V;
//           float switch between SOURCE_PIN and GND (closed = water present); pump relay IN -> PUMP_PIN.
// "Empty" and "full" distances are measured from the sensor: set EMPTY_CM and FULL_CM to your tank.
//
// Not a safety function: an overflow float switch wired in series with the pump is the real protection.
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
const int TRIG_PIN = 18, ECHO_PIN = 19, SOURCE_PIN = 23, PUMP_PIN = 25;
#elif CONFIG_IDF_TARGET_ESP32S3
const int TRIG_PIN = 6, ECHO_PIN = 7, SOURCE_PIN = 15, PUMP_PIN = 4;
#else  // ESP32-C3, C6, S2 ...
const int TRIG_PIN = 6, ECHO_PIN = 7, SOURCE_PIN = 10, PUMP_PIN = 5;
#endif

const float EMPTY_CM = 120, FULL_CM = 15;          // sensor to water, empty and full
const float START_BELOW = 25, STOP_ABOVE = 85;     // percent

fn::Device dev("tank-pump");
fn::Ultrasonic tank(TRIG_PIN, ECHO_PIN, "tank");
fn::DigitalSensor source(SOURCE_PIN, "source", "Source has water");
fn::Relay pump("pump", "Pump");

bool autoMode = true;
bool wanted = false;   // the sketch has asked the pump to run

void setup() {
  Serial.begin(115200);

  tank.tank(EMPTY_CM, FULL_CM).every(2000);        // adds the "level" reading (index 1)
  source.activeLow().labels("water", "DRY").debounce(500);
  pump.channel(PUMP_PIN, "Pump").maxOn(1200);

  dev.page("Control", "sliders")
      .toggle("auto", "Auto mode", [](bool on) { autoMode = on; if (!on) { wanted = false; pump.set(0, false); } }).icon("power")
      .value("state", "State").icon("activity");
  dev.ui().set("auto", true);

  dev.use(tank).use(source).use(pump);
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 1000) return;
  last = millis();

  const bool sourceOk = source.active();
  if (!sourceOk && pump.isOn(0)) { wanted = false; pump.set(0, false); }   // dry source: off, whatever the mode

  if (wanted && !pump.isOn(0)) {                         // the maximum run time ended it
    wanted = false;
    autoMode = false;
    dev.ui().set("auto", false);
    dev.ui().set("state", "STOPPED: ran 20 min without reaching the level");
    return;
  }

  const float level = tank.value(1);
  if (!autoMode || isnan(level)) return;                // no reading: leave the pump as it is
  if (level < START_BELOW && sourceOk && !pump.isOn(0)) { wanted = pump.set(0, true); dev.ui().set("state", "Filling"); }
  else if (level > STOP_ABOVE && pump.isOn(0)) { wanted = false; pump.set(0, false); dev.ui().set("state", "Full"); }
}
