// FlyghtEdge — 10 GPS tracker: position, speed and a track on a map.
//
// A NMEA GPS module (u-blox NEO-6M/7M/8M, BN-220, ...) on a UART gives the dashboard a live map with
// the track you have driven, plus speed, altitude and satellites. The GPS also sets the clock, so
// timestamps are right without Wi-Fi.
//
// Wiring (3.3 V logic):   GPS TX -> GPS_RX pin     GPS RX -> GPS_TX pin     VCC 3.3-5 V, GND
// Open the dashboard:     Devices > Map (the track), and the "Position" page (the numbers).
// Outdoors the first fix takes 30 s to a few minutes; indoors there is usually none.
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
const int GPS_RX = 16, GPS_TX = 17;
#elif CONFIG_IDF_TARGET_ESP32S3
const int GPS_RX = 18, GPS_TX = 17;
#else  // ESP32-C3, C6, S2 ...
const int GPS_RX = 10, GPS_TX = 0;
#endif

fn::Device dev("gps-tracker");
fn::Gps gps(GPS_RX, GPS_TX, 9600, Serial1);

void setup() {
  Serial.begin(115200);

  gps.track(600, 5)        // keep up to 600 points, add one every 5 m
      .setClock(true);     // set the device clock from the GPS time

  dev.page("Position", "map")
      .value("speed", "Speed", "km/h").decimals(0).icon("gauge")
      .value("alt", "Altitude", "m").decimals(0).icon("activity")
      .value("sats", "Satellites").decimals(0).icon("radio")
      .gauge("hdop", "HDOP (lower is better)", "", 0, 10).icon("gauge");

  dev.use(gps);
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 1000) return;
  last = millis();

  const auto& f = gps.fix();
  dev.ui().set("sats", (int)f.sats);
  if (gps.hasFix()) {
    dev.ui().set("speed", f.speedKmh);
    dev.ui().set("alt", f.altM);
    dev.ui().set("hdop", f.hdop);
  }
}
