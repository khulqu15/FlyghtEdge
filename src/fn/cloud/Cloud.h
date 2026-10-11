// FlyghtEdge — Flyght Cloud: claim the device, then mirror the dashboard over MQTT.
//
//   dev.cloud().enable();                 // off by default; or turn it on from the dashboard
//   dev.license(FN_LICENSE_KEY);          // the same key is the claim key
//
// What it does (the device contract of the Flyght Cloud service):
//   1. POST <site>/api/v1/device/claim with the license key -> a device token + where to connect.
//   2. MQTT to the broker the server names, as <prefix>/<DEVICE_ID>/{status,info,ui,state/*,set/*}.
//   3. After a refusal, GET <site>/api/v1/device/config, then connect to what it says.
//
// One address only is built in: the site (FN_CLOUD_SITE below, or dev.cloud().site(...)).
// Everything else, the broker included, comes from the server, and the device remembers the
// site the server last named, so moving the server needs no firmware change (see CloudConfig.h).
// Separate from the local Mqtt module: the user's own broker / Home Assistant keeps working.
#pragma once

#include <mqtt_client.h>

#include "../ui/Ui.h"
#include "CloudConfig.h"

namespace fn {

class Cloud : public Module {
public:
  enum class State : uint8_t { Off, NoNetwork, NeedKey, Claiming, Fetching, Connecting, Online, Retry, Stopped };

  const char* id() const override { return "cloud"; }
  const char* title() const override { return "Flyght Cloud"; }
  const char* deps() const override { return "ui,?web"; }

  // Code defaults (first boot); the dashboard / PUT /fn/api/v1/cloud can change them.
  Cloud& enable(bool on = true) { defEnabled_ = on; return *this; }
  Cloud& site(const char* url) { defSite_ = url; return *this; }     // a saved site wins over this
  Cloud& apiKey(const char* key) { defKey_ = key; return *this; }    // default: the key given to license()

  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  State state() const { return state_; }
  bool connected() const { return connected_; }
  // The site in use now, and the link/QR that claims this device in the browser.
  String site() const;
  String activateUrl() const;

private:
  struct Http {
    int status = 0;
    String body, retryAfter;
  };

  void load();
  void routes(Web& web);
  String keyInUse() const;
  void setState(State s, const String& why = String());
  void schedule(uint32_t inMs) { nextAt_ = millis() + inMs; }
  void backoff();

  // HTTPS to the server (one request at a time, on its own task)
  void claim();
  void fetchConfig(bool afterRefusal);
  void request(const String& url, const String& body, bool post, const String& bearer, std::function<void(const Http&)> done);
  void onClaimed(const Http& r);
  void onConfig(const Http& r, bool afterRefusal);
  bool takeConfig(const JsonRef& data, bool withToken);
  void siteTried(bool reached);
  void forgetToken();

  // MQTT to the broker the server named
  void startMqtt();
  void stopMqtt();
  void onMqttUp();
  void onMqttDown(const char* why);
  void onRefused();
  void onMessage(const String& topic, const String& payload);
  void publishWidget(const Widget& w);
  void publishAll();
  bool publish(const String& topic, const String& payload, bool retain, int qos);
  static void mqttEvent(void* arg, esp_event_base_t base, int32_t id, void* data);

  Ui* ui_ = nullptr;
  Kv kv_{"fn.cloud"};
  esp_mqtt_client_handle_t client_ = nullptr;

  // settings (NVS) and code defaults
  bool enabled_ = false, defEnabled_ = false;
  String savedSite_, defSite_, key_, defKey_, token_;
  String mqttUrl_, prefix_ = "flyght", httpConfig_, httpTelemetry_, httpCommands_;
  uint16_t keepalive_ = 30;

  // runtime
  State state_ = State::Off;
  String why_, base_, lwt_;
  bool connected_ = false, busy_ = false, refusedHandled_ = false;
  bool configFresh_ = false;   // config asked once this boot (or answered with the claim)
  uint8_t mqttFails_ = 0;      // broker attempts in a row that did not connect
  uint32_t nextAt_ = 0, upAt_ = 0;
  int attempt_ = 0, siteFails_ = 0;
  uint32_t tx_ = 0, rx_ = 0, connects_ = 0;
  String partTopic_, partData_;
};

}  // namespace fn
