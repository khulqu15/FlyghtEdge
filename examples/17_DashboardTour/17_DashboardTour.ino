// FlyghtEdge — 17 Dashboard tour: every widget type, pages, sizes, icons and who may change what.
//
// No wiring at all. Open the dashboard and look at the three pages:
//   Controls   toggle, button, slider, select, text         (what you can change)
//   Readouts   gauge, value, chart                           (what you can only watch)
//   Admin      a card only an admin may change               (.role)
// Everything you change here is a callback in the sketch: the same callbacks run when the change comes
// from MQTT, the HTTP API or a flow. Drag cards in Edit layout; the order is remembered on the device.
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

fn::Device dev("dashboard-tour");

void setup() {
  Serial.begin(115200);

  dev.page("Controls", "sliders")
      .toggle("lamp", "Lamp", [](bool on) { Serial.printf("lamp %s\n", on ? "on" : "off"); }).icon("bulb")
      .button("ping", "Ping", [] { Serial.println("ping!"); }).icon("bell")
      .slider("level", "Level", 0, 100, 5, [](float v) { Serial.printf("level %.0f\n", v); }).unit("%").size(2).icon("sliders")
      .select("mode", "Mode", {"Auto", "Manual", "Away"}, [](const String& m) { Serial.println("mode " + m); }).icon("list")
      .text("note", "Note", [](const String& s) { Serial.println("note: " + s); }).size(2).icon("type");

  dev.page("Readouts", "activity")
      .gauge("temp", "Temperature", "°C", 0, 50).icon("thermo")
      .value("uptime", "Uptime", "s").decimals(0).icon("activity")
      .chart("wave", "A sine wave", "", 90).every(1000).size(3).icon("chart");

  dev.page("Admin", "lock")
      .toggle("maintenance", "Maintenance mode", [](bool on) { Serial.println(on ? "maintenance" : "normal"); })
      .role(fn::Role::Admin).icon("hardhat");

  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 1000) return;
  last = millis();

  const float t = millis() / 1000.0f;
  dev.ui().set("temp", 25.0f + 8.0f * sinf(t / 20.0f));
  dev.ui().set("uptime", (unsigned long)(millis() / 1000));
  dev.ui().set("wave", 50.0f + 40.0f * sinf(t / 8.0f));
}
