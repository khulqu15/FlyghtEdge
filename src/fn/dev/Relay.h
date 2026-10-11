// FlyghtEdge — Relay: 1-16 channels on GPIO pins, each one a toggle on the dashboard.
//
//   fn::Relay relays("relay", "Pump room");
//   relays.channel(4, "Pump 1").maxOn(600);                      // active-high, off after 10 minutes
//   relays.channel(5, "Valve", fn::Relay::Type::ActiveLow);      // low-level trigger boards
//   relays.latching(6, 7, "Gate");                               // bistable relay: set pin, reset pin
//   relays.channel(8, "Forward").interlock(1);                   // channels of one group
//   relays.channel(9, "Reverse").interlock(1);                   // are never on together
//   void setup() { ...; dev.use(relays); dev.begin(); }
//
// Each channel is a Toggle widget "relay_1", "relay_2", ... so MQTT, Home Assistant, flows and Flyght
// Cloud see it like any other widget. An admin moves the pins from the card menu (three dots), see
// Pins.h; a saved assignment wins over the pins in the sketch. A channel without a pin does nothing.
//
// Safety: a channel starts in its boot state (off unless bootOn()), a pin is written before it
// becomes an output, maxOn() / pulse() switch a channel off by themselves, and a channel that is
// blocked by its interlock group refuses the command and says so in the log. This is a convenience,
// not a safety function: an emergency stop belongs in hardware.
#pragma once

#include "../ui/Ui.h"
#include "Pins.h"

namespace fn {

class Relay : public Module, public PinDevice {
public:
  enum class Type : uint8_t { ActiveHigh, ActiveLow, Latching };
  static constexpr int kMaxChannels = 16;

  // `id` and `title` must outlive the object (string literals). `id` is at most 15 characters.
  explicit Relay(const char* id = "relay", const char* title = "Relays") : id_(id), title_(title) {}

  // Add a channel (before begin()). pin -1 = not wired yet. Latching coils get a 50 ms pulse (high).
  Relay& channel(int pin, const char* name = nullptr, Type type = Type::ActiveHigh);
  Relay& latching(int setPin, int resetPin, const char* name = nullptr);
  // Settings of the channel added last.
  Relay& bootOn(bool on = true);
  Relay& maxOn(uint32_t seconds);        // switch off by itself after this long (0 = never)
  Relay& pulse(uint32_t milliseconds);   // every ON lasts this long, then OFF (0 = stays on)
  Relay& interlock(uint8_t group);       // 1-255; channels of one group are never on together
  Relay& minOff(uint32_t seconds);       // rest time: after switching off it cannot be switched on again for this long
  Relay& page(const char* name) { pageName_ = name; return *this; }

  const char* id() const override { return id_; }
  const char* title() const override { return title_; }
  const char* deps() const override { return "ui"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  int channels() const { return n_; }
  bool isOn(int ch) const { return ch >= 0 && ch < n_ && ch_[ch].on; }
  // false when the channel does not exist, has no pin, or its interlock group is busy
  bool set(int ch, bool on);

  // PinDevice
  const char* pinDeviceId() const override { return id_; }
  const char* pinDeviceTitle() const override { return title_; }
  bool pinsDriveLoad() const override { return true; }
  void pinSlots(std::vector<PinSlot>& out) const override;
  void pinApply(const std::vector<int>& pins) override;

private:
  struct Chan {
    String name;
    Type type = Type::ActiveHigh;
    int pin = -1, pin2 = -1;          // pin2 = reset coil of a latching relay
    int defPin = -1, defPin2 = -1;    // what the sketch asked for
    bool bootOn = false;
    uint32_t maxOnMs = 0, pulseMs = 0, minOffMs = 0, offAt = 0;
    uint8_t group = 0;
    bool on = false;
    uint32_t since = 0;
    int coilPin = -1;                 // the latching pin that is high now
    uint32_t coilAt = 0;
    String wid;
  };

  Chan* last() { return n_ ? &ch_[n_ - 1] : nullptr; }
  bool attached(const Chan& c) const { return c.pin >= 0 && (c.type != Type::Latching || c.pin2 >= 0); }
  void drive(Chan& c, bool on);
  void prepare(Chan& c);
  void release(int pin);
  void request(int ch, bool on);
  String slotKey(int ch, bool second) const;

  const char* id_;
  const char* title_;
  String pageName_ = "Relays";
  Ui* ui_ = nullptr;
  Chan ch_[kMaxChannels];
  int n_ = 0;
  bool started_ = false;
};

// A solenoid valve: one active-high channel that closes by itself after `maxOnSeconds` (the coil
// overheats if it is kept on) and rests `minOffSeconds` before it opens again.
class Solenoid : public Relay {
public:
  Solenoid(int pin, const char* id = "valve", const char* name = "Valve", uint32_t maxOnSeconds = 60, uint32_t minOffSeconds = 2,
           Type type = Type::ActiveHigh)
    : Relay(id, name) {
    channel(pin, name, type).maxOn(maxOnSeconds).minOff(minOffSeconds);
  }
};

}  // namespace fn
