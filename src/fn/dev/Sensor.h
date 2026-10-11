// FlyghtEdge — base of the sensor modules (Dht, Ds18b20, Ultrasonic, AnalogSensor, DigitalSensor,
// Mhz19, Pzem, Ina219). It gives them what every sensor card needs:
//   * one widget per reading ("<id>_<key>"), a gauge or a value, with an empty "—" while there is no
//     valid reading (a reading that cannot be trusted is never shown as a number);
//   * pins that an admin can move from the card menu (see Pins.h); a saved pin wins over the sketch;
//   * health: degraded while a reading is missing for longer than staleAfter().
//
//   fn::Dht dht(4, fn::Dht::Type::Dht22);
//   void setup() { dht.every(5000); dev.use(dht); dev.begin(); }
//
// loop() never blocks for long: a sensor that needs time (DS18B20 conversion, ultrasonic echo) is a
// small state machine, and the few that must hold a pin for a moment (DHT) do it for about 5 ms.
#pragma once

#include <functional>
#include <vector>

#include "../ui/Ui.h"
#include "Pins.h"

namespace fn {

class Sensor : public Module, public PinDevice {
public:
  const char* id() const override { return id_; }
  const char* title() const override { return title_; }
  const char* deps() const override { return "ui"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  Sensor& page(const char* name) { pageName_ = name; return *this; }
  Sensor& every(uint32_t ms) { periodMs_ = ms < minPeriodMs_ ? minPeriodMs_ : ms; return *this; }
  Sensor& staleAfter(uint32_t ms) { staleMs_ = ms; staleCustom_ = true; return *this; }   // 0 = never; default 5 readings

  // An alarm on the first reading: on at >= high, off at <= low (default: the same value). Adds an
  // "alarm" card (ALARM / OK); `cb` runs on the loop task when it changes, e.g. to switch a relay.
  Sensor& alarm(float high, float low = NAN, std::function<void(bool)> cb = nullptr);
  bool alarmOn() const { return alarmOn_; }

  // Last valid value of a reading (NaN when there is none). Index = order of declaration.
  float value(int reading) const { return reading >= 0 && reading < (int)rd_.size() ? rd_[reading].value : NAN; }

  // PinDevice
  const char* pinDeviceId() const override { return id_; }
  const char* pinDeviceTitle() const override { return title_; }
  bool pinsNeedRestart() const override { return restart_; }
  void pinSlots(std::vector<PinSlot>& out) const override;
  void pinApply(const std::vector<int>& pins) override;

protected:
  Sensor(const char* id, const char* title, uint32_t defPeriodMs, uint32_t minPeriodMs = 100)
    : id_(id), title_(title), periodMs_(defPeriodMs < minPeriodMs ? minPeriodMs : defPeriodMs), minPeriodMs_(minPeriodMs) {}

  // Declared by the derived class in its constructor.
  int addReading(const char* key, const char* label, const char* unit, uint8_t decimals, float lo, float hi, bool gauge = true,
                 const char* icon = "gauge");
  int addSlot(const char* key, const char* label, pure::PinRole role, int defPin, const char* proto = "gpio");

  void busPins() { restart_ = true; }   // UART / I2C: a new pin is used from the next start
  int pin(int slot) const { return slot >= 0 && slot < (int)sl_.size() ? sl_[slot].pin : -1; }
  void publish(int reading, float v);   // NaN = no valid reading
  void publishText(int reading, const char* text, float numeric);   // a reading that is a word (Wet / Dry)
  void scale(int reading, float lo, float hi, uint8_t decimals);
  bool due(uint32_t now) const { return now - lastAt_ >= periodMs_; }
  void mark(uint32_t now) { lastAt_ = now; }
  uint32_t periodMs() const { return periodMs_; }
  Ui* ui() const { return ui_; }

  // Hardware with the current pins. hw() is called in begin() and after a pin move, unhw() before it.
  virtual bool hw() = 0;
  virtual void unhw() {}
  virtual void tick(uint32_t now) = 0;

private:
  struct Reading {
    String key, wid, label, unit, icon;
    uint8_t dec = 1;
    float lo = 0, hi = 100, value = NAN;
    bool gauge = true;
    uint32_t okAt = 0;
    bool seen = false;
  };
  struct Slot {
    String key, label, proto;
    pure::PinRole role = pure::PinRole::In;
    int pin = -1, def = -1;
  };

  const char* id_;
  const char* title_;
  String pageName_ = "Sensors";
  uint32_t periodMs_, minPeriodMs_, staleMs_ = 0, lastAt_ = 0;
  bool staleCustom_ = false, restart_ = false, hwOk_ = false, started_ = false, alarmOn_ = false;
  uint32_t startAt_ = 0;
  int alarmReading_ = -1;
  float alarmHigh_ = NAN, alarmLow_ = NAN;
  std::function<void(bool)> alarmCb_;
  Ui* ui_ = nullptr;
  std::vector<Reading> rd_;
  std::vector<Slot> sl_;
};

}  // namespace fn
