// FlyghtEdge — shared contracts: roles, events, scheduler, module base class.
#pragma once

#include <Arduino.h>
#include <functional>
#include <vector>

#include "Json.h"
#include "Kv.h"
#include "Log.h"

namespace fn {

#define FN_VERSION "2.1.1"
#define FN_API_VERSION 1

// Copyright / license notice compiled into every firmware (boot log, GET /sys).
extern const char kNotice[];

class App;

// ---------------------------------------------------------------------------
// Roles. Higher value = more rights. Routes and widgets declare a minimum.
// ---------------------------------------------------------------------------
namespace Role {
enum : uint8_t { None = 0, Viewer = 1, Operator = 2, Admin = 3 };
inline const char* name(uint8_t r) {
  switch (r) {
    case Viewer: return "viewer";
    case Operator: return "operator";
    case Admin: return "admin";
    default: return "none";
  }
}
}  // namespace Role

// ---------------------------------------------------------------------------
// Events. Emitted on the loop task only; modules running in other tasks use
// App::post() to hop onto the loop task first.
// ---------------------------------------------------------------------------
namespace ev {
enum : uint16_t {
  Any = 0,
  AppReady = 1,
  StaConnected,      // s = ssid
  StaGotIp,          // s = ip
  StaDisconnected,   // i = reason code, s = human text
  ApStarted,         // s = ip
  ApStopped,
  ApClientJoin,      // i = client count, s = client ip
  ApClientLeave,     // i = client count, s = client ip (if known)
  MdnsReady,         // s = hostname
  MqttConnected,
  MqttDisconnected,
  WidgetChanged,     // s = widget id
  ConfigChanged,     // s = module id
  Login,             // i = role, s = client ip
  TimeSynced,
  User = 1000        // first id free for applications
};
}  // namespace ev

struct Event {
  uint16_t id = 0;
  int32_t i = 0;
  String s;
};

class Bus {
public:
  using Fn = std::function<void(const Event&)>;
  void on(uint16_t id, Fn fn) { subs_.push_back({id, std::move(fn)}); }
  void emit(const Event& e) {
    for (size_t k = 0; k < subs_.size(); ++k)
      if (subs_[k].id == ev::Any || subs_[k].id == e.id) subs_[k].fn(e);
  }
  void emit(uint16_t id, int32_t i = 0, const String& s = String()) {
    Event e;
    e.id = id; e.i = i; e.s = s;
    emit(e);
  }

private:
  struct Sub { uint16_t id; Fn fn; };
  std::vector<Sub> subs_;
};

// ---------------------------------------------------------------------------
// Cooperative scheduler driven by App::loop(). Safe across millis() overflow.
// ---------------------------------------------------------------------------
class Scheduler {
public:
  using Id = uint32_t;
  Id every(uint32_t periodMs, std::function<void()> fn, bool runNow = false);
  Id after(uint32_t delayMs, std::function<void()> fn);
  void cancel(Id id);
  void run(uint32_t now);

private:
  struct Task { Id id; uint32_t next; uint32_t period; std::function<void()> fn; bool alive; };
  std::vector<Task> tasks_;
  Id nextId_ = 1;
};

// ---------------------------------------------------------------------------
// Module health, shown in the System page and GET /fn/api/v1/sys/health.
// ---------------------------------------------------------------------------
struct Health {
  enum State : uint8_t { Ok, Degraded, Failed, Disabled };
  State state = Ok;
  String msg;
  static Health ok(const String& m = String()) { return {Ok, m}; }
  static Health degraded(const String& m) { return {Degraded, m}; }
  static Health failed(const String& m) { return {Failed, m}; }
  static Health disabled(const String& m = "disabled") { return {Disabled, m}; }
  static const char* name(State s) {
    switch (s) { case Ok: return "ok"; case Degraded: return "degraded"; case Failed: return "failed"; default: return "disabled"; }
  }
};

// ---------------------------------------------------------------------------
// Module contract. Implement id() and begin(); everything else is optional.
//
//   class Buzzer : public fn::Module {
//    public:
//     const char* id() const override { return "buzzer"; }
//     const char* deps() const override { return "web,?mqtt"; }  // '?' = optional
//     bool begin(fn::App& app) override { ... return true; }
//     void loop(uint32_t now) override { ... }
//   };
// ---------------------------------------------------------------------------
class Module {
public:
  virtual ~Module() {}

  virtual const char* id() const = 0;
  virtual const char* title() const { return id(); }
  // Comma separated module ids this module needs; prefix with '?' if optional.
  virtual const char* deps() const { return ""; }

  // Called once, in dependency order. Return false to mark the module failed;
  // the rest of the app keeps running.
  virtual bool begin(App& app) = 0;
  virtual void loop(uint32_t nowMs) {}
  virtual void onEvent(const Event& e) {}
  virtual Health health() const { return Health::ok(); }
  // Extra diagnostics merged into the module entry of /sys/health.
  virtual void describe(JsonRef out) const {}

  bool running() const { return running_; }
  App& app() const { return *app_; }

private:
  friend class App;
  App* app_ = nullptr;
  bool running_ = false;
  bool failed_ = false;
};

// Wall clock helpers. The clock is valid after NTP or after a browser sync.
bool timeValid();
uint32_t unixTime();  // 0 when the clock is not valid
void setUnixTime(uint32_t sec);

}  // namespace fn
