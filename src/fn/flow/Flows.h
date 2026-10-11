// FlyghtNinno — Visual flows: automations built by connecting blocks
// (like a Figma prototype or Blender's node editor), stored on the device.
//
//   fn::Flows flows;
//   dev.license(FN_LICENSE_KEY);            // flows are a Pro feature: a Pro key
//   dev.features().defaultOn("flows");
//   dev.use(flows);
//
// Triggers (boot, interval, widget, MQTT, HTTP hook, Aperture, button, manual)
// start a run; the run walks the wires through logic (delay, if, repeat, split)
// and actions (lights, widgets, GPIO, MQTT, HTTP, LCD, MCU ... any module
// can add blocks with fn::flowNode()).
//
// Branches: a block with several wires runs them in PARALLEL; the Split
// block can run its outputs one after another (SERIAL) instead.
// Threads: a flow runs on the main loop, or (Enterprise) on its own FreeRTOS
// task so its timing is independent of the rest of the app.
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "../web/Web.h"
#include "Nodes.h"

namespace fn {

class Flows : public Module {
public:
  const char* id() const override { return "flows"; }
  const char* title() const override { return "Visual flows"; }
  const char* deps() const override { return "web,?ui,?mqtt"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  void onEvent(const Event& e) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  // Fire every "Manual" trigger of a flow. Returns false if none.
  bool run(const String& flowId, const String& value = String());
  // Fire every "HTTP hook" trigger with this name.
  int hook(const String& name, const String& value);
  // Fire every "Aperture" trigger that listens to this feed ("people" | "ppe")
  // and camera ("" matches any). `count` is the number on the wire.
  int aperture(const String& feed, const String& camera, long count);

  static constexpr size_t kMaxFlows = 8, kMaxNodes = 40, kMaxWires = 80, kMaxTokens = 16;

private:
  struct Node {
    String id, type;
    int16_t x = 0, y = 0;
    std::map<String, String> p;
    const NodeDef* def = nullptr;
    uint32_t last = 0;      // interval / button / Aperture bookkeeping
    bool level = true;
    bool seen = false;      // Aperture: an event has arrived before
  };
  struct Wire { uint8_t from, out, to; };
  struct Cont { uint8_t node; bool reenter; };
  struct Token {
    uint32_t id;
    uint8_t node;
    bool waiting = false, async = false, reenter = false;
    int8_t pendingOut = 0;
    uint32_t wakeAt = 0;
    String value;
    std::vector<Cont> stack;
    std::vector<uint16_t> counters;
  };
  struct Flow {
    String id, name;
    bool on = true, rtos = false;
    std::vector<Node> nodes;
    std::vector<Wire> wires;
    std::vector<Token> tokens;
    uint32_t runs = 0, lastRun = 0, steps = 0, dropped = 0;
    TaskHandle_t task = nullptr;
    SemaphoreHandle_t mtx = nullptr;
    volatile bool stop = false;
    Flows* owner = nullptr;
  };

  bool allowed(Response* s) const;
  void defineBuiltins();
  Flow* parse(const JsonRef& j, String* err) const;
  void toJson(const Flow& f, JsonRef out, bool full) const;
  bool load();
  bool store(const Flow& f);
  void install(Flow* f);
  void uninstall(Flow* f);
  Flow* find(const String& id) const;
  void fire(Flow& f, uint8_t trigger, const String& value);
  void step(Flow& f, uint32_t now);
  void exec(Flow& f, Token& t, uint32_t now);
  void follow(Flow& f, Token& t, int8_t out);
  void endBranch(Flow& f, Token& t);
  void event(const Flow& f, uint8_t node);
  void routes(Web& web);
  static void taskMain(void* arg);
  void triggersLoop(uint32_t now);

  std::vector<Flow*> flows_;
  Kv kv_{"fn.flows"};
  Web* web_ = nullptr;
  uint32_t seq_ = 0, lastFlush_ = 0, totalRuns_ = 0;
  bool booted_ = false;
  std::vector<String> mqttTopics_;
  std::vector<String> events_;
  SemaphoreHandle_t evMtx_ = nullptr;
  QueueHandle_t httpQ_ = nullptr;
};

}  // namespace fn
