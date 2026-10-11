// FlyghtNinno — editions (Free / Pro / Enterprise) and feature switches.
//
// Each feature has a minimum edition and a default state. A feature is ON
// only when the edition allows it AND it is switched on (in code or in
// Settings > Plan & features). The edition comes from the license key only
// (App::license); evaluation mode allows every edition for a limited time.
//
//   dev.license(FN_LICENSE_KEY);              // a Pro key
//   dev.features().defaultOn("flows");       // on at first boot
//   if (app.features().on("rbac")) { ... }
#pragma once

#include <Arduino.h>
#include <vector>

#include "Json.h"
#include "Kv.h"

namespace fn {

enum class Edition : uint8_t { Free = 0, Pro = 1, Enterprise = 2 };
const char* editionName(Edition e);

class Features {
public:
  struct Def {
    String id, title, desc;
    Edition min;
    bool def;
  };

  Features();
  // Modules may register their own switches; the first definition wins.
  void define(const char* id, const char* title, Edition min, bool defaultOn, const char* desc = "");
  void defaultOn(const char* id, bool on = true);  // code default, used until changed in the UI

  Edition edition() const { return edition_; }

  bool allowed(const char* id) const;  // edition high enough
  bool on(const char* id) const;       // allowed and switched on
  // Persists the switch. Returns false (and changes nothing) when not allowed.
  bool set(const char* id, bool on);
  const Def* find(const char* id) const;
  void json(JsonRef out) const;

private:
  friend class App;  // sets the edition from the verified license key
  void edition(Edition e) { edition_ = e; }

  std::vector<Def> defs_;
  mutable std::vector<int8_t> cache_;  // -1 = not read from NVS yet
  Edition edition_ = Edition::Free;
  mutable Kv kv_{"fn.feat"};
};

}  // namespace fn
