// FlyghtEdge — mDNS that keeps http://<hostname>.local reachable.
//
// Starts once an interface is up, restarts on every IP change, refreshes
// periodically for long-running devices, and advertises _flyght._tcp so apps
// can discover every FlyghtEdge device on the LAN.
#pragma once

#include "../web/Web.h"

namespace fn {

class Mdns : public Module {
public:
  const char* id() const override { return "mdns"; }
  const char* title() const override { return "mDNS"; }
  const char* deps() const override { return "?web"; }

  // Extra service, e.g. service("_modbus", "_tcp", 502).
  Mdns& service(const char* name, const char* proto, uint16_t port);
  Mdns& refreshEvery(uint32_t minutes) { refreshMs_ = minutes * 60000UL; return *this; }

  bool begin(App& app) override;
  void loop(uint32_t now) override;
  void onEvent(const Event& e) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  void restart(const char* reason);
  bool ready() const { return ready_; }

private:
  void routes(Web& web);
  struct Svc { String name, proto; uint16_t port; };
  std::vector<Svc> extra_;
  bool ready_ = false, anyIf_ = false;
  uint32_t refreshMs_ = 15UL * 60000, lastStart_ = 0, retryAt_ = 0, refreshes_ = 0, failures_ = 0;
  String reason_ = "boot", host_;
  uint16_t webPort_ = 80;
};

}  // namespace fn
