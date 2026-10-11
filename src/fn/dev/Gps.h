// FlyghtEdge — GPS / GNSS receiver over UART (NMEA: u-blox, ATGM336H, Quectel ...).
//
//   fn::Gps gps(/*rx*/ 16, /*tx*/ 17);      // 9600 baud on Serial1
//   dev.use(gps);
//   if (gps.hasFix()) Serial.println(gps.fix().lat, 6);
//
// Adds a "Map" page (Leaflet, served from the device) with position, track,
// speed, altitude and satellites. Map tiles load when the phone has internet;
// the track and numbers always work offline. The GPS also sets the clock.
#pragma once

#include <HardwareSerial.h>

#include "../pure/PureTypes.h"
#include "../web/Web.h"

namespace fn {

class Gps : public Module {
public:
  // Serial1 exists on every ESP32; pass Serial2 on ESP32 / ESP32-S3 if Serial1 is busy.
  Gps(int rxPin, int txPin = -1, uint32_t baud = 9600, HardwareSerial& port = Serial1)
    : rx_(rxPin), tx_(txPin), baud_(baud), port_(port) {}
  Gps& track(uint16_t maxPoints, float minMeters = 3) { maxPts_ = maxPoints; minM_ = minMeters; return *this; }
  Gps& setClock(bool on) { setClock_ = on; return *this; }

  const char* id() const override { return "gps"; }
  const char* title() const override { return "GPS"; }
  const char* deps() const override { return "?web"; }
  bool begin(App& app) override;
  void loop(uint32_t now) override;
  Health health() const override;
  void describe(JsonRef out) const override;

  const pure::GpsFix& fix() const { return parser_.fix(); }
  bool hasFix() const { return fix().valid || fix().quality > 0; }
  double lat() const { return fix().lat; }
  double lon() const { return fix().lon; }
  void clearTrack() { App::Guard g; track_.clear(); }

private:
  void routes(Web& web);
  void snapshot(JsonRef out) const;
  void onSentence(uint32_t now);

  int rx_, tx_;
  uint32_t baud_;
  HardwareSerial& port_;
  pure::NmeaParser parser_;
  struct Pt { float lat, lon; };
  std::vector<Pt> track_;
  uint16_t maxPts_ = 400;
  float minM_ = 3;
  bool setClock_ = true, clockSet_ = false;
  uint32_t lastByte_ = 0, lastPub_ = 0, lastPt_ = 0, bytes_ = 0;
  Web* web_ = nullptr;
};

}  // namespace fn
