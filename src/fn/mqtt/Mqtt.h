// FlyghtNinno — MQTT bridge for every dashboard widget (uses esp-mqtt from
// the ESP32 core; no extra library).
//
// Topics (base defaults to "flyght/{id}"):
//   <base>/status          online | offline (retained, LWT)
//   <base>/info            device JSON (retained)
//   <base>/state/<widget>  value: ON/OFF, number or text
//   <base>/set/<widget>    command; same formats ("toggle" and "PRESS" too)
// Optional Home Assistant discovery publishes every widget as an entity.
#pragma once

#include <mqtt_client.h>

#include "../ui/Ui.h"

namespace fn {

class Mqtt : public Module {
public:
  const char* id() const override { return "mqtt"; }
  const char* title() const override { return "MQTT"; }
  const char* deps() const override { return "ui,?web"; }

  // Code defaults (first boot); editable on the Integrations page.
  Mqtt& broker(const char* uri) { defUri_ = uri; defEnabled_ = true; return *this; }  // mqtt://host:1883, mqtts://...
  Mqtt& credentials(const char* user, const char* pass) { defUser_ = user; defPass_ = pass; return *this; }
  Mqtt& baseTopic(const char* tpl) { defBase_ = tpl; return *this; }
  Mqtt& homeAssistant(bool on) { defHa_ = on; return *this; }

  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  bool connected() const { return connected_; }
  bool publish(const String& topic, const String& payload, bool retain = false, int qos = 0);
  // Subscribe to any extra topic; callback runs on the loop task.
  void subscribe(const String& topic, std::function<void(const String& topic, const String& payload)> cb);
  String base() const { return base_; }

private:
  struct Sub { String topic; std::function<void(const String&, const String&)> cb; };

  void load();
  void start();
  void stop();
  void onConnected();
  void onMessage(const String& topic, const String& payload);
  void publishWidget(const Widget& w);
  void publishDiscovery();
  void routes(Web& web);
  static void eventHandler(void* arg, esp_event_base_t base, int32_t id, void* data);

  Ui* ui_ = nullptr;
  Kv kv_{"fn.mqtt"};
  esp_mqtt_client_handle_t client_ = nullptr;
  bool enabled_ = false, retain_ = true, ha_ = false, connected_ = false;
  String uri_, user_, pass_, baseTpl_, base_, haPrefix_ = "homeassistant", lastError_;
  String lwtTopic_, clientId_;
  uint32_t tx_ = 0, rx_ = 0, connects_ = 0;
  std::vector<Sub> subs_;
  // partial-message reassembly (esp-mqtt splits large payloads)
  String partTopic_, partData_;

  bool defEnabled_ = false, defHa_ = false;
  String defUri_, defUser_, defPass_, defBase_ = "flyght/{id}";
};

}  // namespace fn
