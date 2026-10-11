// FlyghtEdge — 01 Smart Lamp
//
// 1. Upload, then join the Wi-Fi "smart-lamp-XXXX" from your phone.
// 2. The dashboard opens by itself. Create the admin password (first time).
// 3. Tap the switch: the LED follows. Pick your home Wi-Fi in the wizard.
//
// Later, open http://smart-lamp-xxxx.local from any device on that Wi-Fi.
// Tools > Partition Scheme: "No FS 4MB (2MB APP x2)" (see partitions.csv).

#include <FlyghtEdge.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

const int LED_PIN = 2;  // change to your board's LED pin

fn::Device dev("smart-lamp");

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  dev.page("Control", "home")
      .toggle("light", "Porch Light", [](bool on) { digitalWrite(LED_PIN, on); }).icon("bulb")
      .gauge("temp", "Room Temperature", "°C", 0, 50).icon("thermo");

  dev.license(FN_LICENSE_KEY);

  dev.begin();
}

void loop() {
  dev.loop();

  // Demo value; replace with a real sensor reading.
  static uint32_t last = 0;
  if (millis() - last > 1000) {
    last = millis();
    dev.ui().set("temp", 26.0 + 2.0 * sin(millis() / 20000.0));
  }
}
