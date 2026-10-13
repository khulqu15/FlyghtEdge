// FlyghtEdge — 13 SD logger: write a CSV log to an SD card and browse it from the dashboard.
//
// Every 10 seconds one line is appended to /log.csv (time, uptime, chip temperature). Devices > Files
// is a file manager for the card: browse, preview, download, rename, delete and upload.
//
// Wiring (SPI):   SCK, MISO, MOSI, CS, VCC 3.3 V (or 5 V on boards with a regulator), GND.
// Format the card FAT32. The log keeps working if the card is removed: lines are skipped, not lost
// to a crash, and writing resumes when a card is back (the page shows "mounted").
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
const int SPI_SCK = 18, SPI_MISO = 19, SPI_MOSI = 23, SD_CS = 5;
#elif CONFIG_IDF_TARGET_ESP32S3
const int SPI_SCK = 12, SPI_MISO = 13, SPI_MOSI = 11, SD_CS = 10;
#else  // ESP32-C3, C6, S2 ...
const int SPI_SCK = 4, SPI_MISO = 5, SPI_MOSI = 6, SD_CS = 7;
#endif

fn::Device dev("sd-logger");
fn::SdCard sd(SD_CS);

uint32_t rows = 0;

void setup() {
  Serial.begin(115200);

  sd.pins(SPI_SCK, SPI_MISO, SPI_MOSI);

  dev.page("Logger", "list")
      .value("card", "SD card").icon("plug")
      .value("rows", "Rows written").decimals(0).icon("activity");

  dev.use(sd);
  dev.license(FN_LICENSE_KEY);
  dev.begin();
}

void appendRow() {
  if (!sd.mounted()) return;
  fn::SdCard::Lock lock(sd);   // the file manager and this sketch share the card
  const bool fresh = !sd.fs().exists("/log.csv");
  File f = sd.fs().open("/log.csv", FILE_APPEND);
  if (!f) return;
  if (fresh) f.println("unix_time,uptime_s,chip_c");
  f.printf("%lu,%lu,%.1f\n", (unsigned long)fn::unixTime(), (unsigned long)(millis() / 1000), (double)temperatureRead());
  f.close();
  ++rows;
}

void loop() {
  dev.loop();

  static uint32_t last = 0;
  if (millis() - last < 10000) return;
  last = millis();

  appendRow();
  dev.ui().set("card", sd.mounted() ? "mounted" : "no card");
  dev.ui().set("rows", (unsigned long)rows);
}
