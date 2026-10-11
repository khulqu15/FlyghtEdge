// FlyghtNinno — building blocks for Visual flows.
//
// Any module can add blocks; fn::Flows (the engine and editor) picks them up.
// A block runs on the loop task, gets its parameters (with {value} filled in)
// and the value flowing through the wire, and says which output to follow.
//
//   fn::flowNode({"buzzer.beep", "action", "Beep", "bell", "Short beep on the buzzer",
//                 1, {"next"}, {{"ms", "Length (ms)", "number", "200"}},
//                 [](fn::FlowCtx& c) { beep(c.num("ms", 200)); return fn::NodeResult::next(); }});
#pragma once

#include <Arduino.h>
#include <functional>
#include <deque>
#include <map>
#include <vector>

namespace fn {

class App;

struct FlowParam {
  const char* key;
  const char* label;
  const char* kind;   // text | number | select | color | bool | widget | light | effect | pin
  const char* def;
  const char* opts;   // select options "a|b|c"
};

struct NodeResult {
  int8_t out = 0;        // output to follow, -1 = stop here
  uint32_t waitMs = 0;   // wait before following the output
  bool async = false;    // the block finishes later (see FlowCtx::resume)
  static NodeResult next(int8_t o = 0) { NodeResult r; r.out = o; return r; }
  static NodeResult stop() { NodeResult r; r.out = -1; return r; }
  static NodeResult wait(uint32_t ms, int8_t o = 0) { NodeResult r; r.out = o; r.waitMs = ms; return r; }
};

struct FlowCtx {
  App& app;
  const std::map<String, String>& params;
  String value;           // value travelling along the wire (in and out)
  uint32_t tokenId;
  String flowId;
  // Parameter with {value}, {uptime}, {time}, {id}, {name} filled in.
  String str(const char* key) const;
  long num(const char* key, long def = 0) const;
  bool flag(const char* key, bool def = false) const;
  String raw(const char* key) const;
  // For async blocks: continue the waiting run later (any task).
  std::function<void(int8_t out, const String& value)> resume;
};

struct NodeDef {
  String type, cat, title, icon, desc;
  uint8_t inputs = 1;                 // 0 for triggers
  std::vector<String> outputs;        // labels, one per output
  std::vector<FlowParam> params;
  std::function<NodeResult(FlowCtx&)> run;  // empty for triggers (the engine fires them)
};

// Registry shared by all modules. Later definitions with the same type are
// ignored. Entries never move (deque), so NodeDef pointers stay valid.
void flowNode(const NodeDef& def);
const std::deque<NodeDef>& flowNodes();
const NodeDef* flowNodeFind(const String& type);

// GPIO bookkeeping, so a flow cannot drive a pin that a module (an LED, a bus)
// or the chip itself (SPI flash, USB) is using. Modules claim their pins in begin().
void claimPin(int pin, const char* owner);
// Who uses this pin: a module name, "flash", "USB", or nullptr when free.
const char* pinOwner(int pin);
// Checks a pin for a flow block; returns "" when fine, otherwise the reason.
String pinProblem(int pin, bool output);

}  // namespace fn
