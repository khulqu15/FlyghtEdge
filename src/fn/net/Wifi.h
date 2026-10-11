// FlyghtEdge — station Wi-Fi that behaves well for months.
//
// * Up to 8 saved networks (home, office, hotspot...), tried by priority;
//   if one fails the next is used automatically. No hardcoded password needed:
//   networks are added from the phone (setup wizard / Network page).
// * Direct connect, no background scans, exponential backoff 15s..15min.
// * While a phone is on the setup AP, automatic attempts wait (a station
//   connect can move the radio channel and drop the phone) - except during
//   the 90 s window that follows a user's "Connect" press.
#pragma once

#include <WiFi.h>

#include "../pure/PureTypes.h"
#include "../web/Web.h"

namespace fn {

class Ap;

class Wifi : public Module {
public:
  const char* id() const override { return "wifi"; }
  const char* title() const override { return "Wi-Fi"; }
  const char* deps() const override { return "?web,?ap"; }

  // Code default network (first boot only, optional).
  Wifi& add(const char* ssid, const char* pass, uint8_t priority = 50);
  Wifi& connectTimeout(uint16_t sec) { timeoutMs_ = sec * 1000UL; return *this; }
  Wifi& backoff(uint16_t baseSec, uint16_t maxSec) { backoff_.configure(baseSec * 1000UL, maxSec * 1000UL); return *this; }

  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  bool connected() const { return state_ == State::Connected; }
  String ssid() const { return connected() ? currentSsid_ : String(); }
  // Adds or updates a network with top priority and connects now.
  void connectNow(const String& ssid, const String& pass);
  void reconnect();

private:
  enum class State : uint8_t { Idle, Connecting, Connected, Waiting };
  struct Net { String ssid, pass; uint8_t prio; };

  void load();
  void save();
  void sortNets();
  void attempt(uint32_t now);
  void routes(Web& web);
  void onStaEvent(arduino_event_id_t id, const arduino_event_info_t& info);
  bool deferForAp(uint32_t now) const;
  static const char* reasonText(uint8_t reason);
  const char* stateName() const;

  Kv kv_{"fn.wifi"};
  std::vector<Net> nets_, defaults_;
  State state_ = State::Idle;
  size_t idx_ = 0;
  uint32_t attemptAt_ = 0, nextAt_ = 0, userWindowUntil_ = 0;
  uint32_t timeoutMs_ = 15000;
  pure::Backoff backoff_{15000, 900000};
  String currentSsid_, lastError_;
  uint32_t connects_ = 0, failures_ = 0;
  Ap* ap_ = nullptr;

  // async scan
  bool scanning_ = false;
  uint32_t scanAt_ = 0;
  String scanJson_ = "[]";
};

}  // namespace fn
