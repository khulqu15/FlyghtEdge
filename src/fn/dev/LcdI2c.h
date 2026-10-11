// FlyghtNinno — HD44780 character LCD on a PCF8574 I2C backpack (16x2, 20x4 ...).
//
//   fn::LcdI2c lcd(16, 2);           // address auto-detected (0x27 / 0x3F)
//   dev.use(lcd);
//   lcd.line(0, "Hello!");           // from loop(); mirrored live in the web UI
//
// Adds an "LCD" page showing the real screen; rows can be typed from the phone.
#pragma once

#include "I2c.h"

namespace fn {

class LcdI2c : public Module {
public:
  explicit LcdI2c(uint8_t cols = 16, uint8_t rows = 2, uint8_t addr = 0, TwoWire& wire = Wire)
    : cols_(cols), rows_(rows > 4 ? 4 : rows), addr_(addr), wire_(wire) {}
  LcdI2c& pins(int sda, int scl) { sda_ = sda; scl_ = scl; return *this; }

  const char* id() const override { return "lcd"; }
  const char* title() const override { return "LCD I2C"; }
  const char* deps() const override { return "?web"; }
  bool begin(App& app) override;
  Health health() const override;

  // Call from the loop task.
  void clear();
  void print(uint8_t row, uint8_t col, const String& text);
  void line(uint8_t row, const String& text);  // whole row, padded
  void backlight(bool on);
  bool backlight() const { return bl_; }
  String text(uint8_t row) const { return row < rows_ ? buf_[row] : String(); }
  uint8_t cols() const { return cols_; }
  uint8_t rows() const { return rows_; }

private:
  void send(uint8_t value, bool data);
  void nibble(uint8_t bits);
  void expander(uint8_t v);
  void publish();
  void routes(Web& web);

  uint8_t cols_, rows_, addr_;
  TwoWire& wire_;
  int sda_ = -1, scl_ = -1;
  bool found_ = false, bl_ = true;
  String buf_[4];
  Web* web_ = nullptr;
  uint32_t writes_ = 0;
};

}  // namespace fn
