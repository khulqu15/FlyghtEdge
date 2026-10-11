// FlyghtEdge — link to an external microcontroller over UART (Arduino Uno,
// STM32, RP2040, another ESP ...). Line based and human readable.
//
//   fn::McuLink mcu(/*rx*/ 18, /*tx*/ 19, 115200);
//   mcu.bind("temp", "temp")          // "temp=23.4" from the MCU -> widget "temp"
//      .forward("light")              // widget "light" changes -> "light=1\n" to the MCU
//      .quick("Blink", "LED=BLINK");  // one-tap button in the console
//   dev.use(mcu);
//
// The MCU sends lines like "temp=23.4,hum=60" or {"temp":23.4}; every key
// becomes a live value. Adds a "Serial" page: console, values and quick buttons.
#pragma once

#include <HardwareSerial.h>

#include <map>

#include "../web/Web.h"

namespace fn {

class Ui;

class McuLink : public Module {
public:
  McuLink(int rxPin, int txPin, uint32_t baud = 115200, HardwareSerial& port = Serial1)
    : rx_(rxPin), tx_(txPin), baud_(baud), port_(port) {}
  McuLink& bind(const char* key, const char* widgetId) { binds_.push_back({key, widgetId}); return *this; }
  McuLink& forward(const char* widgetId, const char* key = nullptr) { fwd_.push_back({widgetId, key ? key : widgetId}); return *this; }
  McuLink& quick(const char* label, const char* command) { quick_.push_back({label, command}); return *this; }
  McuLink& onLine(std::function<void(const String&)> cb) { onLine_ = std::move(cb); return *this; }
  McuLink& lineEnding(const char* eol) { eol_ = eol; return *this; }

  const char* id() const override { return "mcu"; }
  const char* title() const override { return "External MCU"; }
  const char* deps() const override { return "?web,?ui"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  void send(const String& line);
  String value(const char* key) const;
  bool online() const { return lastRx_ && millis() - lastRx_ < 5000; }

private:
  struct Pair { String a, b; };
  struct Line { uint32_t seq, ms; bool tx; String text; };
  void handle(const String& line);
  void push(bool tx, const String& text);
  void routes(Web& web);

  int rx_, tx_;
  uint32_t baud_;
  HardwareSerial& port_;
  String eol_ = "\n";
  std::vector<Pair> binds_, fwd_, quick_;
  std::function<void(const String&)> onLine_;
  std::map<String, String> values_;
  std::vector<Line> log_;
  String part_;
  uint32_t seq_ = 0, lastRx_ = 0, rxBytes_ = 0, txBytes_ = 0, lines_ = 0, lastPub_ = 0, pubSeq_ = 0;
  Ui* ui_ = nullptr;
  Web* web_ = nullptr;
  Kv kv_{"fn.mcu"};
};

}  // namespace fn
