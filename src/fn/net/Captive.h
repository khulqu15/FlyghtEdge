// FlyghtNinno — captive portal: the dashboard opens by itself when a phone or
// laptop joins the access point.
//
// How it works:
//   1. DNS: every name resolves to the AP address while a client is on the AP.
//   2. DHCP option 114 advertises the portal URL (set by fn::Ap).
//   3. OS connectivity probes (Android /generate_204, Apple hotspot-detect,
//      Windows connecttest...) and any foreign Host get a 302 to the portal.
//   4. After the client logs in (or enters as guest) its probes are answered
//      with the "internet OK" reply, so the phone keeps the Wi-Fi connection
//      and closes the sign-in sheet. Mode Always keeps redirecting instead.
#pragma once

#include <DNSServer.h>

#include "Ap.h"

namespace fn {

enum class CaptiveMode : uint8_t { UntilLogin = 0, Always = 1 };

class Captive : public Module {
public:
  const char* id() const override { return "captive"; }
  const char* title() const override { return "Captive portal"; }
  const char* deps() const override { return "ap,web"; }

  Captive& mode(CaptiveMode m) { mode_ = m; return *this; }

  bool begin(App& app) override;
  void loop(uint32_t now) override;
  void onEvent(const Event& e) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  // Marks a client as signed in (probes then report "online").
  void accept(const String& ip);

private:
  bool handle(Request& q, Response& s);
  bool isOwnHost(const String& host) const;
  bool accepted(const String& ip);

  Ap* ap_ = nullptr;
  DNSServer dns_;
  bool dnsUp_ = false;
  CaptiveMode mode_ = CaptiveMode::UntilLogin;
  uint32_t lastCheck_ = 0, redirects_ = 0, dnsRestarts_ = 0;
  struct Seen { String ip; uint32_t at; };
  std::vector<Seen> accepted_;
  SemaphoreHandle_t mtx_ = nullptr;
};

}  // namespace fn
