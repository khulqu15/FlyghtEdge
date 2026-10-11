// FlyghtNinno — outbound HTTP webhook for external systems (Node-RED, n8n,
// Zapier, your own backend...).
//
// POSTs JSON when widget values change (debounced) and/or on an interval:
//   {"event":"change","device":"A1B2..","name":"Lampu Pintar","ts":1730000000,
//    "uptime":123,"values":{"lampu":true,"suhu":27.5}}
// With a secret set, each request carries
//   X-Flyght-Signature: sha256=<hex HMAC-SHA256(secret, body)>
// The request runs on its own task, so a slow server never blocks loop().
#pragma once

#include "../ui/Ui.h"

namespace fn {

class Webhook : public Module {
public:
  const char* id() const override { return "webhook"; }
  const char* title() const override { return "HTTP webhook"; }
  const char* deps() const override { return "ui,?web"; }

  Webhook& url(const char* u) { defUrl_ = u; return *this; }
  Webhook& secret(const char* s) { defSecret_ = s; return *this; }
  Webhook& interval(uint16_t sec) { defInterval_ = sec; return *this; }
  // PEM root certificate for https URLs. Without it TLS is encrypted but the
  // server certificate is not verified (shown as a warning in the UI).
  Webhook& caCert(const char* pem) { ca_ = pem; return *this; }

  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  // Queue a custom event, e.g. hook.send("alarm", json) from your sketch.
  bool send(const char* event, const JsonRef& extra = JsonRef());

private:
  struct Job { String url, secret, body; bool test; };
  void load();
  void routes(Web& web);
  bool enqueue(const char* event, bool allValues, bool test, const JsonRef& extra);
  static void worker(void* arg);
  void deliver(Job& job);

  Ui* ui_ = nullptr;
  Kv kv_{"fn.hook"};
  bool enabled_ = false, onChange_ = true;
  String url_, secret_;
  uint16_t interval_ = 0, minGapMs_ = 2000;
  const char* ca_ = nullptr;

  std::vector<String> pending_;  // widget ids changed since the last send
  uint32_t lastSend_ = 0, lastInterval_ = 0;
  QueueHandle_t q_ = nullptr;

  // stats (written by the worker task, read under App::Guard)
  int lastCode_ = 0;
  String lastError_;
  uint32_t sent_ = 0, failed_ = 0, lastAt_ = 0;

  String defUrl_, defSecret_;
  uint16_t defInterval_ = 0;
};

}  // namespace fn
