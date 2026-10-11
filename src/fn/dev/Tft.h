// FlyghtNinno — SPI TFT screen with a live web mirror and drawing tools.
//
// Built-in drivers (no extra library): ST7789 (240x240, 240x320, 135x240,
// 170x320) and ILI9341 (240x320). Any other display works through GfxTarget,
// which wraps an Adafruit_GFX / TFT_eSPI / LovyanGFX object.
//
//   fn::TftSpi panel(fn::TftSpi::ST7789, 240, 240, /*cs*/ 10, /*dc*/ 9, /*rst*/ 8);
//   fn::Tft tft(panel);
//   tft.backlight(7);
//   dev.use(tft);
//   tft.clear(fn::Tft::rgb(15, 23, 42));
//   tft.text(10, 10, "Hello!", 0xFFFF, 2);
//
// Adds a "Screen" page: the same picture in the browser, drawing tools
// (text, rectangle, circle, line), colors, rotation and brightness.
#pragma once

#include <SPI.h>

#include "../web/Web.h"

namespace fn {

// Minimal drawing backend. Implement fillRect (and width/height/rotation).
class TftTarget {
public:
  virtual ~TftTarget() {}
  virtual bool begin() = 0;
  virtual int16_t width() const = 0;
  virtual int16_t height() const = 0;
  virtual void setRotation(uint8_t r) = 0;
  virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) = 0;
  virtual void drawPixel(int16_t x, int16_t y, uint16_t color) { fillRect(x, y, 1, 1, color); }
  virtual void invert(bool on) {}
  virtual const char* name() const { return "custom"; }
};

// Built-in SPI driver for ST7789 and ILI9341 panels.
class TftSpi : public TftTarget {
public:
  enum Chip : uint8_t { ST7789, ILI9341 };
  TftSpi(Chip chip, uint16_t w, uint16_t h, int8_t cs, int8_t dc, int8_t rst = -1, SPIClass& spi = SPI, uint32_t hz = 40000000)
    : chip_(chip), w0_(w), h0_(h), cs_(cs), dc_(dc), rst_(rst), spi_(spi), hz_(hz) {}
  TftSpi& pins(int8_t sck, int8_t mosi, int8_t miso = -1) { sck_ = sck; mosi_ = mosi; miso_ = miso; return *this; }

  bool begin() override;
  int16_t width() const override { return w_; }
  int16_t height() const override { return h_; }
  void setRotation(uint8_t r) override;
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;
  void invert(bool on) override;
  const char* name() const override { return chip_ == ST7789 ? "ST7789" : "ILI9341"; }

private:
  void start();
  void end();
  void command(uint8_t c);
  void data(uint8_t d);
  void window(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

  Chip chip_;
  uint16_t w0_, h0_;
  int16_t w_ = 0, h_ = 0, xoff_ = 0, yoff_ = 0;
  int8_t cs_, dc_, rst_, sck_ = -1, mosi_ = -1, miso_ = -1;
  SPIClass& spi_;
  uint32_t hz_;
};

// Adapter for display libraries you already use. G needs width(), height(),
// setRotation(uint8_t), fillRect(x, y, w, h, color) and drawPixel(x, y, color).
template <class G>
class GfxTarget : public TftTarget {
public:
  explicit GfxTarget(G& g, std::function<bool()> init = nullptr) : g_(g), init_(std::move(init)) {}
  bool begin() override { return init_ ? init_() : true; }
  int16_t width() const override { return g_.width(); }
  int16_t height() const override { return g_.height(); }
  void setRotation(uint8_t r) override { g_.setRotation(r); }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) override { g_.fillRect(x, y, w, h, c); }
  void drawPixel(int16_t x, int16_t y, uint16_t c) override { g_.drawPixel(x, y, c); }
  const char* name() const override { return "GFX"; }

private:
  G& g_;
  std::function<bool()> init_;
};

class Tft : public Module {
public:
  explicit Tft(TftTarget& target) : t_(target) {}
  // Backlight on a PWM pin (brightness slider in the UI).
  Tft& backlight(int pin, bool activeHigh = true) { blPin_ = pin; blHigh_ = activeHigh; return *this; }

  const char* id() const override { return "tft"; }
  const char* title() const override { return "TFT screen"; }
  const char* deps() const override { return "?web"; }
  bool begin(App& app) override;
  Health health() const override;

  // Drawing (call from the loop task). Colors are RGB565.
  static uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) { return (uint16_t)((r & 0xF8) << 8 | (g & 0xFC) << 3 | b >> 3); }
  void clear(uint16_t color = 0x0000);
  void text(int16_t x, int16_t y, const String& s, uint16_t color = 0xFFFF, uint8_t size = 1, int32_t bg = -1);
  void rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color, bool fill = false);
  void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
  void circle(int16_t x, int16_t y, int16_t r, uint16_t color, bool fill = false);
  void rotation(uint8_t r);
  void brightness(uint8_t percent);
  void splash();  // device name, address and a friendly face
  int16_t width() const { return t_.width(); }
  int16_t height() const { return t_.height(); }

private:
  struct Op {
    char k;  // 'c' clear, 't' text, 'r' rect, 'l' line, 'o' circle
    int16_t a, b, c, d;
    uint16_t color;
    int32_t bg;
    uint8_t size;
    bool fill;
    String s;
  };
  void record(const Op& op);
  void draw(const Op& op);
  void opJson(const Op& op, JsonRef o) const;
  void hline(int16_t x, int16_t y, int16_t w, uint16_t c) { t_.fillRect(x, y, w, 1, c); }
  void routes(Web& web);
  static uint16_t parseColor(const String& s, uint16_t def);

  TftTarget& t_;
  bool ok_ = false, inverted_ = false;
  int blPin_ = -1;
  bool blHigh_ = true;
  uint8_t rot_ = 0, bright_ = 100;
  std::vector<Op> ops_;
  Web* web_ = nullptr;
  Kv kv_{"fn.tft"};
  uint32_t draws_ = 0;
};

}  // namespace fn
