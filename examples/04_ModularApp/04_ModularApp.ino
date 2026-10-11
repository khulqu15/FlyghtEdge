// FlyghtEdge — 04 Modular app (pick only what you need)
//
// fn::Device bundles everything. fn::App lets you choose modules one by one;
// anything you leave out is not linked, so the firmware gets smaller.
// Here: setup AP + captive portal + mDNS + login + dashboard. No station
// Wi-Fi, MQTT or webhook — e.g. a stand-alone field instrument.

#include <FlyghtEdge.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

fn::App app("field-meter", "1.0.0");
fn::Web web;
fn::Auth auth;
fn::System sys;
fn::Ap ap;
fn::Captive captive;
fn::Mdns mdns;
fn::Ui ui;

void setup() {
  Serial.begin(115200);

  ap.ssid("FieldMeter-{mac4}").password("meter1234");
  auth.defaultPassword("admin123").allowAnonymous(true, fn::Role::Viewer);

  ui.page("Meter", "gauge")
      .gauge("v", "Voltage", "V", 0, 30).decimals(2)
      .chart("vh", "History", "V", 120).size(3);

  app.use(web).use(auth).use(sys).use(ap).use(captive).use(mdns).use(ui);
  app.license(FN_LICENSE_KEY);
  app.begin();
}

void loop() {
  app.loop();
  static uint32_t last = 0;
  if (millis() - last >= 500) {
    last = millis();
    float v = analogReadMilliVolts(A0) / 1000.0f * 11.0f;  // 1:10 divider
    ui.set("v", v);
    ui.set("vh", v);
  }
}
