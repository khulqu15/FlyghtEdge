// FlyghtNinno — 02 Sensor + MQTT + HTTP
//
// A small greenhouse controller: live chart, gauge, fan speed slider, mode
// select and a pump button. Every widget is also:
//   * on MQTT   flyght/<id>/state/<widget>  and  flyght/<id>/set/<widget>
//   * on HTTP   GET/POST http://<host>.local/fn/api/v1/state
//   * in Home Assistant (discovery) when enabled in Connect > MQTT.
// The broker can also be set from the web UI; the line below is optional.

#include <FlyghtNinno.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

const int FAN_PIN = 4;
const int PUMP_PIN = 5;

fn::Device dev("green-house", "1.2.0");

void setup() {
  Serial.begin(115200);
  pinMode(PUMP_PIN, OUTPUT);
  ledcAttach(FAN_PIN, 25000, 8);

  dev.mqtt().broker("mqtt://192.168.1.10:1883").homeAssistant(true);  // optional default
  dev.auth().allowAnonymous(true, fn::Role::Viewer);

  dev.page("Monitor", "activity")
      .chart("temp", "Air temperature", "°C", 90).every(2000).size(2).icon("thermo")
      .gauge("humidity", "Humidity", "%", 0, 100).icon("droplet")
      .value("soil", "Soil moisture", "%").decimals(0).icon("droplet");

  dev.page("Control", "sliders")
      .slider("fan", "Fan speed", 0, 100, 5, [](float v) { ledcWrite(FAN_PIN, (uint32_t)(v * 2.55f)); }).unit("%").icon("fan")
      .select("mode", "Mode", {"Auto", "Manual", "Away"}, [](const String& m) { Serial.println("mode: " + m); })
      .button("water", "Water for 5 s", [] {
        digitalWrite(PUMP_PIN, HIGH);
        dev.sched().after(5000, [] { digitalWrite(PUMP_PIN, LOW); });
      }).icon("droplet");

  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last >= 2000) {
    last = millis();
    float t = 27.0 + 3.0 * sin(millis() / 60000.0);  // replace with a real sensor
    dev.ui().set("temp", t);
    dev.ui().set("humidity", 62.0 + 8.0 * cos(millis() / 45000.0));
    dev.ui().set("soil", 40 + (millis() / 1000) % 20);
  }
}
