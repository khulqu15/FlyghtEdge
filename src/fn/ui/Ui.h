// FlyghtEdge — dashboard widgets declared from C++, no HTML required.
//
//   ui.page("Kontrol", "home")
//     .toggle("lampu", "Lampu Teras", [](bool on) { digitalWrite(2, on); })
//     .slider("terang", "Kecerahan", 0, 100, 1, [](float v) { ... }).unit("%")
//     .gauge ("suhu", "Suhu", "°C", 0, 50)
//     .chart ("riwayat", "Riwayat suhu", "°C").size(3);
//
//   ui.set("suhu", 27.4);      // from loop(); pushed live to every browser
//
// Every widget is automatically available over the HTTP API
// (/fn/api/v1/state) and MQTT (<base>/state/<id>, <base>/set/<id>).
#pragma once

#include <initializer_list>

#include "../web/Web.h"

namespace fn {

enum class WidgetType : uint8_t { Toggle, Button, Slider, Gauge, Value, Chart, Select, Text };
enum class Source : uint8_t { Code, Web, Mqtt, Http, Cloud };

const char* widgetTypeName(WidgetType t);
const char* sourceName(Source s);

struct Widget {
  String id, label, unit, page, icon;
  String device;           // id of the device module behind the card; the card menu then offers its pin settings
  WidgetType type = WidgetType::Value;
  float min = 0, max = 100, step = 1;
  uint8_t size = 1;        // grid columns: 1 small, 2 wide, 3 full row
  uint8_t decimals = 1;
  uint8_t writeRole = Role::Operator;
  std::vector<String> options;

  // current value
  double num = NAN;
  String text;
  bool on = false;

  // chart history
  std::vector<float> hist;
  uint8_t histMax = 60;
  uint32_t histEveryMs = 1000, histAt = 0;

  std::function<void(bool)> onBool;
  std::function<void(float)> onNum;
  std::function<void()> onPress;
  std::function<void(const String&)> onText;

  bool dirty = false;
  Source source = Source::Code;

  bool writable() const { return type == WidgetType::Toggle || type == WidgetType::Button || type == WidgetType::Slider || type == WidgetType::Select || type == WidgetType::Text; }
  bool numeric() const { return type == WidgetType::Slider || type == WidgetType::Gauge || type == WidgetType::Value || type == WidgetType::Chart; }
  bool textual() const { return type == WidgetType::Select || type == WidgetType::Text || (type == WidgetType::Value && text.length() && isnan(num)); }
  // Plain-text value for MQTT payloads ("ON"/"OFF", numbers, text).
  String payload() const;
  void valueTo(JsonRef obj, const char* key) const;
};

class Ui;

class Page {
public:
  Page& toggle(const char* id, const char* label, std::function<void(bool)> cb = nullptr);
  Page& button(const char* id, const char* label, std::function<void()> cb);
  Page& slider(const char* id, const char* label, float min, float max, float step = 1, std::function<void(float)> cb = nullptr);
  Page& gauge(const char* id, const char* label, const char* unit, float min, float max);
  Page& value(const char* id, const char* label, const char* unit = "");
  Page& chart(const char* id, const char* label, const char* unit = "", uint8_t points = 60);
  Page& select(const char* id, const char* label, std::initializer_list<const char*> options, std::function<void(const String&)> cb = nullptr);
  Page& text(const char* id, const char* label, std::function<void(const String&)> cb = nullptr);

  // Modifiers for the widget added last.
  Page& size(uint8_t cols);
  Page& icon(const char* name);
  Page& unit(const char* u);
  Page& decimals(uint8_t d);
  Page& role(uint8_t minRoleToChange);
  Page& device(const char* deviceId);   // the module (a PinDevice) that owns this card
  Page& every(uint32_t chartSampleMs);

  const String& id() const { return id_; }
  const String& name() const { return name_; }
  const String& iconName() const { return icon_; }

private:
  friend class Ui;
  Page(Ui* ui, const String& name, const String& icon);
  Widget& add(const char* id, const char* label, WidgetType type);
  Ui* ui_;
  String id_, name_, icon_;
  Widget* last_ = nullptr;
};

class Ui : public Module {
public:
  const char* id() const override { return "ui"; }
  const char* title() const override { return "Dashboard"; }
  const char* deps() const override { return "web,?auth"; }

  Page& page(const char* name, const char* icon = "layout-grid");
  Ui& accent(const char* cssColor) { accent_ = cssColor; return *this; }
  Ui& pushInterval(uint16_t ms) { pushMs_ = ms; return *this; }

  // --- values (call from the loop task) ---
  void set(const char* id, double v);
  void set(const char* id, float v) { set(id, (double)v); }
  void set(const char* id, int v) { set(id, (double)v); }
  void set(const char* id, long v) { set(id, (double)v); }
  void set(const char* id, unsigned v) { set(id, (double)v); }
  void set(const char* id, unsigned long v) { set(id, (double)v); }
  void set(const char* id, bool v);
  void set(const char* id, const char* v);
  void set(const char* id, const String& v) { set(id, v.c_str()); }
  double number(const char* id) const;
  bool flag(const char* id) const;
  String text(const char* id) const;

  // Parses raw input (from web/MQTT/HTTP) for a widget, runs its callback.
  // Must run on the loop task. Returns false with a reason on bad input.
  bool apply(const String& id, const String& raw, Source src, String* err = nullptr);
  // Checks input without applying it (any task).
  bool validate(const String& id, const String& raw, uint8_t role, int* httpCode, String* err) const;

  // Integrations subscribe here; called on the loop task for each change.
  void listen(std::function<void(const Widget&, Source)> fn) { listeners_.push_back(std::move(fn)); }

  // The "ui" message of Flyght Cloud: pages and widgets without values, history, plugins or accent.
  // Widgets whose id the cloud would not accept are left out, at most maxWidgets are listed.
  void cloudSchema(JsonRef out, size_t maxWidgets = 64) const;

  Widget* find(const String& id) const;
  const std::vector<Widget*>& widgets() const { return widgets_; }
  void values(JsonRef out) const;

  bool begin(App& app) override;
  void loop(uint32_t now) override;
  void describe(JsonRef out) const override;

private:
  friend class Page;
  void touch(Widget& w, Source src);
  void routes(Web& web);
  void schema(JsonRef out) const;
  void loadLayout();
  bool saveLayout();
  static String rawFromJson(const JsonRef& v);

  Web* web_ = nullptr;
  std::vector<Page*> pages_;
  std::vector<Widget*> widgets_;
  std::vector<std::function<void(const Widget&, Source)>> listeners_;
  String accent_ = "";
  uint16_t pushMs_ = 150;
  uint32_t lastPush_ = 0, pushes_ = 0;
  Kv kv_{"fn.ui"};
  String layoutJson_ = "{}";
};

}  // namespace fn
