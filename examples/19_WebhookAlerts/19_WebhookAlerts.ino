// FlyghtEdge — 19 Webhook alerts: tell another system when something happens.
//
// A door switch (or the BOOT button, to try it) sends an HTTPS POST with a signed JSON body to your
// server, n8n, Node-RED or any webhook receiver. Card changes are sent too, so a receiver also sees
// "the lamp was switched on from the dashboard".
//
//   POST <URL>   X-Flyght-Signature: <HMAC-SHA256 of the body, hex, with SECRET>
//   {"event":"door","device":"...","count":3,"uptime":120, ...}
//
// Verify the signature on the receiving side before you trust a request. Use https:// in production;
// the target and the secret can also be set under Connect > Webhook, and a saved value wins.
//
// Wiring:   a switch between SWITCH_PIN and GND (the internal pull-up is on). Default pin: the BOOT button.
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
const int SWITCH_PIN = 0;
#elif CONFIG_IDF_TARGET_ESP32S3
const int SWITCH_PIN = 0;
#else  // ESP32-C3, C6, S2 ...
const int SWITCH_PIN = 9;
#endif

const char* URL = "https://example.com/flyght-hook";
const char* SECRET = "change-me";

fn::Device dev("door-alert");
fn::DigitalSensor door(SWITCH_PIN, "door", "Door");

uint32_t count = 0;

void setup() {
  Serial.begin(115200);

  dev.webhook().url(URL).secret(SECRET).interval(0);   // interval 0: only on changes and events

  door.activeLow().labels("OPEN", "closed").debounce(80).onChange([](bool open) {
    if (!open) return;
    ++count;
    fn::Json extra;
    extra.set("count", (unsigned long)count);
    extra.set("uptime", (unsigned long)(millis() / 1000));
    dev.webhook().send("door", extra);
  });

  dev.page("Door", "home")
      .value("count", "Times opened").decimals(0).icon("activity");

  dev.use(door);
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();
  dev.ui().set("count", (unsigned long)count);
}
