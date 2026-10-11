// FlyghtEdge — SD card (SPI on every ESP32, or SDMMC on ESP32/S3) + web file manager.
//
//   fn::SdCard sd(5);                 // SPI, CS = GPIO5, default SPI pins
//   sd.pins(18, 19, 23);              // optional SCK, MISO, MOSI
//   dev.use(sd);
//   File f = sd.fs().open("/log.csv", FILE_APPEND);
//
// Adds a "Files" page: folders, upload, download, rename, delete, preview.
// If your own code also writes files while people browse, wrap it in
// { fn::SdCard::Lock l(sd); ... } so the two never collide.
#pragma once

#include <FS.h>
#include <SPI.h>
#include <soc/soc_caps.h>

#include "../web/Web.h"

namespace fn {

class SdCard : public Module {
public:
  explicit SdCard(int cs = SS, SPIClass& spi = SPI) : cs_(cs), spi_(spi) {}
  SdCard& pins(int sck, int miso, int mosi) { sck_ = sck; miso_ = miso; mosi_ = mosi; return *this; }
  SdCard& frequency(uint32_t hz) { hz_ = hz; return *this; }
#if SOC_SDMMC_HOST_SUPPORTED
  // Use the SDMMC peripheral instead of SPI (ESP32, ESP32-S3).
  // On the S3 pins are free to choose; on the ESP32 they are fixed (pass -1).
  SdCard& mmc(bool oneBit = true, int clk = -1, int cmd = -1, int d0 = -1) {
    mmc_ = true; oneBit_ = oneBit; clk_ = clk; cmd_ = cmd; d0_ = d0; return *this;
  }
#endif

  const char* id() const override { return "sd"; }
  const char* title() const override { return "SD card"; }
  const char* deps() const override { return "?web"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  bool mounted() const { return fs_ != nullptr; }
  fs::FS& fs();
  bool remount();

  struct Lock {
    explicit Lock(SdCard& s) : s_(s) { xSemaphoreTakeRecursive(s_.mtx_, portMAX_DELAY); }
    ~Lock() { xSemaphoreGiveRecursive(s_.mtx_); }
    SdCard& s_;
  };

private:
  void routes(Web& web);
  uint64_t total() const;
  uint64_t used() const;
  const char* typeName() const;

  int cs_;
  SPIClass& spi_;
  int sck_ = -1, miso_ = -1, mosi_ = -1;
  uint32_t hz_ = 20000000;
  bool mmc_ = false, oneBit_ = true;
  int clk_ = -1, cmd_ = -1, d0_ = -1;
  fs::FS* fs_ = nullptr;
  SemaphoreHandle_t mtx_ = nullptr;
  uint32_t nextTry_ = 0, uploads_ = 0, downloads_ = 0;
};

}  // namespace fn
