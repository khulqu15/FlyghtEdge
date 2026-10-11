// FlyghtNinno — batteries-included preset.
//
//   fn::Device dev("lampu-pintar");
//   dev.page("Kontrol").toggle("lampu", "Lampu", [](bool on) { digitalWrite(2, on); });
//   dev.begin();                      // in setup()
//   dev.loop();                       // in loop()
//
// Includes: Wi-Fi + setup AP + captive portal + mDNS + web server + login +
// dashboard + MQTT + HTTP webhook + system page. Every part is still a
// separate module: use fn::App + app.use(...) to pick only what you need.
#pragma once

#include "auth/Auth.h"
#include "cloud/Cloud.h"
#include "core/App.h"
#include "hook/Webhook.h"
#include "mqtt/Mqtt.h"
#include "net/Ap.h"
#include "net/Captive.h"
#include "net/Mdns.h"
#include "net/Wifi.h"
#include "sys/System.h"
#include "ui/Ui.h"
#include "web/Web.h"

namespace fn {

class Device : public App {
public:
  explicit Device(const char* name, const char* version = "1.0.0") : App(name, version) {}

  Page& page(const char* name, const char* icon = "layout-grid") { return ui_.page(name, icon); }

  Web& web() { return web_; }
  Auth& auth() { return auth_; }
  Wifi& wifi() { return wifi_; }
  Ap& ap() { return ap_; }
  Captive& captive() { return captive_; }
  Mdns& mdns() { return mdns_; }
  Ui& ui() { return ui_; }
  Mqtt& mqtt() { return mqtt_; }
  Webhook& webhook() { return webhook_; }
  Cloud& cloud() { return cloud_; }
  System& system() { return system_; }

  // Extra modules of your own can still be added before begin().
  void begin() override {
    use(web_).use(auth_).use(system_).use(ap_).use(captive_).use(wifi_).use(mdns_).use(ui_).use(mqtt_).use(webhook_).use(cloud_);
    App::begin();
  }

private:
  Web web_;
  Auth auth_;
  System system_;
  Ap ap_;
  Captive captive_;
  Wifi wifi_;
  Mdns mdns_;
  Ui ui_;
  Mqtt mqtt_;
  Webhook webhook_;
  Cloud cloud_;
};

}  // namespace fn
