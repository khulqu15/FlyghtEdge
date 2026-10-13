// FlyghtEdge — 12 TFT status: put a value on a small colour screen, and mirror it to the dashboard.
//
// A 240x240 ST7789 panel shows the device name, uptime and a value. The dashboard's Devices > Screen
// page shows the same pixels live, so you can see what the panel shows from your phone, and draw on it.
//
// Wiring (SPI):   SCK, MOSI (SDA), CS, DC, RST (optional), BL (backlight, optional), VCC 3.3 V, GND.
// Panels differ: for an ILI9341 use fn::TftSpi::ILI9341 and 240x320; if the picture is shifted or
// colours are inverted, try another rotation (tft.rotation(0..3)).
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
const int SPI_SCK = 18, SPI_MISO = 19, SPI_MOSI = 23, TFT_CS = 15, TFT_DC = 2, TFT_RST = 4, TFT_BL = 32;
#elif CONFIG_IDF_TARGET_ESP32S3
const int SPI_SCK = 12, SPI_MISO = 13, SPI_MOSI = 11, TFT_CS = 14, TFT_DC = 15, TFT_RST = 16, TFT_BL = 21;
#else  // ESP32-C3, C6, S2 ...
const int SPI_SCK = 4, SPI_MISO = 5, SPI_MOSI = 6, TFT_CS = 1, TFT_DC = 2, TFT_RST = -1, TFT_BL = 3;
#endif

// RGB565 colours
const uint16_t BLACK = 0x0000, WHITE = 0xFFFF, GREEN = 0x07E0, AMBER = 0xFD20;

fn::Device dev("tft-status");
fn::TftSpi panel(fn::TftSpi::ST7789, 240, 240, TFT_CS, TFT_DC, TFT_RST);
fn::Tft tft(panel);

void setup() {
  Serial.begin(115200);

  panel.pins(SPI_SCK, SPI_MOSI, SPI_MISO);
  tft.backlight(TFT_BL);

  dev.page("Status", "monitor")
      .value("chip", "Chip temperature", "°C").decimals(1).icon("thermo");

  dev.use(tft);
  dev.license(FN_LICENSE_KEY);
  dev.begin();

  tft.clear(BLACK);
  tft.text(12, 12, "FlyghtEdge", WHITE, 3);
  tft.rect(12, 52, 216, 2, GREEN, true);
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 1000) return;
  last = millis();

  const float t = temperatureRead();   // the chip's own temperature sensor
  dev.ui().set("chip", t);

  // text() with a background colour overwrites the old value: no flicker from a full clear
  tft.text(12, 70, String("Uptime ") + millis() / 1000 + " s    ", WHITE, 2, BLACK);
  tft.text(12, 110, String(t, 1) + " C   ", t > 60 ? AMBER : GREEN, 4, BLACK);
}
