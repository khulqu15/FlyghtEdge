// FlyghtEdge — application container.
//
//   fn::App app("sensor-node", "1.0.0");
//   fn::Wifi wifi;  fn::Web web;  fn::Ui ui;
//   app.use(wifi).use(web).use(ui);
//   app.begin();               // resolves deps, starts modules in order
//   ...  app.loop();           // in loop()
//
// fn::Device (Device.h) is the batteries-included preset built on top of this.
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "Core.h"
#include "Features.h"

namespace fn {

class App {
public:
  // Inline on purpose: compiled in the sketch, so `Serial` is the port the
  // board settings chose (UART or native USB), also with a precompiled library.
  explicit App(const char* name, const char* version = "1.0.0") { log::setOutput(&Serial); init(name, version); }
  virtual ~App() {}

  App& use(Module& m);
  virtual void begin();
  void loop();

  // --- lookup ---
  Module* find(const char* id) const;
  template <typename T>
  T* get(const char* id) const { Module* m = find(id); return (m && m->running()) ? static_cast<T*>(m) : nullptr; }
  const std::vector<Module*>& modules() const { return modules_; }

  // --- license ---
  // API key from the FlyghtEdge license (call before begin()). It sets the
  // edition (Free / Pro / Enterprise) and is checked offline, with ECDSA P-256.
  // Without a valid key the app runs in evaluation mode: every feature on for
  // 30 minutes per boot, then it stops until the next reset.
  //   dev.license(FN_LICENSE_KEY);   // #define FN_LICENSE_KEY "fnk_live_..." in secrets.h
  App& license(const char* key) { licenseKey_ = key ? key : ""; licenseKey_.trim(); return *this; }
  // A key typed in the dashboard is kept in NVS and used when the sketch gives none; it applies at the next start.
  bool sketchKey() const { return licenseKey_.length() > 0; }
  bool keepKey(const String& key);
  String keptKey() const;
  bool dropKey();
  // Removed in 2.0: the edition comes from the license key, see license().
  App& edition(Edition e) = delete;

  // --- infrastructure ---
  Bus& bus() { return bus_; }
  // Free / Pro / Enterprise switches (see Features.h).
  Features& features() { return features_; }
  const Features& features() const { return features_; }
  Scheduler& sched() { return sched_; }
  // Runs fn on the loop task. Safe to call from any task (HTTP, MQTT, events).
  bool post(std::function<void()> fn);
  // Global lock for state shared with the HTTP server task.
  static void lock();
  static void unlock();
  struct Guard { Guard() { App::lock(); } ~Guard() { App::unlock(); } };

  // --- identity ---
  const String& name() const { return name_; }          // product/model slug, e.g. "lampu-pintar"
  const String& version() const { return version_; }    // application firmware version
  const String& deviceId() const { return deviceId_; }  // 12 hex chars from the factory MAC
  String displayName() const;                           // user editable, defaults to name
  void setDisplayName(const String& n);
  String hostname() const;                              // sanitized, user editable
  void setHostname(const String& h);
  // Expands {name} {id} {mac4} {mac6} {version} {chip}.
  String expand(const String& tpl) const;

  // --- lifecycle helpers ---
  void restart(const char* reason, uint32_t delayMs = 800);
  void factoryReset();
  uint32_t uptimeSec() const { return millis() / 1000; }
  // Delay at the end of each loop() so lower priority tasks can run.
  // 1 ms by default; set 0 for tight control loops on dual-core chips.
  void setYield(uint8_t ms) { yieldMs_ = ms; }

protected:
  String name_, version_, deviceId_;

private:
  void init(const char* name, const char* version);
  bool sortModules();

  std::vector<Module*> modules_;
  Bus bus_;
  Features features_;
  Scheduler sched_;
  QueueHandle_t postQ_ = nullptr;
  String licenseKey_;
  // Written by the HTTP task too (a saved key restarts the device while it is stopped, when post() jobs are dropped).
  volatile uint32_t restartAt_ = 0;
  uint8_t yieldMs_ = 1;
  bool begun_ = false;
  bool stopLogged_ = false;
};

}  // namespace fn
