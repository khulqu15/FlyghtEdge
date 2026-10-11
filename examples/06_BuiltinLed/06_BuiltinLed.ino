// FlyghtNinno — 06 Builtin LED: the onboard LED, a colour picker, effects and Visual flows.
//
// No sensors needed: this tests everything with the LED that is already on
// the board.
//   * Boards with an addressable RGB LED (ESP32-S3/C3/C6 DevKits: RGB_BUILTIN)
//     get the full colour picker and every effect.
//   * Boards with a plain LED (LED_BUILTIN) get on/off, brightness and the
//     single-colour effects (blink, breathe, heartbeat, SOS ...).
//
// Open the dashboard, then:  Devices > Lights   (colour picker + effects)
//                            Devices > Flows    (connect blocks, press Run)
// The BOOT button (GPIO0) can start a flow: try the "Button plays rainbow" example.
//
// Tools > Partition Scheme: "No FS 4MB (2MB APP x2)" or use the partitions.csv here.

#include <FlyghtNinno.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

fn::Device dev("led-lab");

#if defined(RGB_BUILTIN)
fn::RgbLed led(RGB_BUILTIN, "builtin");   // WS2812 pixel on the board
#elif defined(LED_BUILTIN)
fn::Led led(LED_BUILTIN, "builtin");      // plain LED, PWM dimming
#else
fn::Led led(2, "builtin");                // most ESP32 DevKits: GPIO2
#endif

fn::Flows flows;

void setup() {
  Serial.begin(115200);

  // Test build: every feature on at first boot (Settings > Plan & features can
  // change them). Pro / Enterprise ones need a key of that edition, or
  // evaluation mode (no key), which allows everything for 30 min per boot.
  for (const char* id : {"guest", "apiTokens", "flows", "rbac", "whitelabel", "flowsRtos"})
    dev.features().defaultOn(id);

  // Setup Wi-Fi of the board (access point). Join it from a phone or laptop.
  dev.ap().ssid("kendai-c3-lib-001").password("123456789");

  // The LED starts soft blue, breathing. Changes in the UI are remembered.
  led.color(0x2563eb).effect(fn::Fx::Breathe, 3000);
#if defined(RGB_BUILTIN)
  // Red and green swapped? This board's LED takes RGB order (most DevKits use
  // GRB, the default: delete this line there). Also selectable on the Lights page.
  led.colorOrder(fn::ColorOrder::RGB);
#endif

  dev.page("LED", "bulb")
      .toggle("power", "LED", [](bool on) { led.on(on); }).icon("power")
      .slider("bright", "Brightness", 0, 100, 5, [](float v) { led.brightness((uint8_t)v); }).unit("%")
      .select("fx", "Effect", {"solid", "blink", "breathe", "heartbeat", "rainbow", "candle", "police", "sos"},
              [](const String& id) { led.effect(id); })
      .button("party", "Party!", [] { led.effect(fn::Fx::Rainbow, 1500); }).icon("sparkles")
      .value("uptime", "Uptime", "s").icon("activity");

  dev.use(led).use(flows);
  dev.license(FN_LICENSE_KEY);
  dev.begin();

  Serial.printf("[led-lab] LED ready on %s\n", led.isRgb() ? "RGB_BUILTIN" : "a plain GPIO");
}

void loop() {
  dev.loop();

  // Keep the dashboard in step with changes made elsewhere (Lights page,
  // MQTT, flows). Non-blocking: once a second.
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    const auto& s = led.state();
    dev.ui().set("power", s.on);
    dev.ui().set("bright", (int)s.brightness);
    dev.ui().set("uptime", millis() / 1000);
  }
}
