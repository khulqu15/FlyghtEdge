// FlyghtEdge — 11 GPS geofence: a siren when the device leaves a circle.
//
// Set HOME_LAT / HOME_LON and RADIUS_M below. While "Armed" is on, the relay (a siren, a buzzer, a
// warning light) switches on when the GPS position is farther than the radius from home, and off
// again once it is back inside 90 % of the radius (so it does not chatter on the edge).
//
// Wiring:   GPS TX -> GPS_RX    GPS RX -> GPS_TX    relay board IN -> RELAY pin (active-high)
// Dashboard: "Geofence" page (distance, armed, siren), Devices > Map (where it is).
// The siren starts when the device leaves the circle and switches itself off after 60 s, so it
// cannot run forever; it starts again only after the device has been back inside.
//
// This is a convenience, not a safety function: a GPS has no fix indoors and can be wrong by metres.
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

// ---- pins (the wiring diagram in docs/guides is generated from this block) ----
#if CONFIG_IDF_TARGET_ESP32
const int GPS_RX = 16, GPS_TX = 17, RELAY = 25;
#elif CONFIG_IDF_TARGET_ESP32S3
const int GPS_RX = 18, GPS_TX = 17, RELAY = 4;
#else  // ESP32-C3, C6, S2 ...
const int GPS_RX = 10, GPS_TX = 0, RELAY = 5;
#endif

// The centre of the circle and its radius (here: Monas, Jakarta). Change these.
const double HOME_LAT = -6.175392, HOME_LON = 106.827153;
const float RADIUS_M = 150;

fn::Device dev("geofence");
fn::Gps gps(GPS_RX, GPS_TX, 9600, Serial1);
fn::Relay siren("siren", "Siren");

bool armed = true;
bool outside = false;

// Great-circle distance in metres (haversine); good to a few metres at these distances.
double metersBetween(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0, d2r = M_PI / 180.0;
  const double dLat = (lat2 - lat1) * d2r, dLon = (lon2 - lon1) * d2r;
  const double a = sin(dLat / 2) * sin(dLat / 2) + cos(lat1 * d2r) * cos(lat2 * d2r) * sin(dLon / 2) * sin(dLon / 2);
  return 2 * R * asin(sqrt(a));
}

void setup() {
  Serial.begin(115200);

  siren.channel(RELAY, "Siren").maxOn(60);
  gps.track(300, 5).setClock(true);

  dev.page("Geofence", "map")
      .toggle("armed", "Armed", [](bool on) { armed = on; if (!on) siren.set(0, false); }).icon("bell")
      .value("dist", "Distance from home", "m").decimals(0).icon("map")
      .value("state", "State").icon("activity");
  dev.ui().set("armed", true);

  dev.use(gps).use(siren);
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 1000) return;
  last = millis();

  if (!gps.hasFix()) {
    dev.ui().set("state", "Waiting for a GPS fix");
    return;
  }
  const double d = metersBetween(HOME_LAT, HOME_LON, gps.lat(), gps.lon());
  dev.ui().set("dist", d);

  const bool before = outside;
  if (!outside && d > RADIUS_M) outside = true;
  else if (outside && d < RADIUS_M * 0.9) outside = false;

  dev.ui().set("state", outside ? "OUTSIDE" : "Inside");
  if (armed && outside != before) siren.set(0, outside);   // on the change only: maxOn can end the siren
}
