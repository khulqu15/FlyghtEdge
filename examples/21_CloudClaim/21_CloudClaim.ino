// FlyghtEdge — 21 Flyght Cloud: claim the device and see it from anywhere.
//
// Flyght Cloud (https://flyght.obayan.id) lists this device next to your others on the web and lets you
// watch and control it away from home. It is off in a normal sketch; this one switches it on.
//
//   1. Put your key in secrets.h:   #define FN_LICENSE_KEY "fnk_live_..."   (get one on the site)
//   2. Upload, join the device's Wi-Fi once and give it your home Wi-Fi (the setup wizard does this).
//   3. The Serial Monitor prints a link. Open it while signed in to claim the device in your account.
//   4. Settings > Flyght Cloud on the local dashboard shows the status and the same link.
//
// The key you use for dev.license() is the key the device registers with. The server address is built
// in (and can be changed in Settings); everything else, the broker included, comes from the server.
// The local dashboard, MQTT and webhooks keep working whether or not the cloud is reachable.
//
// Status: compiled for ESP32-S3; not yet run on a board or against the server.
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

fn::Device dev("cloud-demo");

void setup() {
  Serial.begin(115200);

  dev.page("Demo", "home")
      .toggle("lamp", "Lamp", [](bool on) { Serial.printf("lamp %s\n", on ? "on" : "off"); }).icon("bulb")
      .gauge("temp", "Temperature", "°C", 0, 40).icon("thermo");

  dev.cloud().enable();            // off by default; also a switch in Settings > Flyght Cloud
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 5000) return;
  last = millis();

  dev.ui().set("temp", 24.0f + 3.0f * sinf(millis() / 30000.0f));   // replace with a real sensor

  // The claim link, printed until the device is online.
  static bool printed = false;
  if (!dev.cloud().connected() && !printed && dev.cloud().state() != fn::Cloud::State::Off) {
    Serial.println("Claim this device: " + dev.cloud().activateUrl());
    printed = true;
  }
}
