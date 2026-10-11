// FlyghtNinno — device info, health of every module, logs, restart, factory
// reset, firmware update (OTA upload) and clock sync.
#pragma once

#include "../web/Web.h"

namespace fn {

class System : public Module {
public:
  const char* id() const override { return "sys"; }
  const char* title() const override { return "System"; }
  const char* deps() const override { return "web"; }

  System& ntp(const char* server1, const char* server2 = "time.google.com") { ntp1_ = server1; ntp2_ = server2; return *this; }
  System& timezone(const char* posixTz) { tz_ = posixTz; return *this; }  // e.g. "WIB-7"

  bool begin(App& app) override;
  void loop(uint32_t now) override;
  void onEvent(const Event& e) override;
  Health health() const override;

private:
  void routes(Web& web);
  void info(JsonRef out) const;
  void healthAll(JsonRef out) const;

  Web* web_ = nullptr;
  const char* ntp1_ = "pool.ntp.org";
  const char* ntp2_ = "time.google.com";
  const char* tz_ = "UTC0";
  uint32_t lastStatus_ = 0;
  bool otaBusy_ = false, ntpStarted_ = false, timeAnnounced_ = false;
};

}  // namespace fn
