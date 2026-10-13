// FlyghtEdge — 16 MCU bridge: show the values of another microcontroller and command it.
//
// An Arduino Uno, STM32, RP2040 or another ESP is wired to a UART and talks in plain lines:
//   MCU -> ESP32   "temp=23.4,hum=61"     every key becomes a live value (bound ones fill a card)
//   ESP32 -> MCU   "led=1"                when the "led" card changes
// Devices > Serial adds a console, the values, and one-tap buttons.
//
// A minimal sketch for the other board (Arduino, 115200 baud):
//     void setup() { Serial.begin(115200); pinMode(LED_BUILTIN, OUTPUT); }
//     void loop() {
//       Serial.println("temp=" + String(20 + random(50) / 10.0) + ",hum=" + String(55 + random(10)));
//       while (Serial.available()) {
//         String l = Serial.readStringUntil('\n');
//         if (l.startsWith("led=")) digitalWrite(LED_BUILTIN, l.endsWith("1"));
//       }
//       delay(1000);
//     }
//
// Wiring:   MCU TX -> MCU_RX pin     MCU RX -> MCU_TX pin     common GND.
// Mind the voltage: a 5 V Arduino TX needs a divider (or a level shifter) into a 3.3 V ESP32 pin.
//
// Status: compiled for ESP32-S3; not yet run on a board.
// Tools > Partition Scheme: "No FS 4MB (2MB APP x2)" or use the partitions.csv here.

#include <FlyghtEdge.h>

// License key (API_KEY). Put it in a secrets.h next to this sketch, kept out of
// version control:   #define FN_LICENSE_KEY "fnk_live_..."
// Without a key the app runs in evaluation mode: everything on, 30 min per boot.
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define FN_LICENSE_KEY ""
#endif

// ---- pins (the wiring diagram in docs/guides is generated from this block) ----
#if CONFIG_IDF_TARGET_ESP32
const int MCU_RX = 26, MCU_TX = 27;
#elif CONFIG_IDF_TARGET_ESP32S3
const int MCU_RX = 4, MCU_TX = 5;
#else  // ESP32-C3, C6, S2 ...
const int MCU_RX = 20, MCU_TX = 21;
#endif

fn::Device dev("mcu-bridge");
fn::McuLink mcu(MCU_RX, MCU_TX, 115200, Serial1);

void setup() {
  Serial.begin(115200);

  dev.page("Remote board", "cpu")
      .value("temp", "Temperature", "°C").decimals(1).icon("thermo")
      .value("hum", "Humidity", "%").decimals(0).icon("droplet")
      .toggle("led", "LED").icon("bulb");

  mcu.bind("temp", "temp")        // "temp=.." fills the "temp" card
      .bind("hum", "hum")
      .forward("led")             // the "led" card sends "led=1" / "led=0"
      .quick("Ping", "PING");     // a button in the console
  dev.use(mcu);

  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 5000) return;
  last = millis();
  if (!mcu.online()) Serial.println("[mcu] no data in the last 5 s: check wiring and baud rate");
}
