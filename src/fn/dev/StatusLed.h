// FlyghtEdge — Status LED: what the board tells you when there is no dashboard and no serial.
//
//   fn::Device dev("greenhouse");     // the preset already drives the onboard LED
//   fn::StatusLed status(2);          // or put the light on a pin of your own
//   void setup() { ...; dev.use(status); dev.begin(); }
//
// Read it by counting the blinks in every three seconds:
//
//   3 quick blinks     starting up
//   2 slow blinks      the setup access point is waiting for a phone
//   2 fast blinks      trying a saved Wi-Fi network
//   1 short blip       on Wi-Fi
//   2 short blips      on Wi-Fi and on Flyght Cloud
//   4 blinks           out of networks, waiting before it tries again
//   fast and steady    the device has stopped
//
// A board with an addressable pixel (`RGB_BUILTIN`) shows the same patterns in colour: white while
// starting, blue on the setup access point, amber while connecting, green on Wi-Fi, teal on the
// cloud, red when there is no network.
//
// The LED stands down when something else owns its pin — a light of your own, or a default I2C,
// SPI or serial pin on boards that put the onboard LED there — so a sketch that worked before
// keeps working. `dev.status().off()` switches it off altogether.
#pragma once

#include "../core/App.h"

namespace fn {

class StatusLed : public Module {
public:
  // pin -1 = the onboard LED of the board (RGB_BUILTIN, else LED_BUILTIN) when it has one.
  explicit StatusLed(int pin = -1) : pin_(pin) {}

  StatusLed& brightness(uint8_t percent) { bright_ = percent > 100 ? 100 : percent; return *this; }
  StatusLed& activeLow(bool on = true) { activeLow_ = on; return *this; }   // plain LED wired to 3V3
  StatusLed& addressable(bool on) { rgb_ = on; forced_ = true; return *this; }  // override the board default
  StatusLed& off(bool on = true) { off_ = on; return *this; }  // leave the LED alone: dev.status().off();

  const char* id() const override { return "status"; }
  const char* title() const override { return "Status LED"; }
  // It only blinks: a stopped device may still say that it has stopped.
  bool runsWhenHalted() const override { return true; }

  bool begin(App& app) override;
  void loop(uint32_t now) override;
  void onEvent(const Event& e) override;
  Health health() const override;
  void describe(JsonRef out) const override;

private:
  uint8_t phaseNow() const;             // pure::StatusPhase, kept as a number: Pure.h is internal
  void write(bool on);

  class Wifi* wifi_ = nullptr;
  class Ap* ap_ = nullptr;
  class Cloud* cloud_ = nullptr;
  int pin_;
  uint8_t bright_ = 20, phase_ = 0;
  bool rgb_ = false, forced_ = false, activeLow_ = false, off_ = false;
  bool lit_ = false, ready_ = false;
  const char* standDown_ = nullptr;     // set when something else owns the pin
  uint32_t phaseAt_ = 0;
};

}  // namespace fn
