// FlyghtEdge — motors. Like the sensors, each class is its own module and its own object file.
//
//   DcMotor   brushed DC motor on an H-bridge (L298N, TB6612, L9110, DRV8871 ...): speed -100..100 %
//   Esc       brushless motor through an ESC (50 Hz servo pulse): throttle 0..100 %
//   Stepper   step/dir driver (A4988, DRV8825, TB6600 ...): move to a position, with acceleration
//   (Solenoid is in Relay.h: a relay channel with a maximum on-time and a rest time.)
//
// Safety first: every motor starts stopped, a reversal goes through zero, "Stop" is a card and also a
// call (stop()), and a motor that was told to run for a while stops by itself (maxRun). This is a
// convenience, not a safety function: an emergency stop belongs in hardware (a contactor, STO).
// The pins can be moved from the card menu (three dots > Settings) by an admin who types the device
// name; the motor is stopped before a pin moves.
#pragma once

#include "../ui/Ui.h"
#include "Pins.h"

namespace fn {

class DcMotor : public Module, public PinDevice {
public:
  // `pwmPin` = the enable / speed input, `in1` and `in2` = the direction inputs. With in2 = -1 only one
  // direction pin is used (forward = high). The pin numbers are defaults; a saved assignment wins.
  DcMotor(int pwmPin, int in1Pin, int in2Pin = -1, const char* id = "motor", const char* title = "DC motor");
  DcMotor& ramp(float percentPerSecond) { rate_ = percentPerSecond; return *this; }   // default 100: 1 s to full speed, 0 = none
  DcMotor& maxRun(uint32_t seconds) { maxRunMs_ = seconds * 1000u; return *this; }    // stops by itself, 0 = never
  DcMotor& pwmFrequency(uint32_t hz) { hz_ = hz; return *this; }                      // default 20 kHz
  DcMotor& page(const char* name) { pageName_ = name; return *this; }

  void speed(float percent);   // the speed to ramp to, -100..100
  void stop();                 // to zero at once, no ramp
  float speed() const { return cur_; }

  const char* id() const override { return id_; }
  const char* title() const override { return title_; }
  const char* deps() const override { return "ui"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;

  const char* pinDeviceId() const override { return id_; }
  const char* pinDeviceTitle() const override { return title_; }
  bool pinsDriveLoad() const override { return true; }
  void pinSlots(std::vector<PinSlot>& out) const override;
  void pinApply(const std::vector<int>& pins) override;

private:
  bool attach();
  void detach();
  void write();
  const char* id_;
  const char* title_;
  String pageName_ = "Motors";
  Ui* ui_ = nullptr;
  int pins_[3], defs_[3];            // pwm, in1, in2
  bool ok_ = false;
  float target_ = 0, cur_ = 0, rate_ = 100;
  uint32_t last_ = 0, runSince_ = 0, maxRunMs_ = 0, hz_ = 20000;
  String wSpeed_, wStop_, wNow_;
};

class Esc : public Module, public PinDevice {
public:
  Esc(int pin, const char* id = "esc", const char* title = "Brushless motor");
  Esc& pulse(uint16_t minUs, uint16_t maxUs) { minUs_ = minUs; maxUs_ = maxUs; return *this; }   // default 1000 - 2000 us
  Esc& ramp(float percentPerSecond) { rate_ = percentPerSecond; return *this; }                 // default 50, 0 = none
  Esc& armTime(uint32_t ms) { armMs_ = ms; return *this; }                                      // idle pulse before it accepts a throttle, default 3000
  Esc& maxRun(uint32_t seconds) { maxRunMs_ = seconds * 1000u; return *this; }
  Esc& page(const char* name) { pageName_ = name; return *this; }

  void throttle(float percent);   // 0..100, ignored while the ESC is arming
  void stop();                    // idle pulse at once
  bool armed() const { return armed_; }
  float throttle() const { return cur_; }

  const char* id() const override { return id_; }
  const char* title() const override { return title_; }
  const char* deps() const override { return "ui"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;

  const char* pinDeviceId() const override { return id_; }
  const char* pinDeviceTitle() const override { return title_; }
  bool pinsDriveLoad() const override { return true; }
  void pinSlots(std::vector<PinSlot>& out) const override;
  void pinApply(const std::vector<int>& pins) override;

private:
  bool attach();
  void write();
  const char* id_;
  const char* title_;
  String pageName_ = "Motors";
  Ui* ui_ = nullptr;
  int pin_, def_;
  bool ok_ = false, armed_ = false;
  uint16_t minUs_ = 1000, maxUs_ = 2000;
  float target_ = 0, cur_ = 0, rate_ = 50;
  uint32_t last_ = 0, armedAt_ = 0, armMs_ = 3000, runSince_ = 0, maxRunMs_ = 0;
  String wThr_, wStop_;
};

class Stepper : public Module, public PinDevice {
public:
  Stepper(int stepPin, int dirPin, int enablePin = -1, const char* id = "stepper", const char* title = "Stepper");
  Stepper& maxSpeed(float stepsPerSecond) { maxSps_ = stepsPerSecond; return *this; }       // default 800, up to about 5000
  Stepper& acceleration(float stepsPerSecond2) { accel_ = stepsPerSecond2; return *this; }  // default 1600
  Stepper& limits(long minPosition, long maxPosition) { min_ = minPosition; max_ = maxPosition; return *this; }
  Stepper& enableActiveHigh(bool on = true) { enHigh_ = on; return *this; }                 // default: the driver is enabled by a low level
  Stepper& releaseAfter(uint32_t ms) { idleMs_ = ms; return *this; }                        // disable the driver this long after a move, 0 = keep it on
  Stepper& reverse(bool on = true) { reverse_ = on; return *this; }
  Stepper& page(const char* name) { pageName_ = name; return *this; }

  // Positions are in steps from where the power came on (or from setPosition()).
  bool moveTo(long position);
  bool moveBy(long steps) { return moveTo(target_ + steps); }
  void stop();   // stops at once; the position stays right
  void setPosition(long p) { if (!run_) { pos_ = p; target_ = p; } }
  long position() const { return pos_; }
  bool moving() const { return run_; }

  const char* id() const override { return id_; }
  const char* title() const override { return title_; }
  const char* deps() const override { return "ui"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;

  const char* pinDeviceId() const override { return id_; }
  const char* pinDeviceTitle() const override { return title_; }
  bool pinsDriveLoad() const override { return true; }
  void pinSlots(std::vector<PinSlot>& out) const override;
  void pinApply(const std::vector<int>& pins) override;

private:
  static void onTimer(void* self);
  bool attach();
  void drive(bool enable);
  bool start(long target);
  const char* id_;
  const char* title_;
  String pageName_ = "Motors";
  Ui* ui_ = nullptr;
  int pins_[3], defs_[3];   // step, dir, enable
  bool ok_ = false, enHigh_ = false, reverse_ = false, wasRun_ = false, pending_ = false;
  float maxSps_ = 800, accel_ = 1600;
  long min_ = -10000, max_ = 10000, pendingTo_ = 0;
  volatile long pos_ = 0;
  long target_ = 0;
  volatile bool run_ = false;
  volatile int dir_ = 1;
  volatile uint32_t done_ = 0, total_ = 0;
  uint32_t idleMs_ = 2000, stoppedAt_ = 0, shownAt_ = 0;
  bool enabled_ = false;
  void* timer_ = nullptr;
  String wTarget_, wPos_, wStop_;
};

}  // namespace fn
