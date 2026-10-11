// FlyghtNinno — lights: plain LEDs (digital / PWM) and RGB LEDs (onboard
// WS2812 "RGB_BUILTIN", or 3-pin PWM), with non-blocking millis() effects.
//
//   fn::Led status(2);                       // plain LED on GPIO2 (PWM dimming)
//   fn::RgbLed pixel(RGB_BUILTIN);           // onboard addressable LED
//   fn::RgbLed strip(5, 6, 7, "desk");       // R, G, B pins (common cathode)
//   dev.use(status).use(pixel);
//
//   pixel.color(0x2563eb);                   // from code, any time
//   pixel.effect(fn::Fx::Breathe, 3000);     // runs in the background
//
// Every light appears on the "Lights" page (color picker + effect gallery),
// over HTTP (/fn/api/v1/lights) and MQTT (<base>/light/<id>/set|state), and
// as blocks in Visual flows.
#pragma once

#include "../pure/PureTypes.h"
#include "../web/Web.h"

namespace fn {

class Mqtt;
using pure::Fx;

// Byte order of an addressable RGB LED. WS2812B (most DevKits) is GRB; some
// boards use RGB parts, then red and green look swapped: pick RGB.
enum class ColorOrder : uint8_t { GRB = 0, RGB, BRG, RBG, GBR, BGR };
const char* colorOrderName(ColorOrder o);
bool colorOrderFrom(const String& name, ColorOrder& out);

class Light : public Module {
public:
  struct State {
    bool on = true;
    uint32_t color = 0xffffff, color2 = 0x2563eb;
    uint8_t brightness = 100;  // percent
    Fx fx = Fx::Solid;
    uint16_t period = 1000;    // effect speed in ms
  };

  Light(const char* id, const char* title, bool rgb) : id_(id), title_(title), rgb_(rgb) {}
  const char* id() const override { return id_; }
  const char* title() const override { return title_; }
  const char* deps() const override { return "?web,?mqtt"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;

  // Control (call from the loop task).
  Light& on(bool v = true);
  Light& off() { return on(false); }
  Light& toggle() { return on(!st_.on); }
  Light& color(uint32_t rgbHex);
  Light& color(uint8_t r, uint8_t g, uint8_t b) { return color((uint32_t)r << 16 | (uint32_t)g << 8 | b); }
  Light& brightness(uint8_t percent);
  Light& effect(Fx fx, uint16_t periodMs = 0, uint32_t color2 = 0xFFFFFFFF);
  bool effect(const String& id, uint16_t periodMs = 0);  // "rainbow", "breathe", ...
  // Applies a JSON patch {on,color,color2,brightness,effect,period}; returns false on bad input.
  bool apply(const JsonRef& patch, String* err = nullptr);

  const State& state() const { return st_; }
  bool isRgb() const { return rgb_; }
  // Addressable LEDs only: channel order (code default; the Lights page can change it).
  Light& colorOrder(ColorOrder o) { order_ = o; return *this; }
  ColorOrder colorOrder() const { return order_; }
  virtual bool hasColorOrder() const { return false; }
  void stateJson(JsonRef out) const;
  static const std::vector<Light*>& all();

protected:
  // Drive the hardware with the final colour (brightness and gamma applied).
  virtual void output(uint8_t r, uint8_t g, uint8_t b) = 0;
  virtual bool hwBegin() = 0;
  ColorOrder order_ = ColorOrder::GRB;

private:
  void changed();
  static void sharedRoutes(App& app);
  static void flowBlocks();

  const char* id_;
  const char* title_;
  bool rgb_;
  State st_;
  pure::Rgb last_;
  bool dirty_ = true, saveSoon_ = false;
  String saved_;  // last state written to NVS
  uint32_t t0_ = 0, lastFrame_ = 0, lastSave_ = 0;
  Web* web_ = nullptr;
  Mqtt* mqtt_ = nullptr;
};

// Plain LED: digitalWrite, or PWM brightness when pwm=true.
class Led : public Light {
public:
  explicit Led(int pin, const char* id = "led", bool activeHigh = true, bool pwm = true)
    : Light(id, "LED", false), pin_(pin), high_(activeHigh), pwm_(pwm) {}
protected:
  bool hwBegin() override;
  void output(uint8_t r, uint8_t g, uint8_t b) override;
private:
  int pin_;
  bool high_, pwm_;
};

// RGB LED: one addressable pixel (WS2812 / onboard RGB_BUILTIN) or 3 PWM pins.
class RgbLed : public Light {
public:
  explicit RgbLed(int dataPin, const char* id = "rgb") : Light(id, "RGB LED", true), pin_(dataPin) {}
  RgbLed(int r, int g, int b, const char* id = "rgb", bool commonAnode = false)
    : Light(id, "RGB LED", true), pin_(-1), pr_(r), pg_(g), pb_(b), anode_(commonAnode) {}
  bool hasColorOrder() const override { return pin_ >= 0; }
protected:
  bool hwBegin() override;
  void output(uint8_t r, uint8_t g, uint8_t b) override;
private:
  int pin_, pr_ = -1, pg_ = -1, pb_ = -1;
  bool anode_ = false;
};

}  // namespace fn
