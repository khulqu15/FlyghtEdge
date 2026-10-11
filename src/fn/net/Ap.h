// FlyghtNinno — SoftAP that stays visible.
//
// Modes: Always (default), Fallback (only while station Wi-Fi is down), Off.
// A watchdog re-applies SSID/IP if the interface ever drops, and DHCP option
// 114 (RFC 8910) advertises the portal URL so phones open it on connect.
#pragma once

#include <WiFi.h>

#include "../web/Web.h"

namespace fn {

enum class ApMode : uint8_t { Always = 0, Fallback = 1, Off = 2 };

class Ap : public Module {
public:
  const char* id() const override { return "ap"; }
  const char* title() const override { return "Access point"; }
  const char* deps() const override { return "?web"; }

  // Code defaults (first boot); editable later in the Network page.
  Ap& ssid(const char* tpl) { defSsid_ = tpl; return *this; }       // supports {name} {mac4} ...
  Ap& password(const char* pw) { defPass_ = pw; return *this; }     // "" = open network
  Ap& mode(ApMode m) { defMode_ = m; return *this; }
  Ap& ip(IPAddress ip) { ip_ = ip; return *this; }
  Ap& channel(uint8_t ch) { defChannel_ = ch; return *this; }
  Ap& maxClients(uint8_t n) { maxClients_ = n; return *this; }

  bool begin(App& app) override;
  void loop(uint32_t now) override;
  void onEvent(const Event& e) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  bool active() const { return active_; }
  uint8_t clients() const { return clients_; }
  IPAddress address() const { return ip_; }
  String ssidNow() const { return ssid_; }
  bool isApClient(const String& ip) const;  // true if ip is in the AP subnet
  // Last known IP of a station MAC (from DHCP), used by the captive portal.
  String ipForMac(const uint8_t mac[6]) const;

private:
  void load();
  bool shouldRun() const;
  bool start(const char* reason);
  void stop(const char* reason);
  void routes(Web& web);

  Kv kv_{"fn.ap"};
  String defSsid_ = "{name}-{mac4}", defPass_ = "";
  ApMode defMode_ = ApMode::Always;
  uint8_t defChannel_ = 1;

  String ssid_, pass_;
  ApMode mode_ = ApMode::Always;
  uint8_t channel_ = 1, maxClients_ = 4;
  IPAddress ip_ = IPAddress(192, 168, 4, 1);
  bool active_ = false, staUp_ = false;
  uint8_t clients_ = 0;
  uint32_t staDownSince_ = 0, lastCheck_ = 0, starts_ = 0, recoveries_ = 0;
  char portalUri_[40] = {0};
  struct Lease { uint8_t mac[6]; String ip; };
  std::vector<Lease> leases_;
};

}  // namespace fn
