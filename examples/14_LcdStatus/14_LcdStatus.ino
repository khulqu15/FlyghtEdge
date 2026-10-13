// FlyghtEdge — 14 LCD status: a 16x2 character display, controlled from the dashboard.
//
// An HD44780 LCD with a PCF8574 "backpack" (the little board with four pins) shows the device name and
// the uptime. A toggle on the dashboard switches the backlight; Devices > LCD mirrors the text and lets
// you type a message to it.
//
// Wiring (I2C):   SDA, SCL, VCC 5 V (most backpacks), GND. The address is found by itself (0x27 or 0x3F).
// Many backpacks run the display at 5 V: the I2C lines are fine at 3.3 V on the ESP32.
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
const int I2C_SDA = 21, I2C_SCL = 22;
#elif CONFIG_IDF_TARGET_ESP32S3
const int I2C_SDA = 8, I2C_SCL = 9;
#else  // ESP32-C3, C6, S2 ...
const int I2C_SDA = 8, I2C_SCL = 9;
#endif

fn::Device dev("lcd-status");
fn::LcdI2c lcd(16, 2);   // 16 columns, 2 rows

void setup() {
  Serial.begin(115200);

  lcd.pins(I2C_SDA, I2C_SCL);

  dev.page("Display", "monitor")
      .toggle("backlight", "Backlight", [](bool on) { lcd.backlight(on); }).icon("bulb");
  dev.ui().set("backlight", true);

  dev.use(lcd);
  dev.license(FN_LICENSE_KEY);
  dev.begin();

  lcd.line(0, "FlyghtEdge");
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 1000) return;
  last = millis();

  lcd.line(1, String("Up ") + millis() / 1000 + " s");
}
