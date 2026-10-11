// FlyghtEdge — Bosch BNO055 absolute orientation sensor (I2C), no extra library.
//
//   fn::Bno055 imu;                 // 0x28 on the default Wire pins
//   imu.pins(8, 9).rate(50);        // optional
//   dev.use(imu);
//   ...  imu.yaw(), imu.pitch(), imu.roll()   // degrees
//
// Adds a "Motion" page: artificial horizon, compass, 3D box, calibration
// status, "set as level" and "save calibration".
#pragma once

#include "I2c.h"

namespace fn {

class Bno055 : public Module {
public:
  explicit Bno055(uint8_t addr = 0x28, TwoWire& wire = Wire) : addr_(addr), wire_(wire) {}
  Bno055& pins(int sda, int scl) { sda_ = sda; scl_ = scl; return *this; }
  Bno055& rate(uint16_t hz) { periodMs_ = hz ? 1000 / (hz > 100 ? 100 : hz) : 20; return *this; }
  Bno055& externalCrystal(bool on) { crystal_ = on; return *this; }

  const char* id() const override { return "imu"; }
  const char* title() const override { return "BNO055 IMU"; }
  const char* deps() const override { return "?web"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  bool ready() const { return state_ == State::Run; }
  // Orientation after the "level" offset, degrees. Yaw 0..360, pitch/roll -180..180.
  float yaw() const { return yaw_; }
  float pitch() const { return pitch_; }
  float roll() const { return roll_; }
  float temperature() const { return temp_; }
  // Calibration 0..3 each: system, gyro, accel, mag.
  uint8_t calibration(uint8_t which) const { return which < 4 ? calib_[which] : 0; }
  void level();        // current attitude becomes 0/0/0
  void clearLevel();

private:
  enum class State : uint8_t { Detect, Run, Missing };
  bool init();
  void read();
  bool saveCalibration();
  void routes(Web& web);
  void snapshot(JsonRef out) const;

  uint8_t addr_;
  TwoWire& wire_;
  int sda_ = -1, scl_ = -1;
  bool crystal_ = true;
  State state_ = State::Detect;
  uint32_t periodMs_ = 20, lastRead_ = 0, lastSlow_ = 0, lastPub_ = 0, nextTry_ = 0, reads_ = 0, errors_ = 0;
  float yaw_ = 0, pitch_ = 0, roll_ = 0, temp_ = 0;
  float y0_ = 0, p0_ = 0, r0_ = 0;
  uint8_t calib_[4] = {0, 0, 0, 0};
  bool hasSavedCal_ = false;
  Web* web_ = nullptr;
  Kv kv_{"fn.imu"};
};

}  // namespace fn
