// FlyghtEdge — 03 Custom module
//
// Shows how to package your own feature as a reusable fn::Module:
// it gets its own settings (NVS), REST endpoint, health entry on the System
// page, events from other modules, and runs inside the same loop.

#include <FlyghtEdge.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

// A reusable "heartbeat LED" module: blinks faster while Wi-Fi is down.
class Heartbeat : public fn::Module {
public:
  explicit Heartbeat(int pin) : pin_(pin) {}

  const char* id() const override { return "heartbeat"; }
  const char* title() const override { return "Heartbeat LED"; }
  const char* deps() const override { return "web"; }  // '?mqtt' would make it optional

  bool begin(fn::App& app) override {
    pinMode(pin_, OUTPUT);
    periodMs_ = kv_.u32("period", 1000);

    // GET/PUT /fn/api/v1/heartbeat
    auto* web = app.get<fn::Web>("web");
    web->get("/fn/api/v1/heartbeat", fn::Role::Viewer, [this](fn::Request&, fn::Response& s) {
      fn::Json j;
      j.set("period", (unsigned long)periodMs_).set("online", online_);
      s.json(j);
    });
    web->put("/fn/api/v1/heartbeat", fn::Role::Admin, [this](fn::Request& q, fn::Response& s) {
      fn::Json b = q.json();
      uint32_t p = (uint32_t)b.num("period", periodMs_);
      if (p < 100 || p > 10000) return s.error(422, "period must be 100-10000 ms");
      kv_.putU32("period", p);
      this->app().post([this, p] { periodMs_ = p; });  // apply on the loop task
      s.ok();
    });
    return true;
  }

  void loop(uint32_t now) override {
    uint32_t period = online_ ? periodMs_ : periodMs_ / 4;
    if (now - last_ >= period / 2) {
      last_ = now;
      on_ = !on_;
      digitalWrite(pin_, on_);
    }
  }

  void onEvent(const fn::Event& e) override {
    if (e.id == fn::ev::StaGotIp) online_ = true;
    if (e.id == fn::ev::StaDisconnected) online_ = false;
  }

  fn::Health health() const override {
    return online_ ? fn::Health::ok("slow blink: online") : fn::Health::degraded("fast blink: no Wi-Fi");
  }

private:
  int pin_;
  fn::Kv kv_{"fn.heartbeat"};
  uint32_t periodMs_ = 1000, last_ = 0;
  bool on_ = false, online_ = false;
};

fn::Device dev("custom-module");
Heartbeat heartbeat(2);

void setup() {
  Serial.begin(115200);
  dev.use(heartbeat);  // add your module next to the built-in ones
  dev.page("Status").value("uptime", "Uptime", "s").decimals(0);
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();
  dev.ui().set("uptime", (unsigned long)(millis() / 1000));
}
