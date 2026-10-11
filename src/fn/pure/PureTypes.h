// FlyghtNinno — small value types shared by public headers (Wi-Fi backoff,
// login throttle, GPS fix, colours, light effects). The logic behind them
// lives in Pure.cpp / Pure.h, which are internal.
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>

namespace fn {
namespace pure {

// Exponential backoff: base, 2*base, 4*base ... capped at max.
class Backoff {
public:
  explicit Backoff(uint32_t baseMs = 30000, uint32_t maxMs = 900000) : base_(baseMs), max_(maxMs) {}
  void configure(uint32_t baseMs, uint32_t maxMs);
  void reset() { failures_ = 0; }
  uint32_t fail();             // registers one failure, returns the delay before the next attempt
  uint32_t current() const;
  uint32_t failures() const { return failures_; }

private:
  uint32_t base_, max_;
  uint32_t failures_ = 0;
};

// Login throttle: after `freeTries` failures, lock for lockMs, doubling each
// further failure up to maxLockMs. Time is passed in so it is testable.
class RateLimiter {
public:
  RateLimiter(uint8_t freeTries = 5, uint32_t lockMs = 30000, uint32_t maxLockMs = 600000)
    : free_(freeTries), lock_(lockMs), maxLock_(maxLockMs) {}
  bool allowed(uint32_t nowMs) const;
  uint32_t retryAfterMs(uint32_t nowMs) const;
  void fail(uint32_t nowMs);
  void success() { fails_ = 0; lockedUntil_ = 0; }
  uint32_t failures() const { return fails_; }

private:
  uint8_t free_;
  uint32_t lock_, maxLock_;
  uint32_t fails_ = 0;
  uint32_t lockedUntil_ = 0;
};

// Latest GPS fix (from NMEA GGA + RMC).
struct GpsFix {
  bool valid = false;          // RMC status A
  uint8_t quality = 0;         // GGA: 0 none, 1 GPS, 2 DGPS, 4 RTK fixed, 5 RTK float
  uint8_t sats = 0;
  double lat = 0, lon = 0;     // degrees, south/west negative
  float altM = 0, hdop = 0;
  float speedKmh = 0, course = 0;
  uint8_t hh = 0, mm = 0, ss = 0, day = 0, month = 0;
  uint16_t year = 0;
  uint32_t sentences = 0, bad = 0;
};

// NMEA 0183 parser. Feed characters; complete, checksum-valid GGA/RMC
// sentences update the fix. Talker id is ignored ($GP, $GN, $GL, $GA, $BD).
class NmeaParser {
public:
  bool feed(char c);                       // true when a sentence was accepted
  bool feed(const std::string& line);
  const GpsFix& fix() const { return fix_; }
  static double coord(const std::string& v, const std::string& hemi);  // ddmm.mmmm -> degrees

private:
  bool finish();
  void time(const std::string& t);

  char buf_[100];
  size_t len_ = 0;
  bool active_ = false;
  GpsFix fix_;
};

struct Rgb {
  uint8_t r = 0, g = 0, b = 0;
  bool operator==(const Rgb& o) const { return r == o.r && g == o.g && b == o.b; }
  bool operator!=(const Rgb& o) const { return !(*this == o); }
};

// Light effects (see Light.h).
enum class Fx : uint8_t { Solid, Blink, Breathe, Heartbeat, Strobe, Rainbow, Cycle, Candle, Police, Sos, Sunrise, Alarm, Count };

}  // namespace pure
}  // namespace fn
